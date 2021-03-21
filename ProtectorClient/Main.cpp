#include "ProtectorClient.h"

#include <iostream>

// Define helpers:
bool parseCommandLine(std::pair<std::wstring, std::wstring>& userInput, int argc, wchar_t** argv);
void showUsage(const std::wstring& appName);

int wmain(int argc, wchar_t* argv[]) {

	// Parse the user command line arguments:
	std::pair<std::wstring, std::wstring> input;
	if (!parseCommandLine(input, argc, argv))
	{
		return 1;
	}

	try
	{
		const ProtectorClient protectorClient;

		// Adding path to block:
		if (input.first == L"-a")
		{
			// TODO: Make sure the given path is legit, otherwise display error to the user

			protectorClient.addPath(input.second);
		}

		// Remove blocked path:
		else if (input.first == L"-r")
		{
			// TODO: Make sure the given path is legit, otherwise display error to the user

			protectorClient.removePath(input.second);
		}

		// Get all the user defined paths:
		else if (input.first == L"-p")
		{
			std::vector<std::wstring> paths = protectorClient.getAllPaths();
			for (auto& path : paths)
			{
				std::wcout << "[Path] :" << path << std::endl;
			}
		}
	}
	catch (std::exception& exception)
	{
		std::cout << "[Error]" << exception.what() << std::endl;
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
		<< "\t-a,--append <PATH>\tSpecify path to protect from\n"
		<< "\t-r,--remove <PATH>\tSpecify path to remove from protection\n"
		<< "\t-p,--paths \t\tShow all the defined paths\n"
		<< "\t-e,--events \t\tShow event of blocked execution\n"
		<< std::endl;
}