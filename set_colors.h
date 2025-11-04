#pragma once

#include <span>

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void setColors(const CorsairDeviceId* device_id, const Leds& leds);

void waitForColors();