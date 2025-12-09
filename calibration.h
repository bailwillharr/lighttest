#pragma once

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void calibrationTransmit(const CorsairDeviceId* device_id, Leds& leds);

void calibrationTransmitForText(const CorsairDeviceId* device_id, Leds& leds);