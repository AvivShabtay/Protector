#include "pch.h"
#include "../Protector/ProtectorCommon.h"

// Global variables:
HANDLE g_hProtectorDevice;
bool g_DeviceCreated = false;

// Define prototypes:
int Error(const char* text);

int main() {

	WCHAR path[] = L"C:\\Test";
	//DWORD dwBytesRead;
	DWORD dwReturned;

	// Acquire device to Sysmon driver:
	g_hProtectorDevice = CreateFile(L"\\\\.\\Protector", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (g_hProtectorDevice == INVALID_HANDLE_VALUE) {
		return Error("Could not get Protector device");
	}
	g_DeviceCreated = true;


	// Request protection from executing programs from specific path (using WRITE operation):
	/*
	if (!(::WriteFile(g_hProtectorDevice, path, sizeof(path), &dwBytesRead, nullptr))) {
		return Error("Could not read data");
	}
	*/

	// Setup data:
	ProtectorPath protectorPath;
	protectorPath.Type = RequestType::Add;
	wcscpy_s(protectorPath.Path, sizeof(path), path);

	// Request protection from executing programs from specific path (using DEVICE_CONTROL operation):
	if (!(::DeviceIoControl(g_hProtectorDevice, IOCTL_PROTECTOR_SET_PATH, &protectorPath, sizeof(protectorPath), nullptr, 0, &dwReturned, nullptr))) {
		return Error("Could not commit device control request");
	}

	CloseHandle(g_hProtectorDevice);
}

/*
* Get error text, retrieve GetLastError value and prints it.
*/
int Error(const char* text) {
	printf("%s (%d)\n", text, ::GetLastError());

	// Close resources:
	if (g_DeviceCreated)
		CloseHandle(g_hProtectorDevice);

	return 1;
}