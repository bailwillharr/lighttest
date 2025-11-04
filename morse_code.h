#pragma once

#include <string_view>

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void transmitMorseCode(const CorsairDeviceId* device_id, Leds& leds, std::string_view text);