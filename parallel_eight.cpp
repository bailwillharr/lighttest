#include "parallel_eight.h"

#include <array>
#include <chrono>
#include <thread>

#include "set_colors.h"

void transmitParallelEight(const CorsairDeviceId* device_id, Leds& leds, std::span<const uint8_t> data)
{
	constexpr uint8_t colors[8][3]{
		{0, 0, 0},{0, 0, 255},{0, 255, 0},{0, 255, 255},{255, 0, 0},{255, 0, 255},{255, 255, 0},{255, 255, 255} };
	float left = leds.getBounds().min_x_pos;
	const float segment_width = (leds.getBounds().max_x_pos - left) / 4.0f;
	const float top = leds.getBounds().min_y_pos;
	const float segment_height = (leds.getBounds().max_y_pos - top) / 2.0f;
	for (int i = 0; i < 8; ++i) {
		const float segment_top = (i % 2 == 0) ? top : top + segment_height;
		auto segment = leds.getLedsInBounds(left, segment_top, segment_width, segment_height);
		for (auto led_index : segment) {
			leds.setLed(led_index, colors[i][0], colors[i][1], colors[i][2]);
		}
		if (i % 2 == 1) {
			left += segment_width;
		}
	}

	waitForColors();
	setColors(device_id, leds);

	std::this_thread::sleep_for(std::chrono::seconds(5));

}