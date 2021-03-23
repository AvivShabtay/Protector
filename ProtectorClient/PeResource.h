#pragma once

#include <iostream>

#include <Windows.h>

class PeResource
{
public:
	explicit PeResource(int resourceId, std::wstring resourceName);

	virtual ~PeResource();

	LPVOID getResourceData() const;

	void saveResourceToFileSystem(const std::wstring& path) const;

private:
	int m_resourceId;
	std::wstring m_resourceName;
	LPVOID m_resourceData;
	int m_resourceSize;
};

