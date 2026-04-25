#include "sampling_test.h"

#include <cmath>

#include <chrono>
#include <numbers>
#include <thread>

#include "fixed_update_loop.h"
#include "set_colors.h"
#include "my_print.h"

void samplingTest(const CorsairDeviceId* device_id, Leds& leds, double frequency, int iterations)
{

	myPrint("Transmitting at {} Hz for {} sec", frequency, static_cast<double>(iterations) / frequency);

	auto start = std::chrono::high_resolution_clock::now();

	waitForColors();

	startFixedUpdateLoop(iterations, static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
		leds.setAll(0, (iteration % 2) * 255, 0);
		waitForColors();
		setColors(device_id, leds);
		});

	waitForColors();
}