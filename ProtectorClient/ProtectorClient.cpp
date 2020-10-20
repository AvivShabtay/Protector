#include "pch.h"
#include "../Protector/ProtectorCommon.h"
#include <vector>
#include <iostream>
#include <string>

// Global variables:
HANDLE g_hProtectorDevice;
bool g_DeviceCreated = false;

// Define prototypes:
int Error(const char* text);
bool ParseCommandLine(std::pair<std::wstring, std::wstring>& userInput, int argc, wchar_t** argv);
void ShowUsage(std::wstring appName);

int wmain(int argc, wchar_t* argv[]) {

	// Parse the user command line arguments:
	std::pair<std::wstring, std::wstring> input;
	if (!ParseCommandLine(input, argc, argv)) {
		return -1;
	}

	DWORD dwReturned;

	// Acquire device to Protector driver:
	g_hProtectorDevice = CreateFile(L"\\\\.\\Protector", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (g_hProtectorDevice == INVALID_HANDLE_VALUE) {
		return Error("Could not get Protector device");
	}

	// Indicates successful handle to the device:
	g_DeviceCreated = true;

	// Adding path to block:
	if (input.first == L"-a") {

		// Make sure the given path is legit,
		// otherwise display error to the user

		// Setup data:
		ProtectorPath protectorPath;
		::ZeroMemory(protectorPath.Path, MaxPath + 1);
		protectorPath.Type = RequestType::Add;
		wcscpy_s(protectorPath.Path, MaxPath + 1, input.second.c_str());

		// Request protection from executing programs from specific path (using DEVICE_CONTROL operation):
		if (!(::DeviceIoControl(g_hProtectorDevice, IOCTL_PROTECTOR_ADD_PATH, &protectorPath, sizeof(protectorPath), nullptr, 0, &dwReturned, nullptr))) {
			return Error("Could not commit device control request");
		}
	}

	// Remove blocked path:
	else if (input.first == L"-r") {
		// Make sure the given path is legit,
		// otherwise display error to the user

		// Setup data:
		ProtectorPath protectorPath;
		::ZeroMemory(protectorPath.Path, MaxPath + 1);
		protectorPath.Type = RequestType::Remove;
		wcscpy_s(protectorPath.Path, MaxPath + 1, input.second.c_str());

		// Request remove protection of specific path (using DEVICE_CONTROL operation):
		if (!(::DeviceIoControl(g_hProtectorDevice, IOCTL_PROTECTOR_REMOVE_PATH, &protectorPath, sizeof(protectorPath), nullptr, 0, &dwReturned, nullptr))) {
			return Error("Could not commit device control request");
		}
	}

	// Get all the user defined paths:
	else if (input.first == L"-p") {

		// Setup data:
		int length = 0;

		// Request the number of blocked path (using DEVICE_CONTROL operation):
		if (!(::DeviceIoControl(g_hProtectorDevice, IOCTL_PROTECTOR_GET_PATHS_LEN, nullptr, 0, &length, sizeof(int), &dwReturned, nullptr))) {
			return Error("Could not commit device control request");
		}

		if (length == 0) {
			printf("There are no blocked paths available yet\n");
			return -1;
		}

		printf("Number of blocked paths: %d\n", length);

		auto bufferSize = length * sizeof(ProtectorPath);

		// Buffer for incoming data:
		ProtectorPath* buffer = (ProtectorPath*)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, bufferSize);
		if (!buffer) {
			return Error("Could not allocate memory for incoming paths");
		}

		// Request blocked paths available (using DEVICE_CONTROL operation):
		if (!(::DeviceIoControl(g_hProtectorDevice, IOCTL_PROTECTOR_GET_PATHS, nullptr, 0, buffer, bufferSize, &dwReturned, nullptr))) {
			::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, buffer);
			return Error("Could not commit device control request");
		}

		for (ProtectorPath* path = buffer;
			path < buffer + length;
			path++)
		{
			printf("Blocked Path: %ws\n", path->Path);
		}

		::HeapFree(::GetProcessHeap(), HEAP_NO_SERIALIZE, buffer);
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

/*
* Helper function to parse the user command line to the application
* specific use cases.
*/
bool ParseCommandLine(std::pair<std::wstring, std::wstring>& input, int argc, wchar_t** argv) {
	if (argc <= 1) {
		ShowUsage(argv[0]);
		return false;
	}

	for (int i = 1; i < argc; i++) {
		std::wstring arg = argv[i];

		if ((arg == L"-h") || (arg == L"--help")) {
			ShowUsage(argv[0]);
			return false;
		}

		if ((arg == L"-a") || (arg == L"--append")) {
			if (i + 1 < argc) {
				input.first = arg;
				input.second = argv[i + 1];
				return true;
			}
			else {
				std::wcerr << L"--append option requires <path>." << std::endl;
			}
		}

		if ((arg == L"-r") || (arg == L"--remove")) {
			if (i + 1 < argc) {
				input.first = arg;
				input.second = argv[i + 1];
				return true;
			}
			else {
				std::wcerr << L"--remove option requires <path>." << std::endl;
			}
		}

		if ((arg == L"-p") || (arg == L"--paths")) {
			input.first = arg;
			return true;
		}

		if ((arg == L"-e") || (arg == L"--events")) {
			input.first = arg;
			return true;
		}
	}

	ShowUsage(argv[0]);
	return false;
}

/*
* Helper function to display the application options.
*/
void ShowUsage(std::wstring appName) {
	std::wcerr << "Usage: " << appName << " [option(s)]\n"
		<< "Options:\n"
		<< "\t-h,--help\t\tShow this help message\n"
		<< "\t-a,--append <PATH>\tSpecify path to protect from\n"
		<< "\t-r,--remove <PATH>\tSpecify path to remove from protection\n"
		<< "\t-p,--paths \t\tShow all the defined paths\n"
		<< "\t-e,--events \t\tShow event of blocked execution\n"
		<< std::endl;
}