#include "set_colors.h"

#include <cassert>

#include <atomic>
#include <span>

#include <iCUESDK/iCUESDK.h>

#include "corsair_helpers.h"

static std::atomic<bool> s_color_set{ true };

static void onColorSet(void*, CorsairError error)
{
	CHECKCORSAIR(error);
	s_color_set.store(true, std::memory_order_relaxed);
}

void setColors(const CorsairDeviceId* device_id, const Leds& leds)
{
	assert(s_color_set.load(std::memory_order_relaxed) == true);
	s_color_set.store(false, std::memory_order_relaxed);
	CHECKCORSAIR(CorsairSetLedColorsBuffer(*device_id, static_cast<int>(leds.getCount()), leds.getColorsBuffer()));
	CHECKCORSAIR(CorsairSetLedColorsFlushBufferAsync(onColorSet, nullptr));

}

void waitForColors()
{
	while (s_color_set.load(std::memory_order_relaxed) == false) {
		_mm_pause(); // designed for spinning on an atomic variable (like here)
	}
}