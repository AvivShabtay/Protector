#include "Protector.h"
#include "ProtectorCommon.h"
#include "AutoLock.h"

#include <ntddk.h>

// Define prototypes:
DRIVER_UNLOAD ProtectorUnload;
DRIVER_DISPATCH ProtectorCreateClose;
DRIVER_DISPATCH ProtectorRead;
DRIVER_DISPATCH ProtectorWrite;
DRIVER_DISPATCH ProtectorDeviceControl;
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);
void PushItem(LIST_ENTRY* entry);
void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);
PLIST_ENTRY CheckPathInBlackList(WCHAR* imagePath);
void ConvertWcharStringToUpperCase(WCHAR* source);
NTSTATUS AddPathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);
NTSTATUS RemovePathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);
NTSTATUS GetPathListLengthHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);
NTSTATUS GetPathsHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation);

// Global variables:
Globals g_Globals;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {

	auto status = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Protector");
	bool symLinkCreated = false;
	bool processCallbackRegistered = false;

	// Initialize linked list for the events:
	InitializeListHead(&g_Globals.ItemsHead);
	g_Globals.Mutex.Init();

	do {

		// Create device object for user-mode communication:
		UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\Protector");
		status = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, TRUE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device (0x%08X)\n", status));
			break;
		}

		// Choose the IO communication method:
		DeviceObject->Flags |= DO_DIRECT_IO;

		// Create symbolic link to the device object:
		status = IoCreateSymbolicLink(&symLink, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (0x%08X)\n", status));
			break;
		}

		symLinkCreated = true;

		// Register callback function for process creation:
		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register process callback (0x%08X)\n", status));
			break;
		}

		processCallbackRegistered = true;

	} while (false);

	// In case of failure:
	if (!NT_SUCCESS(status)) {
		if (symLinkCreated)
		{
			IoDeleteSymbolicLink(&symLink);
		}
		if (DeviceObject)
		{
			IoDeleteDevice(DeviceObject);
		}
		if (processCallbackRegistered)
		{
			PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
		}

	}

			IoDeleteSymbolicLink(&symLink);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
	}

	// Define driver prototypes:
	DriverObject->DriverUnload = ProtectorUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = ProtectorCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = ProtectorCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ] = ProtectorRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = ProtectorWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProtectorDeviceControl;

	return status;
}

/*
* The Driver's dispatch routine for create and close operations.
*/
NTSTATUS ProtectorCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return STATUS_SUCCESS;
}

/*
* The Driver's unload function called whenever the kernel unloads
* the driver.
*/
void ProtectorUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	// Remove the callback to the notification:
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);

	// Remove the symbolic link:
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Protector");
	IoDeleteSymbolicLink(&symLink);

	// Remove the device driver:
	IoDeleteDevice(DriverObject->DeviceObject);

	// Free allocated memory:
	while (!IsListEmpty(&g_Globals.ItemsHead)) {
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ProtectorPath>, Entry));
	}
}

/*
* The Driver's dispatch routine for read operations.
*/
NTSTATUS ProtectorRead(PDEVICE_OBJECT, PIRP Irp) {
	auto status = STATUS_SUCCESS;
	auto count = 0;

	// Finish the request:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = count;
	IoCompleteRequest(Irp, 0);
	return status;
}

/*
* The Driver's dispatch routine for write operations.
*/
NTSTATUS ProtectorWrite(PDEVICE_OBJECT, PIRP Irp) {
	auto status = STATUS_SUCCESS;
	auto count = 0;

	// Finish the request:
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = count;
	IoCompleteRequest(Irp, 0);
	return status;
}

