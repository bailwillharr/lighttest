#pragma once

#include <span>

#include <iCUESDK/iCUESDK.h>

void setColors(const CorsairDeviceId* device_id, std::span<const CorsairLedColor> led_colors);

void waitForColors();