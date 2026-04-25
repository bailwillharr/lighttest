#pragma once

#include <filesystem>
#include <vector>

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void crosstalkTransmit(const CorsairDeviceId* device_id, Leds& leds);