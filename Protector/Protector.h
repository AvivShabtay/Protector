#pragma once
#include "FastMutex.h"

#define DRIVER_PREFIX "Protector: "
#define DRIVER_TAG 'Prtc'

UNICODE_STRING PathToProtect;
FastMutex Mutex;

struct Globals {
	LIST_ENTRY ItemsHead;
	int ItemCount;
	FastMutex Mutex;
};

template<typename T>
struct FullItem {
	LIST_ENTRY Entry;
	T Data;
};
