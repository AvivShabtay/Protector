#include "pch.h"
#include "../Protector/ProtectorCommon.h"

// Define prototypes:
int Error(const char* text);

int main() {

	WCHAR path[] = L"C:\\Temp";
	DWORD dwBytesRead;

	// Acquire device to Sysmon driver:
	HANDLE hProtectorDevice = CreateFile(L"\\\\.\\Protector", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hProtectorDevice == INVALID_HANDLE_VALUE) {
		return Error("Could not get Protector device");
	}

	// Request to read data from sysmon driver:
	if (!(::WriteFile(hProtectorDevice, path, sizeof(path), &dwBytesRead, nullptr))) {
		return Error("Could not read data");
	}

	CloseHandle(hProtectorDevice);
}

/*
* Get error text, retrieve GetLastError value and prints it.
*/
int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());
	return 1;
}