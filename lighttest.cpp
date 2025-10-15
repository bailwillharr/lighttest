#include <cassert>

#include <span>
#include <iostream>
#include <format>
#include <condition_variable>
#include <mutex>

#include <Windows.h>

#include <iCUESDK/iCUESDK.h>

#include "corsair_helpers.h"
#include "my_print.h"
#include "static_vector.h"

struct StateChangedContext {
	std::mutex mutex{}; // accessed by main thread and onStateChanged thread
	std::condition_variable cv{}; // for waking up main thread once connected
	bool connected = false;
};

struct ColorSetContext {
	std::mutex mutex{}; // accessed by main thread and onColorSet thread
	std::condition_variable cv{}; // for waking up main thread once color is changed
	bool color_changed = false;
};

// Runs on a separate thread
static void onStateChanged(void* context, const CorsairSessionStateChanged* event_data)
{
	assert(context);
	assert(event_data);

	StateChangedContext& state_changed_context = *reinterpret_cast<StateChangedContext*>(context);

	switch (event_data->state) {
	case CSS_Invalid:
		assert(false);
		break;
	case CSS_Closed:
		// not connected yet or just disconnected
		break;
	case CSS_Connecting:
		myPrint("Connecting...");
		break;
	case CSS_Timeout:
		myPrint("Connection timeout. Retrying. Is iCUE running?");
		break;
	case CSS_ConnectionRefused:
		die("Server did not allow connection");
		break;
	case CSS_ConnectionLost:
		die("Server closed connection");
		break;
	case CSS_Connected: {
		myPrint("Connected!");
		myPrint("iCUE version: {}", event_data->details.serverHostVersion);
		myPrint("SDK version: {}", event_data->details.serverVersion);
		{
			std::lock_guard lock(state_changed_context.mutex);
			state_changed_context.connected = true;
		}
		state_changed_context.cv.notify_one(); // wait up main thread
	} break;
	default:
		assert(false);
		break;
	}
}

static void onColorSet(void* context, CorsairError error)
{
	CHECKCORSAIR(error);
	ColorSetContext& color_set_context = *reinterpret_cast<ColorSetContext*>(context);
	{
		std::lock_guard lock(color_set_context.mutex);
		color_set_context.color_changed = true;
	}
	color_set_context.cv.notify_one(); // wait up main thread
}

static void setColor(const CorsairDeviceId* device_id, ColorSetContext& color_set_context, CorsairLedLuid led, unsigned char r, unsigned char g, unsigned char b)
{
	CorsairLedColor led_color{};
	led_color.id = led;
	led_color.r = r;
	led_color.g = g;
	led_color.b = b;
	led_color.a = 255;
	CHECKCORSAIR(CorsairSetLedColorsBuffer(*device_id, 1, &led_color));
	{
		std::lock_guard lock(color_set_context.mutex);
		color_set_context.color_changed = false;
	}
	CHECKCORSAIR(CorsairSetLedColorsFlushBufferAsync(onColorSet, reinterpret_cast<void*>(&color_set_context)));
	{
		std::unique_lock lock(color_set_context.mutex);
		color_set_context.cv.wait(lock, [&]() {return color_set_context.color_changed; });
	}
}

static void setColors(const CorsairDeviceId* device_id, ColorSetContext& color_set_context, std::span<const CorsairLedColor> leds)
{
	CHECKCORSAIR(CorsairSetLedColorsBuffer(*device_id, static_cast<int>(leds.size()), leds.data()));
	{
		std::lock_guard lock(color_set_context.mutex);
		color_set_context.color_changed = false;
	}
	CHECKCORSAIR(CorsairSetLedColorsFlushBufferAsync(onColorSet, reinterpret_cast<void*>(&color_set_context)));
	{
		std::unique_lock lock(color_set_context.mutex);
		color_set_context.cv.wait(lock, [&]() {return color_set_context.color_changed; });
	}
}

// call waitForColors() after
static void setColorsNoWait(const CorsairDeviceId* device_id, ColorSetContext& color_set_context, std::span<const CorsairLedColor> leds)
{
	CHECKCORSAIR(CorsairSetLedColorsBuffer(*device_id, static_cast<int>(leds.size()), leds.data()));
	{
		std::lock_guard lock(color_set_context.mutex);
		color_set_context.color_changed = false;
	}
	CHECKCORSAIR(CorsairSetLedColorsFlushBufferAsync(onColorSet, reinterpret_cast<void*>(&color_set_context)));
}

static void waitForColors(ColorSetContext& color_set_context)
{
	std::unique_lock lock(color_set_context.mutex);
	color_set_context.cv.wait(lock, [&]() {return color_set_context.color_changed; });
}

static CorsairLedLuid ledFromLetter(const CorsairDeviceId* device_id, char key_name)
{
	assert(key_name >= 'A' && key_name <= 'Z');
	CorsairLedLuid luid{};
	CHECKCORSAIR(CorsairGetLedLuidForKeyName(*device_id, key_name, &luid));
	return luid;
}

