#include "ProtectorClient.h"
#include "PeResource.h"
#include "ServiceManager.h"
#include "Win32ErrorCodeException.h"

#include <iostream>
#include <sstream>

#include <Psapi.h>
#include <Windows.h>

// Define helpers:
bool parseCommandLine(std::pair<std::wstring, std::wstring>& userInput, int argc, wchar_t** argv);
void showUsage(const std::wstring& appName);
std::wstring getTempDirectoryPath();
std::wstring createTemporaryPath(const std::wstring& exeNameWithExtension);

const int PROTECTOR_DRIVER_RESOURCE_ID = 101;
const std::wstring RESOURCE_NAME(L"PROTECTOR_DRIVER");
const std::wstring DRIVER_NAME_WITH_EXTENSION(L"Protector.sys");
const std::wstring SERVICE_NAME(L"Protector");
const std::uint32_t SERVICE_TYPE = SERVICE_KERNEL_DRIVER;

int wmain(int argc, wchar_t* argv[]) {

	// Parse the user command line arguments:
	std::pair<std::wstring, std::wstring> input;
	if (!parseCommandLine(input, argc, argv))
	{
		return 1;
	}

	try
	{
		// Loading Protector driver:
		if (input.first == L"-i")
		{
			const PeResource resource(PROTECTOR_DRIVER_RESOURCE_ID, RESOURCE_NAME);

			const std::wstring binPath(createTemporaryPath(DRIVER_NAME_WITH_EXTENSION));
			resource.saveResourceToFileSystem(binPath);
			std::wcout << "[+] Dump Protector.sys to: " << binPath << std::endl;

			ServiceManager serviceManager(SERVICE_NAME, binPath, SERVICE_TYPE);
			serviceManager.installAndStart();
			std::wcout << "[+] Install Protector driver successfully" << std::endl;
		}

		// Unloading Protector driver:
		else if (input.first == L"-s")
		{
			const std::wstring binPath(createTemporaryPath(DRIVER_NAME_WITH_EXTENSION));

			ServiceManager serviceManager(SERVICE_NAME, binPath, SERVICE_TYPE);
			serviceManager.stopAndRemove();
			std::wcout << "[+] Unloading Protector driver successfully" << std::endl;

			if (!DeleteFile(binPath.c_str()))
			{
				throw Win32ErrorCodeException("Could not delete Protector.sys file");
			}
			std::wcout << "[+] Remove Protector driver successfully" << std::endl;
		}

		// Adding path to block:
		else if (input.first == L"-a")
		{
			// TODO: Make sure the given path is legit, otherwise display error to the user

			const ProtectorClient protectorClient;
			protectorClient.addPath(input.second);
			std::wcout << "[+] Add protection against executing programs from: " << input.second << std::endl;
		}

		// Remove blocked path:
		else if (input.first == L"-r")
		{
			// TODO: Make sure the given path is legit, otherwise display error to the user

			const ProtectorClient protectorClient;
			protectorClient.removePath(input.second);
			std::wcout << "[+] Remove protection against executing programs from: " << input.second << std::endl;
		}

		// Get all the user defined paths:
		else if (input.first == L"-p")
		{
			const ProtectorClient protectorClient;
			std::vector<std::wstring> paths = protectorClient.getAllPaths();
			for (auto& path : paths)
			{
				std::wcout << "[Path] :" << path << std::endl;
			}
		}
	}
	catch (std::exception& exception)
	{
		std::cout << exception.what() << std::endl;
		return 1;
	}

	return 0;
}

/*
* Helper function to parse the user command line to the application
* specific use cases.
*/
bool parseCommandLine(std::pair<std::wstring, std::wstring>& input, int argc, wchar_t** argv) {
	if (argc <= 1) {
		showUsage(argv[0]);
		return false;
	}

	for (int i = 1; i < argc; i++) {
		std::wstring arg = argv[i];

		if ((arg == L"-h") || (arg == L"--help")) {
			showUsage(argv[0]);
			return false;
		}

		if ((arg == L"-i") || (arg == L"--install")) {
			input.first = arg;
			return true;
		}

		if ((arg == L"-s") || (arg == L"--stop")) {
			input.first = arg;
			return true;
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

	showUsage(argv[0]);
	return false;
}

/*
* Helper function to display the application options.
*/
void showUsage(const std::wstring& appName) {
	std::wcerr << "Usage: " << appName << " [option(s)]\n"
		<< "Options:\n"
		<< "\t-h,--help\t\tShow this help message\n"
		<< "\t-i,--install\t\tDump Protector.sys and load it\n"
		<< "\t-s,--stop\t\tStop Protector driver, unload it and cleanup the executable\n"
		<< "\t-a,--append <PATH>\tSpecify path to protect from\n"
		<< "\t-r,--remove <PATH>\tSpecify path to remove from protection\n"
		<< "\t-p,--paths \t\tShow all the defined paths\n"
		<< "\t-e,--events \t\tShow event of blocked execution\n"
		<< std::endl;
}

std::wstring getTempDirectoryPath()
{
	std::vector<WCHAR> tempDirectoryPath(MAX_PATH);
	if (!GetTempPath(MAX_PATH, tempDirectoryPath.data()))
	{
		throw Win32ErrorCodeException("Could not get the temp directory path");
	}

	return tempDirectoryPath.data();
}

std::wstring createTemporaryPath(const std::wstring& exeNameWithExtension)
{
	std::wstringstream binPathStream;
	binPathStream << getTempDirectoryPath();
	binPathStream << "\\";
	binPathStream << exeNameWithExtension;

	return binPathStream.str();
}