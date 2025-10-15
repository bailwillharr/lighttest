#pragma once

#include <iCUESDK/iCUESDK.h>

#include "corsair_helpers.h"
#include "static_vector.h"

// Maps key indices to CorsairLedLuid values
class Leds {
	static_vector<CorsairLedPosition, CORSAIR_DEVICE_LEDCOUNT_MAX> m_led_positions{};

public:
	Leds(const CorsairDeviceId* device_id)
	{
		int num_positions{};
		CHECKCORSAIR(CorsairGetLedPositions(*device_id, m_led_positions.capacity(), m_led_positions.data(), &num_positions));
		m_led_positions.resize_uninitialized(num_positions);
	}

	CorsairLedLuid getLed(uint32_t index) const
	{
		assert(index < m_led_positions.size());
		return m_led_positions[index].id;
	}

	uint32_t getCount() const {
		return m_led_positions.size();
	}
};