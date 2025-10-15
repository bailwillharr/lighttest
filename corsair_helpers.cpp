#include "corsair_helpers.h"

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
