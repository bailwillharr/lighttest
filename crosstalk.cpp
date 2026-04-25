#include  "crosstalk.h"

#include <cmath>

#include <chrono>
#include <numbers>
#include <thread>
#include <vector>
#include <array>
#include <random>
#include <map>

#include <conio.h>

#include "fixed_update_loop.h"
#include "set_colors.h"
#include "my_print.h"

static auto getOrdered105(const Leds& leds)
{
	// generate LUT to get keys in order
	std::map<int, std::vector<int>> rows{};
	for (int i = 0; i < leds.getCount(); ++i) {
		auto pos = leds.getAllLedPositions()[i];
		if (pos.cy < 34.3) {
			// There are 112 LEDs, top 3 shouldn't be added
			continue;
		}
		if (pos.cy > 34.3 && pos.cy <= 38.6) {
			rows[0].push_back(i);
		}
		if (pos.cy > 38.6 && pos.cy <= 59.5) {
			rows[1].push_back(i);
		}
		if (pos.cy > 59.5 && pos.cy <= 78.5) {
			rows[2].push_back(i);
		}
		if (pos.cy > 78.5 && pos.cy <= 97.6) {
			rows[3].push_back(i);
		}
		if (pos.cy > 97.6 && pos.cy <= 116.1) {
			rows[4].push_back(i);
		}
		if (pos.cy > 116.1 && pos.cy <= 135.6) {
			rows[5].push_back(i);
		}
	}

	for (auto& [i, row] : rows) {
		std::sort(row.begin(), row.end(), [&](int first, int second) ->bool {
			// returns true if first is less than second
			double x1 = leds.getAllLedPositions()[first].cx;
			double x2 = leds.getAllLedPositions()[second].cx;
			return x1 < x2;
			});
	}

	// remove multimedia keys
	rows[0].pop_back();
	rows[0].pop_back();
	rows[0].pop_back();
	rows[0].pop_back();

	std::vector<int> ordered{};

	for (const auto& [i, row] : rows) {
		for (int i : row) {
			ordered.push_back(i);
		}
	}

	assert(ordered.size() == 105);

	return ordered;
}

// inputs should be between 0 and N inclusive
static std::array<uint8_t, 3> colorTransformSquare(int r, int g, int b) {
	constexpr int N = 15;
	double R = (static_cast<double>(r) / static_cast<double>(N));
	R = R * R;
	double G = (static_cast<double>(g) / static_cast<double>(N));
	G = G * G;;
	double B = (static_cast<double>(b) / static_cast<double>(N));
	B = B * B;
	return { static_cast<uint8_t>(R * 255.0), static_cast<uint8_t>(G * 255.0), static_cast<uint8_t>(B * 255.0) };
}
// inputs should be between 0 and N inclusive
static std::array<uint8_t, 3> colorTransformCube(int r, int g, int b) {
	constexpr int N = 15;
	double R = (static_cast<double>(r) / static_cast<double>(N));
	R = R * R * R;
	double G = (static_cast<double>(g) / static_cast<double>(N));
	G = G * G * G;
	double B = (static_cast<double>(b) / static_cast<double>(N));
	B = B * B * B;
	return { static_cast<uint8_t>(R * 255.0), static_cast<uint8_t>(G * 255.0), static_cast<uint8_t>(B * 255.0) };
}

// inputs should be between 0 and N inclusive
static std::array<uint8_t, 3> colorTransformLinear(int r, int g, int b) {
	constexpr int N = 15;
	double R = (static_cast<double>(r) / static_cast<double>(N));
	double G = (static_cast<double>(g) / static_cast<double>(N));
	double B = (static_cast<double>(b) / static_cast<double>(N));
	return { static_cast<uint8_t>(R * 255.0), static_cast<uint8_t>(G * 255.0), static_cast<uint8_t>(B * 255.0) };
}

void crosstalkTransmit(const CorsairDeviceId* device_id, Leds& leds)
{
	constexpr double frequency = 1.0;

	auto keys_ordered = getOrdered105(leds);

	_getch();

	startFixedUpdateLoop(128, static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
		leds.setAll(0, 0, iteration * 2);
		myPrint("{}/128", iteration);
		waitForColors();
		setColors(device_id, leds);
		});
	waitForColors();

	myPrint("DONE! (waiting 1 sec)");
	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}