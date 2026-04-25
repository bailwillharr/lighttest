#include "transmit_image.h"

#include <algorithm>
#include <array>
#include <thread>
#include <chrono>
#include <map>
#include <bit>

#include <conio.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "set_colors.h"
#include "my_print.h"
#include "fixed_update_loop.h"

struct Bitmap {
	std::vector<uint8_t> data;
	uint32_t width;
	uint32_t height;
};

// empty on failure
static Bitmap readImage(const std::filesystem::path& path)
{
	const auto filename = path.filename();
	int32_t x{}, y{}, channels_in_file{};
	std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> data(stbi_load(path.string().c_str(), &x, &y, &channels_in_file, 4), stbi_image_free);
	if (!data || x <= 0 || y <= 0) {
		return {};
	}

	// Image asset format:
	// first 4 bytes is width, second 4 bytes is height, remaining data is just R8G8B8A8_SRGB
	const size_t bitmap_size = (static_cast<size_t>(x) * static_cast<size_t>(y) * 4ULL);

	Bitmap output{};

	output.data = std::vector<uint8_t>(data.get(), data.get() + bitmap_size);
	output.width = x;
	output.height = y;

	return output;
}

static auto getOrdered105(const Leds& leds)
{
	// generate LUT to get keys in order
	std::map<int, std::vector<int>> rows{};
	for (int i = 0; i < leds.getCount(); ++i) {
		auto pos = leds.getAllLedPositions()[i];
		if (pos.cy < 34.3) {
			// There are 112 LEDs, top 3 shouldn't be added
			continue;
		}
		if (pos.cy > 34.3 && pos.cy <= 38.6) {
			rows[0].push_back(i);
		}
		if (pos.cy > 38.6 && pos.cy <= 59.5) {
			rows[1].push_back(i);
		}
		if (pos.cy > 59.5 && pos.cy <= 78.5) {
			rows[2].push_back(i);
		}
		if (pos.cy > 78.5 && pos.cy <= 97.6) {
			rows[3].push_back(i);
		}
		if (pos.cy > 97.6 && pos.cy <= 116.1) {
			rows[4].push_back(i);
		}
		if (pos.cy > 116.1 && pos.cy <= 135.6) {
			rows[5].push_back(i);
		}
	}

	for (auto& [i, row] : rows) {
		std::sort(row.begin(), row.end(), [&](int first, int second) ->bool {
			// returns true if first is less than second
			double x1 = leds.getAllLedPositions()[first].cx;
			double x2 = leds.getAllLedPositions()[second].cx;
			return x1 < x2;
			});
	}

	// remove multimedia keys
	rows[0].pop_back();
	rows[0].pop_back();
	rows[0].pop_back();
	rows[0].pop_back();

	std::vector<int> ordered{};

	for (const auto& [i, row] : rows) {
		for (int i : row) {
			ordered.push_back(i);
		}
	}

	assert(ordered.size() == 105);

	return ordered;
}

void transmitImage(const CorsairDeviceId* device_id, Leds& leds, const std::filesystem::path& path)
{
	auto bitmap = readImage(path);
	if (bitmap.data.empty()) {
		myPrint("Failed to open image: {}", path.string());
		abort();
	}

	auto ordered = getOrdered105(leds);

	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();

	const double frequency = 5.0;
	const int iters = static_cast<int>(ceil(static_cast<double>(bitmap.width * bitmap.height) / 105.0));

	// hack to avoid OOB read
	bitmap.data.resize(iters * 105 * 4);

	myPrint("LED count: {}", leds.getCount());

	myPrint("Transmitting for {} seconds", static_cast<double>(iters) / frequency);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	_getch();

	startFixedUpdateLoop(iters, static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
		leds.setAll(0, 0, 0);
		for (int i = 0; i < 105; ++i) {
			uint8_t r = bitmap.data[(iteration * 105 + i) * 4 + 0];
			uint8_t g = bitmap.data[(iteration * 105 + i) * 4 + 1];
			uint8_t b = bitmap.data[(iteration * 105 + i) * 4 + 2];
			leds.setLed(ordered[i], r, g, b);
		}
		waitForColors();
		setColors(device_id, leds);
		});

	waitForColors();

	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();

	std::this_thread::sleep_for(std::chrono::seconds(1));

}

struct Color {
	uint8_t r, g, b;
};

static Color getColor(int i)
{
	Color c{};

	// 128 colors
	// 2 bits for R
	// 3 bits for G
	// 2 bits for B

	int b2 = i % 4;
	i /= 4;
	int g3 = i % 8;
	i /= 8;
	int r2 = i % 4;

	c.r = (r2 * 255) / 3;
	c.g = (g3 * 255) / 7;
	c.b = (b2 * 255) / 3;

	return c;
}

