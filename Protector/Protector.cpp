#include "pch.h"
#include "Protector.h"
#include "ProtectorCommon.h"
#include "AutoLock.h"

// Define prototypes:
DRIVER_UNLOAD ProtectorUnload;
DRIVER_DISPATCH ProtectorCreateClose;
DRIVER_DISPATCH ProtectorRead;
DRIVER_DISPATCH ProtectorWrite;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	UNREFERENCED_PARAMETER(DriverObject);
	auto status = STATUS_SUCCESS;

	// Define the device symbolic link:
	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Protector");
	bool symLinkCreated = false;

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
	DriverObject->MajorFunction[IRP_MJ_READ] = ProtectorRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = ProtectorWrite;

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
}

/*
* The Driver's dispatch routine for read operations.
*/
NTSTATUS ProtectorRead(PDEVICE_OBJECT, PIRP Irp) {
	auto status = STATUS_SUCCESS;
	auto count = 0;

	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	if (len == 0)
		status = STATUS_INVALID_BUFFER_SIZE;
	else {

		auto buffer = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
		if (!buffer)
			status = STATUS_INSUFFICIENT_RESOURCES;

		else {
			// Read user-mode data to kernel-space:
			RtlInitUnicodeString(&PathToProtect, (PCWSTR)buffer);
			count = PathToProtect.Length;
			KdPrint((DRIVER_PREFIX "read %d bytes: %wZ", count, PathToProtect));
		}
	}

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
}