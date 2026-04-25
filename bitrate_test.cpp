#include  "bitrate_test.h"

#include <cmath>

#include <chrono>
#include <numbers>
#include <thread>
#include <vector>
#include <array>
#include <random>

#include "fixed_update_loop.h"
#include "set_colors.h"
#include "my_print.h"
#include <unordered_map>

static std::vector<bool> getPRBS7(int num_bits, uint32_t seed)
{
	std::vector<bool> out{};

	std::mt19937 rng(seed);
	std::bernoulli_distribution bit;

	for (int i = 0; i < num_bits; ++i)
	{
		out.push_back(bit(rng));
	}

	return out;
}

static auto getKeysOrdered(const Leds& leds)
{
	// generate LUT to get keys in order
	std::array<std::vector<int>, 6> rows{};
	for (int i = 0; i < leds.getCount(); ++i) {
		auto pos = leds.getAllLedPositions()[i];
		if (pos.cy < 34.3) {
			// There are 112 LEDs, top 3 shouldn't be added
			continue;
		}
		if (pos.cy > 34.3 && pos.cy <= 38.6) {
			rows[0].push_back(i);
			continue;
		}
		if (pos.cy > 38.6 && pos.cy <= 59.5) {
			rows[1].push_back(i);
			continue;
		}
		if (pos.cy > 59.5 && pos.cy <= 78.5) {
			rows[2].push_back(i);
			continue;
		}
		if (pos.cy > 78.5 && pos.cy <= 97.6) {
			rows[3].push_back(i);
			continue;
		}
		if (pos.cy > 97.6 && pos.cy <= 116.1) {
			rows[4].push_back(i);
			continue;
		}
		if (pos.cy > 116.1 && pos.cy <= 135.6) {
			rows[5].push_back(i);
			continue;
		}
		throw std::runtime_error("Key didn't match?");
	}
	for (int i = 0; i < rows.size(); ++i) {
		std::sort(rows[i].begin(), rows[i].end(), [&](int first, int second) ->bool {
			// returns true if first is less than second
			double x1 = leds.getAllLedPositions()[first].cx;
			double x2 = leds.getAllLedPositions()[second].cx;
			return x1 < x2;
			});
	}
	std::vector<int> keys_ordered;
	for (const auto& row : rows) {
		for (int key_index : row) {
			keys_ordered.push_back(key_index);
		}
	}

	//keys_ordered.erase(keys_ordered.begin() + 16);
	//keys_ordered.erase(keys_ordered.begin() + 16);
	//keys_ordered.erase(keys_ordered.begin() + 16);
	//keys_ordered.erase(keys_ordered.begin() + 16);

	return keys_ordered;
}

