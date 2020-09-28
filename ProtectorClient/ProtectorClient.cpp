#include "pch.h"
#include "../Protector/ProtectorCommon.h"

// Define prototypes:
int Error(const char* text);

int main() {

	// Acquire device to Sysmon driver:
	HANDLE hProtectorDevice = CreateFile(L"\\\\.\\Protector", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hProtectorDevice == INVALID_HANDLE_VALUE) {
		return Error("Could not get Protector device");
	}

	// Request to read data from sysmon driver:
	if (!(::ReadFile(hSysmonDevice, buffer, sizeof(buffer), &dwBytesRead, nullptr))) {
		return Error("Could not read data");
	}
}

/*
* Get error text, retrieve GetLastError value and prints it.
*/
int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}