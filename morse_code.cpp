#include "morse_code.h"

#include <chrono>
#include <thread>
#include <span>
#include <string_view>
#include <vector>

#include <iCUESDK/iCUESDK.h>

#include "my_print.h"
#include "fixed_update_loop.h"
#include "set_colors.h"

void transmitMorseCode(const CorsairDeviceId* device_id, Leds& leds, std::string_view text)
{
	std::vector<bool> message{};

	// save typing
	auto dot = [&]() {
		message.push_back(true);
		message.push_back(false);
		};
	auto dash = [&]() {
		message.push_back(true);
		message.push_back(true);
		message.push_back(true);
		message.push_back(false);
		};

	for (char c : text)
	{
		switch (toupper(c)) {
        case 'A':
            dot(); dash();
            break;
        case 'B':
            dash(); dot(); dot(); dot();
            break;
        case 'C':
            dash(); dot(); dash(); dot();
            break;
        case 'D':
            dash(); dot(); dot();
            break;
        case 'E':
            dot();
            break;
        case 'F':
            dot(); dot(); dash(); dot();
            break;
        case 'G':
            dash(); dash(); dot();
            break;
        case 'H':
            dot(); dot(); dot(); dot();
            break;
        case 'I':
            dot(); dot();
            break;
        case 'J':
            dot(); dash(); dash(); dash();
            break;
        case 'K':
            dash(); dot(); dash();
            break;
        case 'L':
            dot(); dash(); dot(); dot();
            break;
        case 'M':
            dash(); dash();
            break;
        case 'N':
            dash(); dot();
            break;
        case 'O':
            dash(); dash(); dash();
            break;
        case 'P':
            dot(); dash(); dash(); dot();
            break;
        case 'Q':
            dash(); dash(); dot(); dash();
            break;
        case 'R':
            dot(); dash(); dot();
            break;
        case 'S':
            dot(); dot(); dot();
            break;
        case 'T':
            dash();
            break;
        case 'U':
            dot(); dot(); dash();
            break;
        case 'V':
            dot(); dot(); dot(); dash();
            break;
        case 'W':
            dot(); dash(); dash();
            break;
        case 'X':
            dash(); dot(); dot(); dash();
            break;
        case 'Y':
            dash(); dot(); dash(); dash();
            break;
        case 'Z':
            dash(); dash(); dot(); dot();
            break;
        case '0':
            dash(); dash(); dash(); dash(); dash();
            break;
        case '1':
            dot(); dash(); dash(); dash(); dash();
            break;
        case '2':
            dot(); dot(); dash(); dash(); dash();
            break;
        case '3':
            dot(); dot(); dot(); dash(); dash();
            break;
        case '4':
            dot(); dot(); dot(); dot(); dash();
            break;
        case '5':
            dot(); dot(); dot(); dot(); dot();
            break;
        case '6':
            dash(); dot(); dot(); dot(); dot();
            break;
        case '7':
            dash(); dash(); dot(); dot(); dot();
            break;
        case '8':
            dash(); dash(); dash(); dot(); dot();
            break;
        case '9':
            dash(); dash(); dash(); dash(); dot();
            break;
		case ' ':
			message.push_back(false);
			message.push_back(false);
			message.push_back(false);
			message.push_back(false);
			// 4 here + 2 for end of character + last char ending OFF unit = 7
			break;
		default:
			myPrint("Unknown char: {}", c);
		}
		message.push_back(false);
		message.push_back(false);
	}

	// 20 Hz is the absolute maximum possible frequency. Any higher and some LED sets are skipped
	// Even then, at 20 Hz the timing isn't super consistent, some sets appear on time, some early, some late.
	// But such inconsistencies can still be resolved with proper sampling.
	// It's likely that the iCUE SDK internally updates the keyboard at 20Hz
	constexpr double FREQUENCY = 20.0; // cycles per second
	auto iters = static_cast<int>(message.size());

	myPrint("Transmitting \"{}\" in morse code at {} Hz for {} sec", text, FREQUENCY, static_cast<double>(iters) / FREQUENCY);

	std::this_thread::sleep_for(std::chrono::seconds(1));

	auto start = std::chrono::high_resolution_clock::now();
	startFixedUpdateLoop(iters, static_cast<int64_t>(1'000'000.0 / FREQUENCY), [&](int iteration) {
		// This code runs 20 times per second
		if (message[iteration] == true) {
			leds.setAll(0, 255, 0);
		}
		else {
			leds.setAll(255, 0, 0);
		}
		waitForColors();
		setColors(device_id, leds);
		});
	waitForColors();

	auto end = std::chrono::high_resolution_clock::now();

	myPrint("Took: {} ms", std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1'000'000LL);
}