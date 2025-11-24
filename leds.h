#pragma once

#include <vector>
#include <limits>
#include <span>

#include <iCUESDK/iCUESDK.h>

#include "corsair_helpers.h"
#include "static_vector.h"

/*

65 is mute led
last 2 are corsair logo
ignore these for now

*/

struct LedBounds {
	float min_x_pos{ std::numeric_limits<float>::max() };
	float max_x_pos{ std::numeric_limits<float>::min() };
	float min_y_pos{ std::numeric_limits<float>::max() };
	float max_y_pos{ std::numeric_limits<float>::min() };
};

class Leds {
	static_vector<CorsairLedPosition, CORSAIR_DEVICE_LEDCOUNT_MAX> m_led_positions{};
	static_vector<CorsairLedColor, CORSAIR_DEVICE_LEDCOUNT_MAX> m_led_colors{};
	LedBounds m_bounds{};

public:
	Leds(const CorsairDeviceId* device_id)
	{
		int num_positions{};
		CHECKCORSAIR(CorsairGetLedPositions(*device_id, m_led_positions.capacity(), m_led_positions.data(), &num_positions));
		m_led_positions.resize_uninitialized(num_positions);

		for (const auto& position : m_led_positions) {
			m_led_colors.emplace_back(CorsairLedColor{ .id = position.id, .r = 0, .g = 0, .b = 0, .a = 255 });
			m_bounds.min_x_pos = fminf(position.cx, m_bounds.min_x_pos);
			m_bounds.max_x_pos = fmaxf(position.cx, m_bounds.max_x_pos);
			m_bounds.min_y_pos = fminf(position.cy, m_bounds.min_y_pos);
			m_bounds.max_y_pos = fmaxf(position.cy, m_bounds.max_y_pos);
		}
	}

	const CorsairLedColor* getColorsBuffer() const {
		return m_led_colors.data();
	}

	uint32_t getCount() const {
		return m_led_colors.size();
	}

	const std::span<const CorsairLedPosition> getAllLedPositions() const {
		return m_led_positions;
	}

	const LedBounds& getBounds() const { return m_bounds; }

	std::vector<uint32_t> getLedsInBounds(float left, float top, float width, float height) const
	{
		const float right = left + width;
		const float bottom = top + height;

		std::vector<uint32_t> leds_in_bounds{};

		uint32_t i = 0;
		for (const auto& position : m_led_positions) {
			if (position.cx >= left && position.cx < right && position.cy >= top && position.cy < bottom) {
				leds_in_bounds.push_back(i);
			}
			++i;
		}

		return leds_in_bounds;
	}

	void setLed(uint32_t led_index, uint8_t r, uint8_t g, uint8_t b)
	{
		assert(led_index < m_led_colors.size());
		m_led_colors[led_index].r = r;
		m_led_colors[led_index].g = g;
		m_led_colors[led_index].b = b;
	}

	void setAll(uint8_t r, uint8_t g, uint8_t b)
	{
		for (uint32_t i = 0; i < m_led_colors.size(); ++i) {
			setLed(i, r, g, b);
		}
	}
};