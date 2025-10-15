#include <cassert>

#include <span>
#include <iostream>
#include <format>
#include <condition_variable>
#include <mutex>

#include <iCUESDK/iCUESDK.h>

#include "corsair_helpers.h"
#include "my_print.h"
#include "static_vector.h"
#include "fixed_update_loop.h"
#include "leds.h"
#include "set_colors.h"

struct StateChangedContext {
	std::mutex mutex{}; // accessed by main thread and onStateChanged thread
	std::condition_variable cv{}; // for waking up main thread once connected
	bool connected = false;
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

	const CorsairDeviceId* device_id = findKeyboard();
	if (!device_id) {
		die("Couldn't find a Corsair keyboard!");
	}

	// Request exclusive control over the keyboard's lighting and key events
	CHECKCORSAIR(CorsairRequestControl(*device_id, CAL_ExclusiveLightingControlAndKeyEventsListening));

	static const Leds leds(device_id);

	/////////
	// RUN //
	/////////

	static_vector<CorsairLedColor, CORSAIR_DEVICE_LEDCOUNT_MAX> led_colors(leds.getCount(), CorsairLedColor{ .r = 0, .g = 0, .b = 0, .a = 255 });

	auto start = std::chrono::high_resolution_clock::now();

	// 20 Hz is the absolute maximum possible frequency. Any higher and some LED sets are skipped
	// Even then, at 20 Hz the timing isn't super consistent, some sets appear on time, some early, some late.
	// But such inconsitencies can still be resolved with proper sampling.
	// It's likely that the iCUE SDK internally updates the keyboard at 20Hz
	constexpr double FREQUENCY = 20.0;
	constexpr int ITERS = 100;
	myPrint("Flashing at {} Hz for {} sec", FREQUENCY, static_cast<double>(ITERS) / FREQUENCY);
	startFixedUpdateLoop(ITERS, static_cast<int64_t>(1'000'000.0 / FREQUENCY), [&](int iteration) {
		// This code runs 20 times per second
		uint8_t color[3]{};
		switch (iteration % 4) {
		case 0:
			color[0] = 255;
			color[1] = 0;
			color[2] = 0;
			break;
		case 1:
			color[0] = 0;
			color[1] = 255;
			color[2] = 0;
			break;
		case 2:
			color[0] = 0;
			color[1] = 0;
			color[2] = 255;
			break;
		case 3:
			color[0] = 0;
			color[1] = 0;
			color[2] = 0;
		}
		for (uint32_t i = 0; i < led_colors.size(); ++i) {
			led_colors[i].id = leds.getLed(i);
			if ((i + iteration) % 2 == 0) {
				led_colors[i].r = color[0];
				led_colors[i].g = color[1];
				led_colors[i].b = color[2];
				led_colors[i].a = 255;
			}
			else {
				led_colors[i].r = 255;
				led_colors[i].g = 255;
				led_colors[i].b = 255;
				led_colors[i].a = 255;
			}
		}
		waitForColors();
		setColors(device_id, led_colors);
		});

	auto end = std::chrono::high_resolution_clock::now();

	myPrint("Took: {} ms", std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1'000'000LL);

	/////////////
	// CLEANUP //
	/////////////

	CHECKCORSAIR(CorsairReleaseControl(nullptr));
	CHECKCORSAIR(CorsairDisconnect());
}