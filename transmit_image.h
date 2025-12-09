#pragma once

#include <filesystem>
#include <vector>

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void transmitImage(const CorsairDeviceId* device_id, Leds& leds, const std::filesystem::path& path);

void transmitText(const CorsairDeviceId* device_id, Leds& leds, std::vector<char> text);