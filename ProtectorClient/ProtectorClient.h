#pragma once

#include "AutoHandle.h"
#include "../Protector/ProtectorCommon.h"

#include <iostream>
#include <vector>

class ProtectorClient final
{
public:
	ProtectorClient();

	~ProtectorClient() = default;

	// Delete copy ctor, move ctor, assignment
	ProtectorClient(const ProtectorClient&) = delete;
	ProtectorClient& operator=(const ProtectorClient&) = delete;
	ProtectorClient(ProtectorClient&&) = delete;
	ProtectorClient& operator=(ProtectorClient&&) = delete;

	/* Request protection from executing programs from specific path. */
	void addPath(const std::wstring& path) const;

	/* Request remove protection of specific path. */
	void removePath(const std::wstring& path) const;

	/* Request all available paths. */
	std::vector<std::wstring> getAllPaths() const;

	std::vector<std::wstring> getAllEvents() const;

	const std::wstring DEVICE_NAME = L"\\\\.\\Protector";

private:
	ProtectorPath createProtectorPath(const std::wstring& path) const;

	AutoHandle protectorDevice;
};

