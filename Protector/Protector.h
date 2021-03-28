#pragma once
#include "FastMutex.h"
#include "Vector.h"
#include "String.h"

#include <ntddk.h>

#define DRIVER_PREFIX "Protector: "
#define DRIVER_TAG 'Prtc'

using ProtectorString = String<DRIVER_TAG, PagedPool>;
using ProtectorVector = Vector<ProtectorString*, DRIVER_TAG>;

/* The Driver's unload function called whenever the kernel unloads the driver. */
DRIVER_UNLOAD ProtectorUnload;

/* The Driver's default dispatch to all the unimplemented dispatch. */
DRIVER_DISPATCH DefaultDispatch;

/* The Driver's default dispatch for create and close dispatch. */
DRIVER_DISPATCH CreateCloseDispatch;

/* Dispatch function for device control I\O requests. */
DRIVER_DISPATCH ProtectorDeviceControl;

/* Helper function to return the IRP to the caller. */
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);

/* The Driver's callback function for Process Notifications. */
void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);

/* Check if given path include in the black-listed list. */
bool IsPathInBlackList(const ProtectorString& imagePath);

/* Handler for add path requests. */
NTSTATUS AddPathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);

/* Handler for remove path requests. */
NTSTATUS RemovePathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);

/* Handler for path list length requests. */
NTSTATUS GetPathListLengthHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);

/* Handler for get paths requests. */
NTSTATUS GetPathsHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);

/* Check if the input buffer size is not smaller than required. */
bool IsValidInputBuffer(_In_ PIO_STACK_LOCATION StackLocation, _In_ ULONGLONG validSize);

/* Check if the output buffer size is not smaller than required. */
bool IsValidOutputBuffer(_In_ PIO_STACK_LOCATION StackLocation, _In_ ULONGLONG validSize);