#include "graph.h"

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

struct NormColor {
	double r, g, b;
};

// range from 0 to 1 in rgb, domain [x,y] both from 0 to one and t for time in seconds
static NormColor myFunc(double x, double y, double t)
{
	NormColor c{};
	// You can tweak frequencies and phase shifts for variety
	//c.r = 0.5 + 0.5 * sin(2 * std::numbers::pi * (x + t * 0.2));
	//c.g = 0.5 + 0.5 * sin(2 * std::numbers::pi * (y + t * 0.3));
	//c.b = 0.5 + 0.5 * sin(2 * std::numbers::pi * (x + y + t * 0.25));

	c.r = (sin((t + x) * 20.0 ) + 1.0) * 0.5;
	c.g = (sin((t + y) * 4.0 ) + 1.0) * 0.5;
	c.b = (cos(t) + 1.0) * 0.5;

	return c;
}

void liveGraph(const CorsairDeviceId* device_id, Leds& leds)
{
	constexpr double FREQUENCY = 20.0; // cycles per second
	auto iters = 1000;

	myPrint("Transmitting at {} Hz for {} sec", FREQUENCY, static_cast<double>(iters) / FREQUENCY);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	auto start = std::chrono::high_resolution_clock::now();
	startFixedUpdateLoop(iters, static_cast<int64_t>(1'000'000.0 / FREQUENCY), [&](int iteration) {
		const double t = static_cast<double>(iteration) / FREQUENCY;
		uint32_t i = 0;
		const auto bounds = leds.getBounds();
		for (const auto& led_position : leds.getAllLedPositions()) {
			const double x = (led_position.cx - bounds.min_x_pos) / (bounds.max_x_pos - bounds.min_x_pos);
			const double y = (led_position.cy - bounds.min_y_pos) / (bounds.max_y_pos - bounds.min_y_pos);
			NormColor c = myFunc(x, y, t);
			leds.setLed(i, static_cast<uint8_t>(c.r * 255.0), static_cast<uint8_t>(c.g * 255.0), static_cast<uint8_t>(c.b * 255.0));
			++i;
		}
		waitForColors();
		setColors(device_id, leds);
		});
	waitForColors();
}