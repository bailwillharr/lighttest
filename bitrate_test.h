#pragma once

#include <filesystem>
#include <vector>

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void bitrateTest(const CorsairDeviceId* device_id, Leds& leds);

void bitrateTestFreqSweep(const CorsairDeviceId* device_id, Leds& leds);

void bitrateTestCellSize(const CorsairDeviceId* device_id, Leds& leds);

void bitrateTestColors(const CorsairDeviceId* device_id, Leds& leds);