/*
* Handler for add path requests.
*/
NTSTATUS AddPathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation) {
	auto& deviceIoControl = StackLocation->Parameters.DeviceIoControl;

	// Check if the incoming buffer isn't correct:
	if (deviceIoControl.InputBufferLength < sizeof(ProtectorPath))
		return STATUS_BUFFER_TOO_SMALL;

	// Get data from the request:
	auto buffer = (ProtectorPath*)Irp->AssociatedIrp.SystemBuffer;
	if (buffer->Type != RequestType::Add)
		return STATUS_INVALID_DEVICE_REQUEST;

	// Allocating continuous memory for new item:
	USHORT size = sizeof(FullItem<ProtectorPath>);
	auto info = (FullItem<ProtectorPath>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);
	if (info == nullptr) {
		KdPrint((DRIVER_PREFIX "failed allocation\n"));
		//TODO: Change to the correct status !!!!
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Convert the path to upper case letters:
	ConvertWcharStringToUpperCase(buffer->Path);

	// Zero the entire structure:
	::memset(info, 0, size);

	// Copy data to the item:
	auto& item = info->Data;
	item.Type = buffer->Type;
	::memset(item.Path, 0, MaxPath + 1);
	::wcsncpy(item.Path, buffer->Path, min(wcslen(buffer->Path), MaxPath));

	// Add item to the linked-list:
	PushItem(&info->Entry);

	KdPrint((DRIVER_PREFIX "Add path: %ws\n", (PCWSTR)item.Path));

	// Complete the request:
	return STATUS_SUCCESS;
}

/*
* Handler for remove path requests.
*/
NTSTATUS RemovePathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation) {
	auto& deviceIoControl = StackLocation->Parameters.DeviceIoControl;

	// Check if the incoming buffer isn't correct:
	if (deviceIoControl.InputBufferLength < sizeof(ProtectorPath))
		return STATUS_BUFFER_TOO_SMALL;

	// Get data from the request:
	auto buffer = (ProtectorPath*)Irp->AssociatedIrp.SystemBuffer;
	if (buffer->Type != RequestType::Remove)
		return STATUS_INVALID_DEVICE_REQUEST;

	// Convert the path to upper case letters:
	ConvertWcharStringToUpperCase(buffer->Path);

	// Get pointer to the item with that path:
	auto pEntry = CheckPathInBlackList(buffer->Path);

	// If the path exist, remove it:
	if (pEntry) {
		AutoLock<FastMutex> lock(g_Globals.Mutex);

		RemoveEntryList(pEntry);
		KdPrint((DRIVER_PREFIX "Remove path: %ws\n", (PCWSTR)buffer->Path));
		ExFreePool(CONTAINING_RECORD(pEntry, FullItem<ProtectorPath>, Entry));

		g_Globals.ItemCount--;
		return STATUS_SUCCESS;
	}

	// Complete the request:
	return STATUS_INVALID_DEVICE_REQUEST;
}

/*
* Handler for path list length requests.
*/
NTSTATUS GetPathListLengthHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation) {
	auto& deviceIoControl = StackLocation->Parameters.DeviceIoControl;

	// Check if the incoming buffer isn't correct:
	if (deviceIoControl.OutputBufferLength < sizeof(ULONG))
		return STATUS_BUFFER_TOO_SMALL;

	// Get data from the request:
	auto buffer = (int*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

	if (buffer) {
		*buffer = g_Globals.ItemCount;
		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_DEVICE_REQUEST;
}

/*
* Handler for get paths requests.
*/
NTSTATUS GetPathsHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation) {
	auto& deviceIoControl = StackLocation->Parameters.DeviceIoControl;

	// Check if the incoming buffer isn't correct:
	if (deviceIoControl.OutputBufferLength < sizeof(ProtectorPath))
		return STATUS_BUFFER_TOO_SMALL;

	// Get data from the request:
	auto buffer = (UCHAR*)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

	if (buffer) {
		AutoLock<FastMutex> lock(g_Globals.Mutex);

		auto size = g_Globals.ItemCount;
		if (size <= 0)
			return STATUS_SUCCESS;

		// Iterate the linked-list:
		PLIST_ENTRY pHead = &g_Globals.ItemsHead;
		PLIST_ENTRY pEntry = pHead->Flink;

		while (pEntry != pHead) {
			// Get the item from the linked list:
			auto item = CONTAINING_RECORD(pEntry, FullItem<ProtectorPath>, Entry);

			// Copy data to user buffer:
			::memcpy(buffer, &item->Data, sizeof(ProtectorPath));

			// Move to the next list entry:
			pEntry = pEntry->Flink;
			buffer += sizeof(ProtectorPath);
		}

		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_DEVICE_REQUEST;
}

/*
* Dispatch function for device control I\O requests.
*/
NTSTATUS ProtectorDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
	auto status = STATUS_NOT_SUPPORTED;

	// Check for valid IRP:
	if (!Irp)
		return CompleteIrp(Irp, STATUS_NOT_SUPPORTED);

	// Run the corresponding handler for the request:
	switch (controlCode) {
	case IOCTL_PROTECTOR_ADD_PATH:
		status = AddPathHandler(Irp, stack);
		break;

	case IOCTL_PROTECTOR_REMOVE_PATH:
		status = RemovePathHandler(Irp, stack);
		break;

	case IOCTL_PROTECTOR_GET_PATHS_LEN:
		status = GetPathListLengthHandler(Irp, stack);
		break;

	case IOCTL_PROTECTOR_GET_PATHS:
		status = GetPathsHandler(Irp, stack);
		break;

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	// Complete the request:
	return CompleteIrp(Irp, status);
}

/*
* Helper function to return the IRP to the caller.
*/
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR info) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);
	return status;
}

