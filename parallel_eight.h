#pragma once

#include <span>

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void transmitParallelEight(const CorsairDeviceId* device_id, Leds& leds, std::span<const uint8_t> data);