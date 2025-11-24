#pragma once

#include <filesystem>

#include <iCUESDK/iCUESDK.h>

#include "leds.h"

void transmitImage(const CorsairDeviceId* device_id, Leds& leds, const std::filesystem::path& path, int resolution);