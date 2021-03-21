#include "ProtectorClient.h"
#include "Win32ErrorCodeException.h"
#include "../Protector/ProtectorCommon.h"

#include <Windows.h>

ProtectorClient::ProtectorClient()
{
	this->protectorDevice.reset(CreateFile(ProtectorClient::DEVICE_NAME.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
		OPEN_EXISTING, 0, nullptr));

	if (INVALID_HANDLE_VALUE == this->protectorDevice.get())
	{
		throw Win32ErrorCodeException("Could not get Protector device");
	}
}

void ProtectorClient::addPath(const std::wstring& path) const
{
	ProtectorPath protectorPath = this->createProtectorPath(path);
	protectorPath.Type = RequestType::Add;

	DWORD returnedBytes;
	if (!DeviceIoControl(protectorDevice.get(), IOCTL_PROTECTOR_ADD_PATH, &protectorPath, sizeof(protectorPath),
		nullptr, 0, &returnedBytes, nullptr))
	{
		throw Win32ErrorCodeException("Could not commit device control request");
	}
}

void ProtectorClient::removePath(const std::wstring& path) const
{
	ProtectorPath protectorPath = this->createProtectorPath(path);
	protectorPath.Type = RequestType::Remove;

	DWORD returnedBytes;
	if (!DeviceIoControl(protectorDevice.get(), IOCTL_PROTECTOR_REMOVE_PATH, &protectorPath, sizeof(protectorPath),
		nullptr, 0, &returnedBytes, nullptr)) {
		throw Win32ErrorCodeException("Could not commit device control request");
	}
}

std::vector<std::wstring> ProtectorClient::getAllPaths() const
{
	DWORD returnedBytes;
	int length = 0;

	// Get how much paths available:
	if (!DeviceIoControl(protectorDevice.get(), IOCTL_PROTECTOR_GET_PATHS_LEN, nullptr, 0, &length,
		sizeof(int), &returnedBytes, nullptr))
	{
		throw Win32ErrorCodeException("Could not commit device control request");
	}

	if (0 == length)
	{
		throw std::runtime_error("There are no blocked paths available");
	}

	const std::unique_ptr<ProtectorPath> buffer(new ProtectorPath[length]);
	if (nullptr == buffer)
	{
		throw Win32ErrorCodeException("Could not allocate memory for incoming paths");
	}

	// Requests all the defined paths:
	const DWORD bufferSize = length * sizeof(ProtectorPath);
	if (!DeviceIoControl(protectorDevice.get(), IOCTL_PROTECTOR_GET_PATHS, nullptr, 0, buffer.get(),
		bufferSize, &returnedBytes, nullptr))
	{
		throw Win32ErrorCodeException("Could not commit device control request");
	}

	std::vector<std::wstring> paths;
	paths.reserve(length);

	for (int i = 0; i < length; i++)
	{
		paths.emplace_back(buffer.get()[i].Path);
	}

	return paths;
}

std::vector<std::wstring> ProtectorClient::getAllEvents() const
{
	// TODO: Complete implementation
	return std::vector<std::wstring>();
}

ProtectorPath ProtectorClient::createProtectorPath(const std::wstring& path) const
{
	ProtectorPath protectorPath{ RequestType::Undefined, L"" };
	wcscpy_s(protectorPath.Path, MaxPath + 1, path.data());
	return protectorPath;
}

