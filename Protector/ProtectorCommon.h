#pragma once

#define PROTECTOR_DEVICE 0x8000

#define IOCTL_PROTECTOR_SET_PATH CTL_CODE(PROTECTOR_DEVICE, \
	0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

const int MaxPath = 300;

enum class RequestType : short {
	Add,
	Remove
};

struct ProtectorPath {
	RequestType Type;
	WCHAR Path[MaxPath + 1];
};