/*
*
*/
void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo) {
	UNREFERENCED_PARAMETER(Process);

	// Process creation notification:
	if (CreateInfo) {

		// Check if the created process contains ImgaeFileName field:
		if (CreateInfo->FileOpenNameAvailable) {
			auto imageFileName = CreateInfo->ImageFileName;
			KdPrint((DRIVER_PREFIX "New process created, ImageFileName: %wZ\n", imageFileName));

			// Use temporary string:
			WCHAR imagePath[MaxPath];
			::memset(imagePath, 0, MaxPath);
			// There is no guarantee that a UNICODE_STRING will be NULL terminated.
			// https://community.osr.com/discussion/243646/wcsstr-in-kernel
			::wcsncpy(imagePath, CreateInfo->ImageFileName->Buffer, min(CreateInfo->ImageFileName->Length, MaxPath));
			//::memcpy(imagePath, CreateInfo->ImageFileName->Buffer, min(CreateInfo->ImageFileName->Length, MaxPath));
			ConvertWcharStringToUpperCase(imagePath);

			// Check if the process path is black-listed:
			if (CheckPathInBlackList(imagePath)) {

				// The path is black-listed, block it:
				CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;

				// Print log:
				auto pid = HandleToULong(ProcessId);
				KdPrint((DRIVER_PREFIX "blocked PID=%d, ImageFileName: %wZ\n", pid, imageFileName));
			}
		}
	}
}

/*
* Check if given path include in the black-listed list.
*/
PLIST_ENTRY CheckPathInBlackList(WCHAR* imagePath) {

	AutoLock<FastMutex> lock(g_Globals.Mutex);

	// Check if the linked list empty:
	if (g_Globals.ItemCount <= 0)
		return false;

	// Iterate the linked-list:
	PLIST_ENTRY pHead = &g_Globals.ItemsHead;
	PLIST_ENTRY pEntry = pHead->Flink;

	while (pEntry != pHead) {

		// Get the item from the linked list:
		auto item = CONTAINING_RECORD(pEntry, FullItem<ProtectorPath>, Entry);

		auto blackListedpath = item->Data.Path;

		// Avoid case of disabling every path by mistake:
		size_t len1 = wcslen(blackListedpath);
		size_t len2 = wcslen(imagePath);

		if (len1 == 0 || len2 == 0)
			return false;

		// Black-listed path:
		if (wcsstr(imagePath, blackListedpath))
			return pEntry;

		// Move to the next list entry:
		pEntry = pEntry->Flink;
	}

	return nullptr;
}


/*
*
*/
void PushItem(LIST_ENTRY* entry) {
	AutoLock<FastMutex> lock(g_Globals.Mutex);

	// In case of too many items, we will remove the oldest item:
	if (g_Globals.ItemCount > 1024) {
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;
		auto item = CONTAINING_RECORD(head, FullItem<ProtectorPath>, Entry);
		ExFreePool(item);
	}

	// Push item to the end of the list:
	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;
}

/*
*
*/
void ConvertWcharStringToUpperCase(WCHAR* source) {
	auto len = wcslen(source);

	for (int i = 0; i < len; i++) {
		auto upperCase = ::RtlUpcaseUnicodeChar(source[i]);
		::memcpy(source + i, &upperCase, 1);
	}
}