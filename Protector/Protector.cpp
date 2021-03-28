#include "Protector.h"
#include "ProtectorCommon.h"
#include "Memory.h"

#include <ntddk.h>

ProtectorVector* g_vector;

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	auto status = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject = nullptr;
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Protector");
	bool symLinkCreated = false;
	bool processCallbackRegistered = false;

	g_vector = new (NonPagedPool, DRIVER_TAG) ProtectorVector();

	do
	{
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
	if (!NT_SUCCESS(status))
	{
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

	DriverObject->DriverUnload = ProtectorUnload;

	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DefaultDispatch;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseDispatch;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ProtectorDeviceControl;

	return status;
}

void ProtectorUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	// Remove the callback to the notification:
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);

	// Remove the symbolic link:
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\Protector");
	IoDeleteSymbolicLink(&symLink);

	// Remove the device driver:
	IoDeleteDevice(DriverObject->DeviceObject);

	// Free allocated memory:
	for (auto& item : *g_vector)
	{
		delete item;
	}

	delete g_vector;
}

NTSTATUS CreateCloseDispatch(PDEVICE_OBJECT, PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return STATUS_SUCCESS;
}

NTSTATUS DefaultDispatch(PDEVICE_OBJECT, PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return STATUS_NOT_SUPPORTED;
}

NTSTATUS ProtectorDeviceControl(PDEVICE_OBJECT, PIRP Irp)
{
	const PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	const ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;
	NTSTATUS status = STATUS_NOT_SUPPORTED;

	if (nullptr == Irp)
	{
		return CompleteIrp(Irp, STATUS_NOT_SUPPORTED);
	}

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

	return CompleteIrp(Irp, status);
}

NTSTATUS AddPathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation)
{
	if (!IsValidInputBuffer(StackLocation, sizeof(ProtectorPath)))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	auto* buffer = static_cast<ProtectorPath*>(Irp->AssociatedIrp.SystemBuffer);
	if (RequestType::Add != buffer->Type)
	{
		KdPrint((DRIVER_PREFIX "Invalid type for request\n"));
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	auto* blockedPath = new (NonPagedPool, DRIVER_TAG) ProtectorString(buffer->Path);
	blockedPath->toUpper();

	g_vector->add(blockedPath);

	KdPrint((DRIVER_PREFIX "Add path: %ws\n", blockedPath->get()));
	return STATUS_SUCCESS;
}

NTSTATUS RemovePathHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation)
{
	if (!IsValidInputBuffer(StackLocation, sizeof(ProtectorPath)))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	auto* buffer = static_cast<ProtectorPath*>(Irp->AssociatedIrp.SystemBuffer);
	if (buffer->Type != RequestType::Remove)
	{
		KdPrint((DRIVER_PREFIX "Invalid type for request\n"));
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	ProtectorString temp(buffer->Path);
	temp.toUpper();

	for (ULONG i = 0; i < g_vector->size(); i++)
	{
		const ProtectorString* current = g_vector->getAt(i);
		if (0 == temp.compare(*current))
		{
			g_vector->removeAt(i);
			return STATUS_SUCCESS;
		}
	}

	return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS GetPathListLengthHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation)
{
	if (!IsValidOutputBuffer(StackLocation, sizeof(ULONG)))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Get data from the request:
	auto* lengthBuffer = static_cast<int*>(MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority));


	if (nullptr != lengthBuffer)
	{
		*lengthBuffer = g_vector->size();
		return STATUS_SUCCESS;
	}

	return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS GetPathsHandler(_In_ PIRP Irp, _In_ PIO_STACK_LOCATION StackLocation)
{
	if (!IsValidOutputBuffer(StackLocation, sizeof(ProtectorPath)))
	{
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Get data from the request:
	auto* clientBuffer = static_cast<UCHAR*>(MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority));

	if (nullptr == clientBuffer)
	{
		return STATUS_INVALID_DEVICE_REQUEST;
	}

	for (auto& item : *g_vector)
	{
		ProtectorPath path{};
		wcscpy_s(path.Path, MaxPath, item->get());

		memcpy(clientBuffer, &path, sizeof(ProtectorPath));

		// Move to the next list entry:
		clientBuffer += sizeof(ProtectorPath);
	}

	return STATUS_SUCCESS;
}

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status, ULONG_PTR info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);
	return status;
}

void OnProcessNotify(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	UNREFERENCED_PARAMETER(Process);

	// If no process creation or the notification doesn't hold the image path:
	if (nullptr == CreateInfo || !CreateInfo->FileOpenNameAvailable)
	{
		return;
	}

	// TODO: Take care in case the created process doesn't contains ImageFileName field

	const ProtectorString imagePath(CreateInfo->ImageFileName->Buffer);
	KdPrint((DRIVER_PREFIX "New process created, ImageFileName: %ws\n", imagePath.get()));

	if (IsPathInBlackList(imagePath)) {

		// The path is black-listed, block it:
		CreateInfo->CreationStatus = STATUS_ACCESS_DENIED;

		const ULONG pid = HandleToULong(ProcessId);
		KdPrint((DRIVER_PREFIX "blocked PID=%d, ImageFileName: %ws\n", pid, imagePath.get()));
	}
}

bool IsPathInBlackList(const ProtectorString& imagePath)
{
	if (0 >= g_vector->size())
	{
		return false;
	}

	const ProtectorString temp(imagePath.toUpper());

	for (auto& blockedString : *g_vector)
	{
		if (blockedString->isSubString(temp))
		{
			return true;
		}
	}

	return false;
}

bool IsValidInputBuffer(PIO_STACK_LOCATION StackLocation, ULONGLONG validSize)
{
	auto& deviceIoControl = StackLocation->Parameters.DeviceIoControl;

	if (validSize > deviceIoControl.InputBufferLength)
	{
		KdPrint((DRIVER_PREFIX "Invalid incoming input buffer\n"));
		return false;
	}

	return true;
}

bool IsValidOutputBuffer(PIO_STACK_LOCATION StackLocation, ULONGLONG validSize)
{
	auto& deviceIoControl = StackLocation->Parameters.DeviceIoControl;

	if (validSize > deviceIoControl.OutputBufferLength)
	{
		KdPrint((DRIVER_PREFIX "Invalid incoming output buffer\n"));
		return false;
	}

	return true;
}
