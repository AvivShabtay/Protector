#include "ProtectorClient.h"
#include "PeResource.h"
#include "ServiceManager.h"
#include "Win32ErrorCodeException.h"
#include "SehTranslatorGuard.h"

#include <iostream>
#include <sstream>
#include <optional>

#include <Psapi.h>
#include <Windows.h>

enum class Option
{
	Load = 0,
	Unload,
	Append,
	Remove,
	Paths,
	Events
};

using OptionArguments = std::pair<Option, std::wstring>;

/* Helper function to parse the user command line to the specific use cases */
std::optional<OptionArguments> parseCommandLine(int argc, wchar_t** argv);

/* Helper function to display the application options. */
void showUsage(const std::wstring& appName);

std::wstring getTempDirectoryPath();
std::wstring createTemporaryPath(const std::wstring& exeNameWithExtension);

const int PROTECTOR_DRIVER_RESOURCE_ID = 101;
const std::wstring RESOURCE_NAME(L"PROTECTOR_DRIVER");
const std::wstring DRIVER_NAME_WITH_EXTENSION(L"Protector.sys");
const std::wstring SERVICE_NAME(L"Protector");
const std::uint32_t SERVICE_TYPE = SERVICE_KERNEL_DRIVER;

int wmain(int argc, PWCHAR argv[]) {

	SehTranslatorGuard sehTranslatorGuard;

	std::optional<OptionArguments> option = parseCommandLine(argc, argv);
	if (!option.has_value())
	{
		showUsage(argv[0]);
		return 1;
	}

	try
	{
		switch (option.value().first)
		{


		case Option::Load:
		{
			// Loading Protector driver

			const PeResource resource(PROTECTOR_DRIVER_RESOURCE_ID, RESOURCE_NAME);

			const std::wstring binPath(createTemporaryPath(DRIVER_NAME_WITH_EXTENSION));
			resource.saveResourceToFileSystem(binPath);
			std::wcout << "[+] Dump Protector.sys to: " << binPath << std::endl;

			ServiceManager serviceManager(SERVICE_NAME, binPath, SERVICE_TYPE);
			serviceManager.installAndStart();
			std::wcout << "[+] Install Protector driver successfully" << std::endl;

			break;
		}

		case Option::Unload:
		{
			// Unloading Protector driver

			const std::wstring binPath(createTemporaryPath(DRIVER_NAME_WITH_EXTENSION));

			ServiceManager serviceManager(SERVICE_NAME, binPath, SERVICE_TYPE);
			serviceManager.stopAndRemove();
			std::wcout << "[+] Unloading Protector driver successfully" << std::endl;

			if (!DeleteFile(binPath.c_str()))
			{
				throw Win32ErrorCodeException("Could not delete Protector.sys file");
			}
			std::wcout << "[+] Remove Protector driver successfully" << std::endl;

			break;
		}

		case Option::Append:
		{
			// Adding path to block

			// TODO: Make sure the given path is legit, otherwise display error to the user

			const ProtectorClient protectorClient;
			protectorClient.addPath(option.value().second);
			std::wcout << "[+] Add protection against executing programs from: " << option.value().second << std::endl;

			break;
		}

		case Option::Remove:
		{
			// Remove blocked path

			// TODO: Make sure the given path is legit, otherwise display error to the user

			const ProtectorClient protectorClient;
			protectorClient.removePath(option.value().second);
			std::wcout << "[+] Remove protection against executing programs from: " << option.value().second << std::endl;

			break;
		}

		case Option::Paths:
		{
			// Get all the user defined paths

			const ProtectorClient protectorClient;
			std::vector<std::wstring> paths = protectorClient.getAllPaths();
			for (auto& path : paths)
			{
				std::wcout << "[Path] :" << path << std::endl;
			}

			break;
		}

		case Option::Events:
		{
			break;
		}

		default:
			break;
		}
	}
	catch (std::exception& exception)
	{
		std::cout << exception.what() << std::endl;
		return 1;
	}

	return 0;
}

std::optional<OptionArguments> parseCommandLine(int argc, wchar_t** argv)
{
	if (argc <= 1 || argc > 3)
	{
		return std::nullopt;
	}

	const std::wstring option = argv[1];
	std::optional<OptionArguments> result = std::nullopt;

	if ((L"-h" == option) || (L"--help" == option))
	{
		return std::nullopt;
	}

	else if ((L"-i" == option) || (L"--install" == option))
	{
		result = { Option::Load, L"" };
	}

	else if ((L"-s" == option) || (L"--stop" == option))
	{
		result = { Option::Unload, L"" };
	}

	else if (3 == argc && (L"-a" == option) || (L"--append" == option))
	{
		result = { Option::Append, argv[2] };
	}

	else if (3 == argc && (L"-r" == option) || (L"--remove" == option))
	{
		result = { Option::Remove, argv[2] };
	}

	else if ((L"-p" == option) || (L"--paths" == option))
	{
		result = { Option::Paths, L"" };
	}

	else if ((L"-e" == option) || (L"--events" == option))
	{
		result = { Option::Events, L"" };
	}

	return result;
}

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