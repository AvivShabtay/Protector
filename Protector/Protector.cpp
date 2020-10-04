#include "pch.h"
#include "Protector.h"
#include "ProtectorCommon.h"
#include "AutoLock.h"

// Define prototypes:
DRIVER_UNLOAD ProtectorUnload;
DRIVER_DISPATCH ProtectorCreateClose;
DRIVER_DISPATCH ProtectorRead;
DRIVER_DISPATCH ProtectorWrite;
DRIVER_DISPATCH ProtectorDeviceControl;
NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0);

// Global variables:
Globals g_Globals;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	UNREFERENCED_PARAMETER(DriverObject);
	auto status = STATUS_SUCCESS;

	// Define the device symbolic link:
	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Protector");
	bool symLinkCreated = false;

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

	} while (false);

	// In case of failure:
	if (!NT_SUCCESS(status)) {
		if (symLinkCreated)
			IoDeleteSymbolicLink(&symLink);
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
	}

	// Define driver prototypes:
	DriverObject->DriverUnload = ProtectorUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = ProtectorCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = ProtectorCreateClose;
	//DriverObject->MajorFunction[IRP_MJ_READ] = ProtectorRead;
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

NTSTATUS ProtectorDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto& deviceIoControl = stack->Parameters.DeviceIoControl;

	// Check if this is illegal IOCTL:
	if (deviceIoControl.IoControlCode != IOCTL_PROTECTOR_SET_PATH)
		return CompleteIrp(Irp, STATUS_INVALID_DEVICE_REQUEST);

	// Check if the incoming buffer isn't correct:
	if (deviceIoControl.InputBufferLength < sizeof(ProtectorPath))
		return CompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL);

	// Print some data for tests:
	auto buffer = (ProtectorPath*)Irp->AssociatedIrp.SystemBuffer;

	// TODO: check if the buffer is valid (size, not-empty).

	switch (buffer->Type)
	{
	case RequestType::Add:
	{
		// Allocating continuous memory for new item:
		USHORT size = sizeof(FullItem<ProtectorPath>);
		auto info = (FullItem<ProtectorPath>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation\n"));
			return CompleteIrp(Irp, STATUS_BUFFER_TOO_SMALL); //TODO: Change to the correct status !!!!
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
		//::memcpy(item.Path, buffer->Path, min(wcslen(buffer->Path), MaxPath));

		// Add item to the linked-list:
		PushItem(&info->Entry);

		KdPrint((DRIVER_PREFIX "Add path: %ws\n", (PCWSTR)item.Path));

		break;
	}
	case RequestType::Remove:
	{
		//KdPrint((DRIVER_PREFIX "Remove path: %wZ\n", path));
		break;
	}
	default:
		break;
	}

	// Complete the request:
	return CompleteIrp(Irp, STATUS_SUCCESS);
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