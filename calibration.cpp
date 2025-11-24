#include "calibration.h"

#include <cmath>

#include <chrono>
#include <numbers>
#include <thread>

#include "fixed_update_loop.h"
#include "set_colors.h"
#include "my_print.h"

template <typename T>
static constexpr T lerp(T a, T b, T t) {
	return a + t * (b - a);
}

struct Color {
	uint8_t r, g, b;
};

// range from 0 to 1 in rgb
static Color getColor(uint32_t i)
{
	Color c{};
	
	// 8 permutations per channel for a total of 512 colours
	
	uint8_t b3 = i % 8;
	i /= 8;
	uint8_t g3 = i % 8;
	i /= 8;
	uint8_t r3 = i % 8;

	c.r = (r3 * 255 + 3) / 7; 
	c.g = (g3 * 255 + 3) / 7;
	c.b = (b3 * 255 + 3) / 7;

	return c;
}

void calibrationTransmit(const CorsairDeviceId* device_id, Leds& leds)
{
	constexpr double FREQUENCY = 10.0; // cycles per second
	auto iters = 512;

	myPrint("Transmitting at {} Hz for {} sec", FREQUENCY, static_cast<double>(iters) / FREQUENCY);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	auto start = std::chrono::high_resolution_clock::now();
	startFixedUpdateLoop(iters, static_cast<int64_t>(1'000'000.0 / FREQUENCY), [&](int iteration) {
		Color c = getColor(iteration);
		leds.setAll(c.r, c.g, c.b);
		waitForColors();
		setColors(device_id, leds);
		});
	waitForColors();
	leds.setAll(255, 0, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}