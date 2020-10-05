#include "pch.h"
#include "../Protector/ProtectorCommon.h"

// Global variables:
HANDLE g_hProtectorDevice;
bool g_DeviceCreated = false;

// Define prototypes:
int Error(const char* text);
bool ParseCommandLine(std::pair<std::wstring, std::wstring>& userInput, int argc, wchar_t** argv);
void ShowUsage(std::wstring appName);

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