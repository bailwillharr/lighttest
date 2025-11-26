#include "transmit_image.h"

#include <algorithm>
#include <array>
#include <thread>
#include <chrono>
#include <map>

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

void transmitImage(const CorsairDeviceId* device_id, Leds& leds, const std::filesystem::path& path)
{
	auto bitmap = readImage(path);
	if (bitmap.data.empty()) {
		myPrint("Failed to open image: {}", path.string());
		abort();
	}

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

	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();

	const double frequency = 10.0;
	const int iters = static_cast<int>(ceil(static_cast<double>(bitmap.width * bitmap.height) / 109.0));

	// hack to avoid OOB read
	bitmap.data.resize(iters * 109 * 4);

	myPrint("LED count: {}", leds.getCount());


	std::vector<int> ordered{};

	for (const auto& [i, row] : rows) {
		for (int i : row) {
			ordered.push_back(i);
		}
	}

	assert(ordered.size() == 109);

	myPrint("Transmitting for {} seconds", static_cast<double>(iters) / frequency);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	startFixedUpdateLoop(iters, static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
		leds.setAll(0, 0, 0);
		for (int i = 0; i < 109; ++i) {
			uint8_t r = bitmap.data[(iteration * 109 + i) * 4 + 0];
			uint8_t g = bitmap.data[(iteration * 109 + i) * 4 + 1];
			uint8_t b = bitmap.data[(iteration * 109 + i) * 4 + 2];
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