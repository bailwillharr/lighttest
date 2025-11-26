#include <cassert>

#include <array>
#include <condition_variable>
#include <format>
#include <iostream>
#include <mutex>
#include <span>

#include <Windows.h>
#include <Lmcons.h>

#include <iCUESDK/iCUESDK.h>

#include "corsair_helpers.h"
#include "leds.h"
#include "morse_code.h"
#include "parallel_eight.h"
#include "my_print.h"
#include "static_vector.h"
#include "graph.h"
#include "calibration.h"
#include "sampling_test.h"
#include "transmit_image.h"
#include "fixed_update_loop.h"
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

	static Leds leds(device_id);

	/////////
	// RUN //
	/////////

	//transmitMorseCode(device_id, leds, user_name_ascii);
	//transmitMorseCode(device_id, leds, "YOU GOT HACKED");
	//transmitParallelEight(device_id, leds, {});
	//liveGraph(device_id, leds);
	//idealTransmit(device_id, leds);
	//samplingTest(device_id, leds, 10.0, 1000);
	transmitImage(device_id, leds, std::filesystem::path(PROJECT_DIR) / "images" / "mandrill.png");
	//calibrationTransmit(device_id, leds);

#if 0
	for (int i = 0; i < leds.getCount(); ++i) {
		auto pos = leds.getAllLedPositions()[i];
		myPrint("index: {}, x: {}, y; {}", i, pos.cx, pos.cy);
		waitForColors();
		leds.setAll(0, 0, 0);
		leds.setLed(i, 255, 0, 0);
		setColors(device_id, leds);
		std::string dummy;
		std::getline(std::cin, dummy);   // waits for Enter
	}
#endif

	/////////////
	// CLEANUP //
	/////////////

	CHECKCORSAIR(CorsairReleaseControl(*device_id));
	CHECKCORSAIR(CorsairDisconnect());
}