static const CorsairDeviceId* findKeyboard()
{
	static static_vector<CorsairDeviceInfo, CORSAIR_DEVICE_COUNT_MAX> s_found_devices;

	const CorsairDeviceId* device_id{};
	const CorsairDeviceFilter filter{ CDT_Keyboard };
	int num_devices{};
	CHECKCORSAIR(CorsairGetDevices(&filter, static_cast<int>(s_found_devices.capacity()), s_found_devices.data(), &num_devices));
	myPrint("Found {} devices:", num_devices);
	s_found_devices.resize_uninitialized(static_cast<uint32_t>(num_devices));
	for (int i = 0; i < num_devices; ++i) {
		const auto& dev = s_found_devices[i];
		myPrint("[{}]:", i);
		myPrint("    model: {}", dev.model);
		myPrint("    serial: {}", dev.serial);
		myPrint("    id: {}", dev.id);
		myPrint("    LED count: {}", dev.ledCount);
		myPrint("    channel count: {}", dev.channelCount);
		if (i == 0) {
			device_id = &dev.id;
		}
	}
	if (!device_id) {
		die("Couldn't find a corsair keyboard");
	}
	return device_id;
}

static void waitTil(LARGE_INTEGER until)
{
	LARGE_INTEGER now{};
	do {
		QueryPerformanceCounter(&now);
	} while (now.QuadPart < until.QuadPart);
}

static LARGE_INTEGER addMicroseconds(LARGE_INTEGER timeline, int64_t microseconds)
{
	static int64_t s_counts_per_microsecond{};
	if (!s_counts_per_microsecond) {
		LARGE_INTEGER freq{};
		QueryPerformanceFrequency(&freq);
		assert(freq.QuadPart > 1'000'000LL);
		s_counts_per_microsecond = freq.QuadPart / 1'000'000LL;
	}
	return LARGE_INTEGER{ .QuadPart = timeline.QuadPart + (microseconds * s_counts_per_microsecond) };
}

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

int main()
{

	/////////////////////
	// INITIALISATIOON //
	/////////////////////

	StateChangedContext state_changed_context{}; // must be valid until CorsairDisconnect()
	{
		CHECKCORSAIR(CorsairConnect(onStateChanged, reinterpret_cast<void*>(&state_changed_context)));
		{
			std::unique_lock lock(state_changed_context.mutex);
			state_changed_context.cv.wait(lock, [&]() {return state_changed_context.connected; });
		}
	}

	// Find keyboard device ID
	const CorsairDeviceId* device_id = findKeyboard();

	// Request exclusive control over the keyboard's lighting and key events
	CHECKCORSAIR(CorsairRequestControl(*device_id, CAL_ExclusiveLightingControlAndKeyEventsListening));

	static const Leds leds(device_id);

	/////////
	// RUN //
	/////////

	static_vector<CorsairLedColor, CORSAIR_DEVICE_LEDCOUNT_MAX> leds_on(leds.getCount(), CorsairLedColor{ .r = 0, .g = 0, .b = 0, .a = 255 });
	//for (uint32_t i = 0; i < leds.getCount(); ++i) {
	//	leds_on[i].id = leds.getLed(i);
	//	leds_on[i].r = rand() % 255;
	//	leds_on[i].g = rand() % 255;
	//	leds_on[i].b = rand() % 255;
	//}
	ColorSetContext color_set_context{};

	// 20 Hz is the absolute maximum possible frequency. Any higher and some LED sets are skipped
	// Even then, at 20 Hz the timing isn't super consistent, some sets appear on time, some early, some late.
	// But such inconsitencies can still be resolved with proper sampling.
	constexpr double FREQUENCY = 10.0f;
	const int iters = 100;
	
	myPrint("Flashing at {} Hz for {} sec", FREQUENCY, static_cast<double>(iters) / FREQUENCY);

	//std::this_thread::sleep_for(std::chrono::milliseconds(250));

	auto start = std::chrono::high_resolution_clock::now();

	LARGE_INTEGER timeline{};
	QueryPerformanceCounter(&timeline);
	bool on = true;
	for (int j = 0; j < iters; ++j) {

		setColorsNoWait(device_id, color_set_context, leds_on);
		// color setting happens asynchronously
		
		for (uint32_t i = 0; i < leds.getCount(); ++i) {
			leds_on[i].id = leds.getLed(i);
			leds_on[i].g = on ? 255 : 0;
		}
		on = !on;

		timeline = addMicroseconds(timeline, 1'000'000LL / static_cast<int64_t>(FREQUENCY));
		waitTil(timeline);

		// in a sense this 'joins' the setColorsNoWait() thread
		waitForColors(color_set_context);

	}
	auto end = std::chrono::high_resolution_clock::now();

	myPrint("Took: {} ms", std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1'000'000LL);

	/////////////
	// CLEANUP //
	/////////////

	CHECKCORSAIR(CorsairReleaseControl(nullptr));
	CHECKCORSAIR(CorsairDisconnect());
}