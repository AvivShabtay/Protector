#pragma once
#include "FastMutex.h"

#include <ntddk.h>

#define DRIVER_PREFIX "Protector: "
#define DRIVER_TAG 'Prtc'

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
