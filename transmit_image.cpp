#include "transmit_image.h"

#include <thread>
#include <chrono>

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

void transmitImage(const CorsairDeviceId* device_id, Leds& leds, const std::filesystem::path& path, int resolution)
{
	auto bitmap = readImage(path);
	if (bitmap.data.empty()) {
		myPrint("Failed to open image: {}", path.string());
		abort();
	}

	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();

	double frequency = 10.0;

	myPrint("LED count: {}", leds.getCount());

	std::this_thread::sleep_for(std::chrono::seconds(1));

	auto start = std::chrono::high_resolution_clock::now();
	startFixedUpdateLoop(leds.getCount(), static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
		leds.setAll(0, 0, 0);
		leds.setLed(65, 255, 255, 255);
		waitForColors();
		setColors(device_id, leds);
		});
	waitForColors();
	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));

}