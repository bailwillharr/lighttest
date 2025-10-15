#include "corsair_helpers.h"

#include "static_vector.h"
#include "my_print.h"

const char* corsairErrToString(CorsairError err)
{
	switch (err)
	{
	case CE_Success:
		return "CE_Success";
	case CE_NotConnected:
		return "CE_NotConnected";
	case CE_NoControl:
		return "CE_NoControl";
	case CE_IncompatibleProtocol:
		return "CE_IncompatibleProtocol";
	case CE_InvalidArguments:
		return "CE_InvalidArguments";
	case CE_InvalidOperation:
		return "CE_InvalidOperation";
	case CE_DeviceNotFound:
		return "CE_DeviceNotFound";
	case CE_NotAllowed:
		return "CE_NotAllowed";
	default:
		return "unknown error";
	}
}

const char* corsairSessionStateToString(CorsairSessionState session_state)
{
	switch (session_state) {
	case CSS_Invalid:
		return "CSS_Invalid";
	case CSS_Closed:
		return "CSS_Closed";
	case CSS_Connecting:
		return "CSS_Connecting";
	case CSS_Timeout:
		return "CSS_Timeout";
	case CSS_ConnectionRefused:
		return "CSS_ConnectionRefused";
	case CSS_ConnectionLost:
		return "CSS_ConnectionLost";
	case CSS_Connected:
		return "CSS_Connected";
	default:
		return "unknown session state";
	}
}

void corsairCheckError(CorsairError err, const char* function_name)
{
	if (err != CE_Success) {
		die("iCUESDK function {} returned {}", function_name, err);
	}
}

const CorsairDeviceId* findKeyboard()
{
	static static_vector<CorsairDeviceInfo, CORSAIR_DEVICE_COUNT_MAX> s_found_devices;

	const CorsairDeviceId* device_id{};
	const CorsairDeviceFilter filter{ CDT_Keyboard };
	int num_devices{};
	CHECKCORSAIR(CorsairGetDevices(&filter, static_cast<int>(s_found_devices.capacity()), s_found_devices.data(), &num_devices));
	myPrint("Found {} devices:", num_devices);
	s_found_devices.resize_uninitialized(static_cast<uint32_t>(num_devices));
	for (int i = 0; i < num_devices; ++i) {
		const auto& dev = s_found_devices[i];
		myPrint("[{}]:", i);
		myPrint("    model: {}", dev.model);
		myPrint("    serial: {}", dev.serial);
		myPrint("    id: {}", dev.id);
		myPrint("    LED count: {}", dev.ledCount);
		myPrint("    channel count: {}", dev.channelCount);
		if (i == 0) {
			device_id = &dev.id;
		}
	}
	return device_id;
}