void transmitText(const CorsairDeviceId* device_id, Leds& leds, std::vector<char> text)
{

	// only keep printable chars + newline (\n)
	std::erase_if(text, [](char c) {
		if (c < 0) return true;
		if (c == '\n') return false;
		return !(std::isprint(c));
		});

	// generate LUT to get keys in order
	std::array<std::vector<int>, 6> rows{};
	for (int i = 0; i < leds.getCount(); ++i) {
		auto pos = leds.getAllLedPositions()[i];
		if (pos.cy < 34.3) {
			// There are 112 LEDs, top 3 shouldn't be added
			continue;
		}
		if (pos.cy > 34.3 && pos.cy <= 38.6) {
			rows[0].push_back(i);
			continue;
		}
		if (pos.cy > 38.6 && pos.cy <= 59.5) {
			rows[1].push_back(i);
			continue;
		}
		if (pos.cy > 59.5 && pos.cy <= 78.5) {
			rows[2].push_back(i);
			continue;
		}
		if (pos.cy > 78.5 && pos.cy <= 97.6) {
			rows[3].push_back(i);
			continue;
		}
		if (pos.cy > 97.6 && pos.cy <= 116.1) {
			rows[4].push_back(i);
			continue;
		}
		if (pos.cy > 116.1 && pos.cy <= 135.6) {
			rows[5].push_back(i);
			continue;
		}
		throw std::runtime_error("Key didn't match?");
	}

	for (int i = 0; i < rows.size(); ++i) {
		std::sort(rows[i].begin(), rows[i].end(), [&](int first, int second) ->bool {
			// returns true if first is less than second
			double x1 = leds.getAllLedPositions()[first].cx;
			double x2 = leds.getAllLedPositions()[second].cx;
			return x1 < x2;
			});
	}

	std::array<std::vector<int>, 8> sections{};
	// top left going right, then bottom left going right
	// section 0
	for (int index = 0; index < rows[0].size(); ++index) {
		if (index < 5) {
			sections[0].push_back(rows[0][index]);
			continue;
		}
		if (index < 13) {
			sections[1].push_back(rows[0][index]);
			continue;
		}
		if (index < 16) {
			sections[2].push_back(rows[0][index]);
			continue;
		}
		sections[3].push_back(rows[0][index]);
	}

	for (int index = 0; index < rows[1].size(); ++index) {
		if (index < 6) {
			sections[0].push_back(rows[1][index]);
			continue;
		}
		if (index < 14) {
			sections[1].push_back(rows[1][index]);
			continue;
		}
		if (index < 17) {
			sections[2].push_back(rows[1][index]);
			continue;
		}
		sections[3].push_back(rows[1][index]);
	}

	for (int index = 0; index < rows[2].size(); ++index) {
		if (index < 6) {
			sections[0].push_back(rows[2][index]);
			continue;
		}
		if (index < 13) {
			sections[1].push_back(rows[2][index]);
			continue;
		}
		if (index < 16) {
			sections[2].push_back(rows[2][index]);
			continue;
		}
		sections[3].push_back(rows[2][index]);
	}

	for (int index = 0; index < rows[3].size(); ++index) {
		if (index < 6) {
			sections[4].push_back(rows[3][index]);
			continue;
		}
		if (index < 13) {
			sections[5].push_back(rows[3][index]);
			continue;
		}
		if (index == 13) {
			// big enter key
			sections[1].push_back(rows[3][index]);
			continue;
		}
		if (index == 17) {
			// numpad plus
			sections[3].push_back(rows[3][index]);
			continue;
		}
		sections[7].push_back(rows[3][index]);
	}

	for (int index = 0; index < rows[4].size(); ++index) {
		if (index < 6) {
			sections[4].push_back(rows[4][index]);
			continue;
		}
		if (index < 13) {
			sections[5].push_back(rows[4][index]);
			continue;
		}
		if (index < 14) {
			sections[6].push_back(rows[4][index]);
			continue;
		}
		sections[7].push_back(rows[4][index]);
	}
	for (int index = 0; index < rows[5].size(); ++index) {
		if (index < 4) {
			sections[4].push_back(rows[5][index]);
			continue;
		}
		if (index < 8) {
			sections[5].push_back(rows[5][index]);
			continue;
		}
		if (index < 11) {
			sections[6].push_back(rows[5][index]);
			continue;
		}
		sections[7].push_back(rows[5][index]);
	}

	assert(sections[0].size() == 17);
	assert(sections[1].size() == 24);
	assert(sections[2].size() == 9);
	assert(sections[3].size() == 12);
	assert(sections[4].size() == 16);
	assert(sections[5].size() == 18);
	assert(sections[6].size() == 4);
	assert(sections[7].size() == 9);

#if 0
	{
		int section_index = 0;
		for (const auto& section : sections) {
			std::cout << "sections[" << section_index << "] = {\n";
			for (const int i : section) {
				// lookup ordered key index from key code
				auto it = std::find(ordered.begin(), ordered.end(), i);
				if (it == ordered.end()) {
					throw std::runtime_error("BAD!");
				}
				// get index from iterator
				auto ordered_index = it - ordered.begin();
				std::cout << static_cast<int>(ordered_index) << ",\n";
			}
			std::cout << "};\n";
			++section_index;
		}
	}
#endif

	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();

	const double frequency = 20.0;
	const int iters = static_cast<int>(std::ceil(static_cast<double>(text.size()) / 3.0));

	text.resize(iters * 3);

	myPrint("Transmitting for {} seconds", static_cast<double>(iters) / frequency);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	startFixedUpdateLoop(iters, static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
		for (int section = 0; section < 8; ++section) {
			for (const int i : sections[section]) {

				uint8_t r{}, g{}, b{};
				{
					char c1 = text.at(iteration * 3 + 0);
					r = ((c1 >> section) & 1) * 255;
					if (section == 7) {
						// show even parity
						r = (std::popcount(static_cast<unsigned char>(c1)) & 1) * 255;
					}
				}

				{
					char c2 = text.at(iteration * 3 + 1);
					g = ((c2 >> section) & 1) * 255;
					if (section == 7) {
						// show even parity
						g = (std::popcount(static_cast<unsigned char>(c2)) & 1) * 255;
					}
				}

				{
					char c3 = text.at(iteration * 3 + 2);
					b = ((c3 >> section) & 1) * 255;
					if (section == 7) {
						// show even parity
						b = (std::popcount(static_cast<unsigned char>(c3)) & 1) * 255;
					}
				}

				leds.setLed(i, r, g, b);

			}
		}
		waitForColors();
		setColors(device_id, leds);
		});

	waitForColors();

	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();

	std::this_thread::sleep_for(std::chrono::seconds(1));
}