void bitrateTest(const CorsairDeviceId* device_id, Leds& leds)
{
	constexpr int num_bits = 50; // per frequency test
	constexpr std::array FREQUENCIES{ 10, 15, 20, 25, 30 };

	// generate LUT to get keys in order
	std::array<std::vector<int>, 6> rows{};
	for (int i = 0; i < leds.getCount(); ++i) {
		auto pos = leds.getAllLedPositions()[i];
		if (pos.cy < 34.3) {
			// There are 112 LEDs, top 3 shouldn't be added
			continue;
		}
		if (pos.cy > 34.3 && pos.cy <= 38.6) {
			rows[0].push_back(i);
			continue;
		}
		if (pos.cy > 38.6 && pos.cy <= 59.5) {
			rows[1].push_back(i);
			continue;
		}
		if (pos.cy > 59.5 && pos.cy <= 78.5) {
			rows[2].push_back(i);
			continue;
		}
		if (pos.cy > 78.5 && pos.cy <= 97.6) {
			rows[3].push_back(i);
			continue;
		}
		if (pos.cy > 97.6 && pos.cy <= 116.1) {
			rows[4].push_back(i);
			continue;
		}
		if (pos.cy > 116.1 && pos.cy <= 135.6) {
			rows[5].push_back(i);
			continue;
		}
		throw std::runtime_error("Key didn't match?");
	}
	for (int i = 0; i < rows.size(); ++i) {
		std::sort(rows[i].begin(), rows[i].end(), [&](int first, int second) ->bool {
			// returns true if first is less than second
			double x1 = leds.getAllLedPositions()[first].cx;
			double x2 = leds.getAllLedPositions()[second].cx;
			return x1 < x2;
			});
	}
	std::vector<int> keys_ordered;
	for (const auto& row : rows) {
		for (int key_index : row) {
			keys_ordered.push_back(key_index);
		}
	}

	std::vector<std::vector<bool>> bits_for_keys{};
	for (int i = 0; i < keys_ordered.size(); ++i) {
		bits_for_keys.push_back(getPRBS7(num_bits, i));
	}

	myPrint("Showing green...");
	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	myPrint("Showing black...");
	leds.setAll(0, 0, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	for (const auto frequency : FREQUENCIES) {
		myPrint("Starting {} Hz...", frequency);
		leds.setAll(255, 0, 0);
		setColors(device_id, leds);
		waitForColors();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		startFixedUpdateLoop(num_bits, static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
			leds.setAll(0, 0, 0);
			for (int i = 0; i < keys_ordered.size(); ++i) {
				uint8_t val = bits_for_keys[i][iteration] ? 255 : 0;
				leds.setLed(keys_ordered[i], 0, val, 0);
			}
			waitForColors();
			setColors(device_id, leds);
			});
		waitForColors();
	}


	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void bitrateTestFreqSweep(const CorsairDeviceId* device_id, Leds& leds)
{
	constexpr int NUM_BITS = 100; // per frequency test
	constexpr std::array FREQUENCIES{ 10, 15, 20, 25, 30 };
	constexpr int SEED = 0;

	const auto bits = getPRBS7(NUM_BITS, SEED);

	myPrint("Showing green...");
	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	myPrint("Showing black...");
	leds.setAll(0, 0, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	for (const auto frequency : FREQUENCIES) {
		myPrint("Starting {} Hz...", frequency);
		leds.setAll(255, 0, 0);
		setColors(device_id, leds);
		waitForColors();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		startFixedUpdateLoop(NUM_BITS, static_cast<int64_t>(1'000'000.0 / frequency), [&](int iteration) {
			uint8_t val = bits[iteration] ? 255 : 0;
			leds.setAll(0, val, 0);
			waitForColors();
			setColors(device_id, leds);
			});
		waitForColors();
	}


	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}


static std::vector<std::vector<int>> divideLedsIntoCells(const Leds& leds, float cellRadius) {
	std::vector<std::vector<int>> cells;

	// Get the bounds of all LEDs
	const LedBounds bounds = leds.getBounds();
	float min_x = bounds.min_x_pos;
	float max_x = bounds.max_x_pos;
	float min_y = 34.3; // override to not include top keys
	float max_y = bounds.max_y_pos;

	// Compute width and height of the grid in units of cellRadius
	int num_cells_x = static_cast<int>(std::ceil((max_x - min_x) / cellRadius));
	int num_cells_y = static_cast<int>(std::ceil((max_y - min_y) / cellRadius));

	// Loop over the grid
	for (int y = 0; y < num_cells_y; ++y) {
		for (int x = 0; x < num_cells_x; ++x) {
			float left = min_x + x * cellRadius;
			float top = min_y + y * cellRadius;

			// Get all LEDs in this "cell rectangle"
			std::vector<uint32_t> ledIndices = leds.getLedsInBounds(left, top, cellRadius, cellRadius);

			// Only add cells that contain at least one LED
			if (!ledIndices.empty()) {
				// Convert to int for your return type
				std::vector<int> cell;
				cell.reserve(ledIndices.size());
				for (uint32_t idx : ledIndices) {
					cell.push_back(static_cast<int>(idx));
				}
				cells.push_back(std::move(cell));
			}
		}
	}

	return cells;
}

void bitrateTestCellSize(const CorsairDeviceId* device_id, Leds& leds)
{
	constexpr int NUM_BITS = 100; // per frequency test
	constexpr std::array CELL_SIZES{ 20, 40, 60, 80 };
	constexpr double FREQUENCY = 10.0;

	myPrint("Showing green...");
	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	myPrint("Showing black...");
	leds.setAll(0, 0, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	auto ordered = getKeysOrdered(leds);

	std::unordered_map<int, int> syskey_to_ordered_index;
	for (int i = 0; i < ordered.size(); ++i) {
		syskey_to_ordered_index.insert(std::make_pair(ordered[i], i));
	}

	for (const auto radius : CELL_SIZES) {
		myPrint("Cell radius {}...", radius);
		leds.setAll(255, 0, 0);
		setColors(device_id, leds);
		waitForColors();
		std::this_thread::sleep_for(std::chrono::seconds(1));

		auto cells = divideLedsIntoCells(leds, radius);
		myPrint("Num cells: {}", cells.size());

		for (const auto& cell : cells) {
			std::cout << "cells.emplace_back(std::vector<int>{";
			for (const int syskey : cell) {
				std::cout << syskey_to_ordered_index.at(syskey) << ",";
			}
			std::cout << "});\n";
		}

		std::vector<std::vector<bool>> bits_per_cell(cells.size());
		for (int i = 0; i < bits_per_cell.size(); ++i) {
			bits_per_cell[i] = getPRBS7(NUM_BITS, i);
		}

		continue;

		startFixedUpdateLoop(NUM_BITS, static_cast<int64_t>(1'000'000.0 / FREQUENCY), [&](int iteration) {
			leds.setAll(0, 0, 0);
			for (int cell_index = 0; cell_index < cells.size(); ++cell_index) {
				for (const auto led_index : cells[cell_index]) {
					uint8_t val = bits_per_cell[cell_index][iteration] ? 255 : 0;
					leds.setLed(led_index, 0, val, 0);
				}
			}
			waitForColors();
			setColors(device_id, leds);
			});
		waitForColors();
	}


	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void bitrateTestColors(const CorsairDeviceId* device_id, Leds& leds)
{
	constexpr int NUM_STATES_BITS = 4;
	constexpr int NUM_STATES = 1 << NUM_STATES_BITS;
	constexpr int ITERS = 100;
	constexpr int BITSTREAM_LENGTH = ITERS * NUM_STATES_BITS;
	constexpr double FREQUENCY = 10.0;

	const auto start_time = std::chrono::high_resolution_clock::now();

	myPrint("Showing colour...");
	leds.setAll(0, 255, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	myPrint("Showing black...");
	leds.setAll(0, 0, 0);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	myPrint("showing calibration sequence for {} states", NUM_STATES);
	leds.setAll(255, 255, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	startFixedUpdateLoop(NUM_STATES, static_cast<int64_t>(1'000'000.0 / FREQUENCY), [&](int iteration) {
		uint32_t value = iteration;
		// Scale to 0–255
		// max possible value for num_bits is (2^num_bits - 1)
		uint32_t max_value = NUM_STATES - 1;

		// Scale value to 0–255
		uint8_t scaled = static_cast<uint8_t>(std::round((value * 255.0) / max_value));
		leds.setAll(0, scaled, 0);
		waitForColors();
		setColors(device_id, leds);
		});
	waitForColors();

	myPrint("Done calibrating. Transmitting...");

	leds.setAll(255, 255, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	const auto bitstream = getPRBS7(BITSTREAM_LENGTH, 0);

	auto it = bitstream.begin();
	startFixedUpdateLoop(ITERS, static_cast<int64_t>(1'000'000.0 / FREQUENCY), [&](int iteration) {
		uint32_t value = 0;
		for (int i = 0; i < NUM_STATES_BITS; ++i) {
			value |= ((*it) ? 1 : 0);
			value <<= 1;
			++it;
		}

		// Scale to 0–255
		// max possible value for num_bits is (2^num_bits - 1)
		uint32_t max_value = NUM_STATES - 1;

		// Scale value to 0–255
		uint8_t scaled = static_cast<uint8_t>(std::round((value * 255.0) / max_value));
		leds.setAll(0, scaled, 0);
		waitForColors();
		setColors(device_id, leds);
		});
	waitForColors();

	const auto elapsed = std::chrono::high_resolution_clock::now() - start_time;

	myPrint("Took: {}", elapsed);


	leds.setAll(0, 0, 255);
	setColors(device_id, leds);
	waitForColors();
	std::this_thread::sleep_for(std::chrono::seconds(1));
}