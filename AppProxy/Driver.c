#include <ntddk.h>
#include <wdf.h>
#include <wdmsec.h>

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD KmdfHelloWorldEvtDeviceAdd;
EVT_WDF_TIMER AppProxy_OnTimer;
NTSTATUS AppProxy_AddDevice(_In_ WDFDRIVER* pWDFDriver, _Out_ WDFDEVICE* pDevice);
NTSTATUS SetupTimer(_In_ WDFOBJECT parent, _Out_ WDFTIMER* phTimer);

WDFTIMER         g_timerHandle;
WDFDRIVER        g_WDFDriver;
WDFDEVICE        g_WDFDevice;

#define HELPER_INIT() NTSTATUS status;
#define HELPER_BAIL() return status;
#define HELPER_BAIL_ON_FAILURE(status) if (!NT_SUCCESS(status)) { return status; }
#define HELPER_CALL(call) status = (call); HELPER_BAIL_ON_FAILURE(status);
#define HELPER_END() return status;

#define Debug(msg, ...) KdPrint(("AppProxy KmdfHelloWorld: " msg "\n", __VA_ARGS__))

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT  DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	HELPER_INIT();
	WDF_DRIVER_CONFIG config;

	Debug("DriverEntry", 0);
	WDF_DRIVER_CONFIG_INIT(&config, WDF_NO_EVENT_CALLBACK);
	config.DriverInitFlags |= WdfDriverInitNonPnpDriver;

	HELPER_CALL(WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, &g_WDFDriver));

	HELPER_CALL(AppProxy_AddDevice(&g_WDFDriver, &g_WDFDevice));

	HELPER_CALL(SetupTimer(g_WDFDevice, &g_timerHandle));

	HELPER_END();
}

NTSTATUS AppProxy_AddDevice(_In_ WDFDRIVER* pWDFDriver, _Out_ WDFDEVICE* pDevice)
{
	NTSTATUS              status = STATUS_SUCCESS;
	PWDFDEVICE_INIT       pWDFDeviceInit = 0;
	WDF_OBJECT_ATTRIBUTES attributes = { 0 };

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	pWDFDeviceInit = WdfControlDeviceInitAllocate(*pWDFDriver, &SDDL_DEVOBJ_KERNEL_ONLY);

	if (pWDFDeviceInit == 0)
	{
		status = STATUS_UNSUCCESSFUL;

		Debug("WdfControlDeviceInitAllocate() [status: %#x]", status);

		HELPER_BAIL();
	}

	WdfDeviceInitSetDeviceType(pWDFDeviceInit, FILE_DEVICE_NETWORK);

	status = WdfDeviceCreate(&pWDFDeviceInit,
		&attributes,
		pDevice);
	if (status != STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVNETWORK_ID,
			DPFLTR_ERROR_LEVEL,
			" !!!! PrvDriverDeviceAdd : WdfDeviceCreate() [status: %#x]\n",
			status);

		HELPER_BAIL();
	}

	WdfControlFinishInitializing(*pDevice);

	return status;
}


VOID AppProxy_OnTimer(_In_ WDFTIMER timer) {
	UNREFERENCED_PARAMETER(timer);
	Debug("AppProxy timer fired", 0);
}

NTSTATUS SetupTimer(_In_ WDFOBJECT parent, _Out_ WDFTIMER* pTimer) {
	NTSTATUS status;
	BOOLEAN success;
	WDF_TIMER_CONFIG  timerConfig;
	WDF_OBJECT_ATTRIBUTES  timerAttributes;

	WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, AppProxy_OnTimer, 2000);
	timerConfig.AutomaticSerialization = TRUE;

	WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
	timerAttributes.ParentObject = parent;

	status = WdfTimerCreate(
		&timerConfig,
		&timerAttributes,
		pTimer
	);
	HELPER_BAIL_ON_FAILURE(status);

	success = WdfTimerStart(*pTimer, -1);
	if (!success) {
		status = STATUS_UNSUCCESSFUL;
		HELPER_BAIL();
	}

	return status;
}
