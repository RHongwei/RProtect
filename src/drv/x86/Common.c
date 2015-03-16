/*++

Module Name:

	Common.c - ����ģ��

Abstract:

	��ģ�鵼��һЩ������ģ�鶼���õ��Ĺ��������������㷨�ȡ�

Author:

	Fypher

	2012/02/27

--*/
#include <ntifs.h>
#include <ntimage.h>
#include "Common.h"
#include "Hook.h"
#include "SysCallFlt.h"
#include "EventHandler.h"
#include "WhiteBlackList.h"

ULONG g_ulOsVersion = OS_VERSION_ERROR;

ULONG g_ulKiSystemService = 0;

//ULONG g_ulKiSystemCall64 = 0;

ULONG g_ulKeServiceDescriptorTable = 0;
ULONG g_ulKiServiceTable = 0;
ULONG g_ulServiceNumber = 0;

ULONG g_ulMmUserProbeAddress = 0;

ULONG g_ulKeServiceDescriptorTableShadow = 0;
ULONG g_ulW32pServiceTable = 0;
ULONG g_ulShadowServiceNumber = 0;

ULONG g_ulNtdllBase = 0;
ULONG g_ulNtDllSize = 0;


PVOID g_pPhysicalMemoryObj = NULL;

// hard coded
ULONG g_ulShadowId_NtUserSetWindowsHookEx = 0;

// for self protect
PEPROCESS g_pProtected = NULL;
//#pragma data_seg()

ULONG g_ulHookPoint = 0;
ULONG g_ulJmpBackPoint = 0;

BYTE g_Signature[10] = {0x8b, 0xfc, 0x3b, 0x35, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x83};
BYTE g_OriginalBytes[8] = {0};

// UCHAR g_HookCodes[17] = {0x68, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, 0xC3, 0x90, 0x90, 0x90};



ULONG GetOsVersion()
{

	ULONG ulOsVersion;
	RTL_OSVERSIONINFOW OsVersionInfo;

	PAGED_CODE();

	OsVersionInfo.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

	if (!NT_SUCCESS(RtlGetVersion(&OsVersionInfo)))
		return OS_VERSION_ERROR;

	switch (OsVersionInfo.dwBuildNumber) {
		case 2195:
			ulOsVersion = OS_VERSION_2000;
			break;

		case 2600:
			ulOsVersion = OS_VERSION_XP;
			break;

		case 3790:
			ulOsVersion = OS_VERSION_SERVER_2003;
			break;

		case 6000:
			ulOsVersion = OS_VERSION_VISTA;
			break;

		case 6001:
			ulOsVersion = OS_VERSION_VISTA_SP1;
			break;

		case 6002:
			ulOsVersion = OS_VERSION_VISTA_SP2;
			break;

		case 7600:
			ulOsVersion = OS_VERSION_WIN7;
			break;

		case 7601:
			ulOsVersion = OS_VERSION_WIN7_SP1;
			break;

		default:
			ulOsVersion = OS_VERSION_ERROR;
	}

	return ulOsVersion;
}



BOOLEAN InitCommonVars()
/*++

Routine Description:

	ȫ�ֱ����ĳ�ʼ������


Arguments:

	None.


Return Value:

	TRUE if successful, and FALSE if not.


Author:

	Fypher

--*/
{
	RTL_PROCESS_MODULE_INFORMATION SysModInfo;
	UNICODE_STRING usRoutineName;
	// InitRegKeys();

	ULONG g_ulAddr;

	PAGED_CODE();

	g_pProtected = NULL;

	RtlInitUnicodeString(&usRoutineName, L"MmUserProbeAddress");
	g_ulMmUserProbeAddress = (ULONG)MmGetSystemRoutineAddress(&usRoutineName);

	g_ulKiSystemService = GetKiSystemServiceAddr();

	RtlInitUnicodeString(&usRoutineName, L"KeServiceDescriptorTable");
	g_ulKeServiceDescriptorTable = (ULONG)MmGetSystemRoutineAddress(&usRoutineName);

	if (g_ulKeServiceDescriptorTable && MmIsAddressValid((PVOID)g_ulKeServiceDescriptorTable))
	{
		g_ulKiServiceTable = *(PULONG)g_ulKeServiceDescriptorTable;
		g_ulServiceNumber = *(PULONG)(g_ulKeServiceDescriptorTable + 8);
	}


	g_ulHookPoint = FindHookPoint();
	g_ulJmpBackPoint = g_ulHookPoint + 8;

	if ( GetSysModInfoByName("ntdll.dll", &SysModInfo) )
	{
        g_ulNtdllBase = (ULONG)SysModInfo.ImageBase;
        g_ulNtDllSize = SysModInfo.ImageSize;
	}

	GetSSDTShadow(&g_ulW32pServiceTable, &g_ulShadowServiceNumber);

	g_pPhysicalMemoryObj = GetSectionObjectByName(L"\\Device\\PhysicalMemory");
	if (g_pPhysicalMemoryObj)
		ObDereferenceObject(g_pPhysicalMemoryObj);


	//!!!
	if (g_ulMmUserProbeAddress && MmIsAddressValid((PVOID)g_ulMmUserProbeAddress)) {
		g_ulMmUserProbeAddress = *(PULONG)g_ulMmUserProbeAddress;
	} else {
		return FALSE;
	}


	// verify
	// +++
	if (!MmIsAddressValid((PVOID)g_ulKiSystemService)		||

		//!MmIsAddressValid((PVOID)g_ulKiSystemCall64)		||

		!MmIsAddressValid((PVOID)g_ulKiServiceTable)		||
		!g_ulServiceNumber									||

		!g_ulW32pServiceTable								||
		!g_ulShadowServiceNumber							||

		!MmIsAddressValid((PVOID)g_ulNtdllBase)			||
		!g_ulNtDllSize										||

		!MmIsAddressValid((PVOID)g_ulHookPoint)			||
		!MmIsAddressValid((PVOID)g_ulJmpBackPoint)				)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
	// ---
}


BOOLEAN GetSSDTShadow(PULONG pulSSDTShadow, PULONG pulShadowServiceNum)
{

	ULONG ulAddr;
	UNICODE_STRING usRoutineName;

	RtlInitUnicodeString(&usRoutineName, L"KeAddSystemServiceTable");

	ulAddr = (ULONG)MmGetSystemRoutineAddress(&usRoutineName);

	if (ulAddr && MmIsAddressValid((PVOID)ulAddr))
	{
		ULONG ulEndAddr = ulAddr + 256;

		if ( ulAddr >= ulAddr + 256 ) // overflow!
			return FALSE;

		while ( ulAddr < ulEndAddr )
		{
			if ( MmIsAddressValid((PVOID)ulAddr) &&
				*(USHORT *)ulAddr == 0x888d			)
			{
				ULONG ulXXX = *(PULONG)(ulAddr + 2);
				*pulSSDTShadow = *(PULONG)(ulXXX + 16);
				*pulShadowServiceNum = *(PULONG)(ulXXX + 24);
				return TRUE;
			}
			++ulAddr;
		}
	}
	return FALSE;
}


ULONG GetKiSystemServiceAddr()
{
	IDTINFO IdtInfo;
	PIDTENTRY pIdtEntry;

	__asm sidt IdtInfo;

	pIdtEntry = (PIDTENTRY)((IdtInfo.HiIDTbase << 16) | IdtInfo.LowIDTbase);

	if (pIdtEntry)
	{
		ULONG ulKiSystemService = (ULONG)((pIdtEntry[0x2e].HiOffset << 16) | pIdtEntry[0x2e].LowOffset);
		if (ulKiSystemService && MmIsAddressValid((PVOID)ulKiSystemService))
			return ulKiSystemService;
	}

	return 0;
}


BOOLEAN InitShadowServiceId()
/*++

Routine Description:

	Ѱ����Ҫ��ص� SSDT Shadow �е�ϵͳ���õĹ��ܺš�


Arguments:

	None.


Return Value:

	TRUE if successful, and FALSE if not.


Comments:

	Ŀǰ���ø���ϵͳ�汾����Ӳ����ķ��������ַ������ã���Ҫ�Ľ���


Author:

	Fypher

--*/
{
	BOOLEAN bRet = FALSE;

	PAGED_CODE();

	switch (g_ulOsVersion) {
		case OS_VERSION_2000:
			g_ulShadowId_NtUserSetWindowsHookEx = 0x212;
			bRet = TRUE;
			break;

		case OS_VERSION_XP:
			g_ulShadowId_NtUserSetWindowsHookEx = 0x225;
			bRet = TRUE;
			break;

		case OS_VERSION_SERVER_2003:
			g_ulShadowId_NtUserSetWindowsHookEx = 0x221;
			bRet = TRUE;
			break;

		case OS_VERSION_VISTA:
		case OS_VERSION_VISTA_SP1:
		case OS_VERSION_VISTA_SP2:
			g_ulShadowId_NtUserSetWindowsHookEx = 0x23d;
			bRet = TRUE;
			break;

		case OS_VERSION_WIN7:
		case OS_VERSION_WIN7_SP1:
			g_ulShadowId_NtUserSetWindowsHookEx = 0x8c;
			bRet = TRUE;
			break;

		default:
			break;
	}

	return bRet;
}

PRTL_PROCESS_MODULES GetSystemModules()
/*++

Routine Description:

	��ȡϵͳģ����Ϣ


Arguments:

	None.


Return Value:

	�ɹ��򷵻�ϵͳģ����Ϣ��ʧ�ܷ��� NULL ��


Comments:

	�ú������ص�ϵͳģ����Ϣʹ����Ϻ���Ҫ�ɵ����߽����ͷţ�ExFreePool����


Author:

	Fypher

--*/
{
	PRTL_PROCESS_MODULES pSysMods = NULL;
	ULONG ulSize = 512;
	NTSTATUS status;

	PAGED_CODE();

	while ( TRUE )
	{
		pSysMods = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(PagedPool, ulSize, ALLOC_TAG);
		if ( !pSysMods )
			return NULL;

		status = ZwQuerySystemInformation(SystemModuleInformation, pSysMods, ulSize, &ulSize);
		if ( status != STATUS_INFO_LENGTH_MISMATCH )
			break;

		ExFreePool(pSysMods);
	}

	if ( !NT_SUCCESS(status) ) {
		ExFreePool(pSysMods);
		pSysMods = NULL;
	}

	return pSysMods;
}


BOOLEAN GetSysModInfoByName(IN char * strModName, OUT PRTL_PROCESS_MODULE_INFORMATION pSysModInfo)
/*++

Routine Description:

	��ȡָ�����Ƶ�ģ����Ϣ


Arguments:

	strModName - ָ��ģ�������

	pSysModInfo - ����ָ����ģ����Ϣ


Return Value:

	TRUE if successful, and FALSE if not.


Author:

	Fypher

--*/
{
	PRTL_PROCESS_MODULES pSysMods;
	PRTL_PROCESS_MODULE_INFORMATION pModInfo;
	ULONG i, ulModNum;
	BOOLEAN bRet = FALSE;

	PAGED_CODE();

	pSysMods = GetSystemModules();
	if (!pSysMods)
		return FALSE;

	ulModNum = pSysMods->NumberOfModules;
	pModInfo = pSysMods->Modules;

	for (i = 0; i < ulModNum; ++i) {
		if (pModInfo->FullPathName) {
			char * strFileName = strrchr(pModInfo->FullPathName, '\\');
			if (strFileName)
				strFileName++;
			else
				strFileName = pModInfo->FullPathName;

			if (!_stricmp(strModName, strFileName)) {
				RtlCopyMemory(pSysModInfo, pModInfo, sizeof(RTL_PROCESS_MODULE_INFORMATION));
				bRet = TRUE;
				break;
			}
		}
		pModInfo += 1;
	}
	ExFreePool(pSysMods);

	return bRet;
}


PSYSTEM_PROCESS_INFORMATION GetSystemProcesses()
/*++

Routine Description:

	��ȡϵͳ�еĽ�����Ϣ


Arguments:

	None.


Return Value:

	�ɹ��򷵻�ϵͳ�еĽ�����Ϣ��ʧ�ܷ��� NULL��


Comments:

	�ú������صĽ�����Ϣʹ����Ϻ���Ҫ�ɵ����߽����ͷţ�ExFreePool����


Author:

	Fypher

--*/
{
	ULONG ulSize = 512;
	PSYSTEM_PROCESS_INFORMATION pSysProcInfo;
	NTSTATUS status;

	PAGED_CODE();

	while ( TRUE )
	{
		pSysProcInfo = (PSYSTEM_PROCESS_INFORMATION)ExAllocatePoolWithTag(PagedPool, ulSize, ALLOC_TAG);
		if ( !pSysProcInfo )
			return NULL;

		status = ZwQuerySystemInformation(SystemProcessesAndThreadsInformation, pSysProcInfo, ulSize, &ulSize);
		if ( status != STATUS_INFO_LENGTH_MISMATCH )
			break;
		ExFreePool(pSysProcInfo);
	}

	if ( !NT_SUCCESS(status) ) {
		ExFreePool(pSysProcInfo);
		return NULL;
	}

	return pSysProcInfo;
}

//HANDLE GetProcIdByName(WCHAR * wcsProcName)
//{
//	HANDLE Pid = (HANDLE)0;
//	PSYSTEM_PROCESS_INFORMATION pSysProcInfo, pBuf;
//	NTSTATUS status;
//	UNICODE_STRING uniProcName;
//
//	PAGED_CODE();
//
//	pBuf = pSysProcInfo = GetSystemProcesses();
//	if (!pBuf)
//		return 0;
//
//	RtlInitUnicodeString(&uniProcName, wcsProcName);
//
//	while (TRUE) {
//		if (RtlEqualUnicodeString(&uniProcName, &pSysProcInfo->ImageName, TRUE)) {
//			Pid = pSysProcInfo->UniqueProcessId;
//			break;
//		}
//
//		if (pSysProcInfo->NextEntryOffset == 0)
//			break;
//
//		pSysProcInfo = (PSYSTEM_PROCESS_INFORMATION)( (PCHAR)pSysProcInfo + pSysProcInfo->NextEntryOffset );
//	}
//	ExFreePool(pBuf);
//
//	return Pid;
//}


//ULONG GetThreadNumOfProcess(HANDLE Pid)
//{
//	PSYSTEM_PROCESS_INFORMATION pSysProcInfo, pBuf;
//	NTSTATUS status;
//	ULONG ulThreadNum = 0;
//
//	PAGED_CODE();
//
//	pBuf = pSysProcInfo = GetSystemProcesses();
//	if (!pBuf)
//		return 0;
//
//	while (TRUE) {
//		if (Pid == pSysProcInfo->UniqueProcessId) {
//			ulThreadNum = pSysProcInfo->NumberOfThreads;
//			break;
//		}
//
//		if (pSysProcInfo->NextEntryOffset == 0)
//			break;
//
//		pSysProcInfo = (PSYSTEM_PROCESS_INFORMATION)( (PCHAR)pSysProcInfo + pSysProcInfo->NextEntryOffset );
//	}
//	ExFreePool(pBuf);
//
//	return ulThreadNum;
//}


ULONG GetHandleCountOfProcess(IN HANDLE Pid)
/*++

Routine Description:

	��ȡ�ض�������ӵ�еľ����


Arguments:

	Pid - ָ�����̵�Pid


Return Value:

	TRUE if successful, and FALSE if not.


Comments:

	�ú����������ж��ض������Ƿ�������ϡ�


Author:

	Fypher

--*/
{
	PSYSTEM_PROCESS_INFORMATION pSysProcInfo, pBuf;
	NTSTATUS status;
	ULONG ulHandleCount = 0;

	PAGED_CODE();

	pBuf = pSysProcInfo = GetSystemProcesses();
	if (!pBuf)
		return 0;

	while (TRUE) {
		if (Pid == pSysProcInfo->UniqueProcessId) {
			ulHandleCount = pSysProcInfo->HandleCount;
			break;
		}

		if (pSysProcInfo->NextEntryOffset == 0)
			break;

		pSysProcInfo = (PSYSTEM_PROCESS_INFORMATION)( (PCHAR)pSysProcInfo + pSysProcInfo->NextEntryOffset );
	}
	ExFreePool(pBuf);

	return ulHandleCount;
}



PVOID GetSectionObjectByName(IN WCHAR * wcsObjName)
/*++

Routine Description:

	��ȡָ�����Ƶ�Section����


Arguments:

	wcsObjName - ָ��Section���������


Return Value:

	�ɹ��򷵻�Section Object��ʧ�ܷ��� NULL


Comments:

	�ú������ص�Section������Ҫ�ɵ����߸���ObDereferenceObject��


Author:

	Fypher

--*/
{
	UNICODE_STRING usObjName;
	OBJECT_ATTRIBUTES oa;
	NTSTATUS NtStatus;
	HANDLE hSection;
	PVOID pSecObj;

	PAGED_CODE();

	RtlInitUnicodeString(&usObjName, wcsObjName);
	InitializeObjectAttributes(	&oa,
								&usObjName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL
							);
	NtStatus = ZwOpenSection(&hSection, 0, &oa);
	if (!NT_SUCCESS(NtStatus))
		return NULL;

	NtStatus = ObReferenceObjectByHandle(hSection, 0, NULL, KernelMode, &pSecObj, NULL);
	ZwClose(hSection);
	if (!NT_SUCCESS(NtStatus))
		return NULL;

	return pSecObj;
}


ULONG MyGetProcAddress(IN ULONG ulModBase, IN CHAR * strRoutineName)
/*++

Routine Description:

	��ָ��ģ���л�ȡָ���ĵ���������ַ


Arguments:

	ulModBase - ָ��ģ��ļ��ص�ַ

	strRoutineName - ָ������������


Return Value:

	�ɹ��򷵻غ�����ַ��ʧ�ܷ��� NULL


Comments:

	�ú���Ŀǰֻ�����64λPEģ�鹤������Ҫ���ơ�


Author:

	Fypher

--*/
{
	PIMAGE_DOS_HEADER pDosHdr;
	PIMAGE_OPTIONAL_HEADER pOptHdr;
	PIMAGE_EXPORT_DIRECTORY pExportTable;

	PULONG pulFuncAddrArray;
	PULONG pulFuncNameArray;
	PUSHORT pusFuncOrdinalsArray;

	ULONG ulIndex;
	PCHAR strFuncName;

	ULONG i;

	PAGED_CODE();

	pDosHdr = (PIMAGE_DOS_HEADER)ulModBase;
	pOptHdr = (PIMAGE_OPTIONAL_HEADER)(ulModBase + pDosHdr->e_lfanew + 4 + sizeof(IMAGE_FILE_HEADER)/*24*/);
	pExportTable = (PIMAGE_EXPORT_DIRECTORY)(ulModBase + pOptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);


    pulFuncAddrArray = (PULONG)(ulModBase + pExportTable->AddressOfFunctions);

    pulFuncNameArray = (PULONG)(ulModBase + pExportTable->AddressOfNames);

    pusFuncOrdinalsArray = (PUSHORT)(ulModBase + pExportTable->AddressOfNameOrdinals);

    for (i = 0; i < pExportTable->NumberOfNames; i++) {
		ulIndex = pusFuncOrdinalsArray[i] + pExportTable->Base - 1;
		strFuncName = (PCHAR)(ulModBase + pulFuncNameArray[i]);
		if (!_stricmp(strFuncName, strRoutineName))
			return ulModBase + pulFuncAddrArray[ulIndex];
    }

	return 0;
}

BOOLEAN IsCurrentProcRProcess()
/*++

Routine Description:

	��ǰ�����Ƿ��ǿͻ���


Arguments:

	None.


Return Value:

	TRUE if successful, and FALSE if not.


Comments:

	�ú����������ұ���������Ŀǰû��ʵ�־��幦�ܡ�


Author:

	Fypher

--*/
{
	PAGED_CODE();

 	/*
 	PUNICODE_STRING pusCurProcName = GetProcNameByEproc(PsGetCurrentProcess());
	BOOLEAN bRet = IsUniStrEndWithWcs(pusCurProcName, DRV_NAME);

	if (pusCurProcName)
		ExFreePool(pusCurProcName);

	return bRet;
	*/
	return TRUE;		// always return TRUE
}

PUNICODE_STRING GetProcNameByEproc(IN PEPROCESS pEproc)
/*++

Routine Description:

	��ȡָ�����̵�����������


Arguments:

	pEproc - ָ�����̵�EPROCESS��ַ


Return Value:

	�ɹ��򷵻ؽ�������ʧ�ܷ���NULL


Comments:

	�ú������صĽ��������ɵ��������ͷţ�ExFreePool����


Author:

	Fypher

--*/
{
	NTSTATUS NtStatus;
	HANDLE hProc = NULL;
	PBYTE pBuf = NULL;
	ULONG ulSize = 32;

	PAGED_CODE();

	//
	// 1. pEproc --> handle
	//
	NtStatus = ObOpenObjectByPointer(	pEproc,
										OBJ_KERNEL_HANDLE,
										NULL,
										0,
										NULL,
										KernelMode,
										&hProc
									);

	if (!NT_SUCCESS(NtStatus))
		return NULL;

	//
	// 2. ZwQueryInformationProcess
	//
	while ( TRUE )
	{
		pBuf = ExAllocatePoolWithTag(NonPagedPool, ulSize, ALLOC_TAG);
		if ( !pBuf ) {
			ZwClose(hProc);
			return NULL;
		}

		NtStatus = ZwQueryInformationProcess(	hProc,
												ProcessImageFileName,
												pBuf,
												ulSize,
												&ulSize);
		if ( NtStatus != STATUS_INFO_LENGTH_MISMATCH )
			break;

		ExFreePool(pBuf);
	}

	ZwClose(hProc);

	if ( !NT_SUCCESS(NtStatus) ) {
		ExFreePool(pBuf);
		return NULL;
	}

	//
	// 4. over
	//
	return (PUNICODE_STRING)pBuf;
}


BOOLEAN IsUniStrEndWithWcs(IN PUNICODE_STRING puniXXX, IN PWSTR wcsSubXXX)
/*++

Routine Description:

	�ж�ĳUnicodeString�Ƿ���ĳ��PWSTR��β


Arguments:

	puniXXX - ...
	wcsSubXXX - ...


Return Value:

	TRUE or FALSE


Comments:

	TRUE if successful, and FALSE if not.


Author:

	Fypher

--*/
{
	USHORT usUniXXXLen, usWcsXXXLen;
	PWSTR wcsPos;

	//PAGED_CODE();

	if (!puniXXX || !puniXXX->Buffer || !puniXXX->Length/* || !wcsSubXXX*/)
		return FALSE;

	usUniXXXLen = puniXXX->Length / sizeof(WCHAR);
	usWcsXXXLen = (USHORT)wcslen(wcsSubXXX);

	if (usWcsXXXLen > usUniXXXLen || !usWcsXXXLen)
		return FALSE;

	wcsPos = puniXXX->Buffer + (usUniXXXLen - usWcsXXXLen);

	if (_wcsnicmp(wcsPos, wcsSubXXX, usWcsXXXLen)) {
		return FALSE;
	}

	return TRUE;
}


/*
#pragma PAGEDCODE
BOOLEAN IsWcsEndWithWcs(PWSTR pwcsXXX, PWSTR wcsSubXXX, ULONG ulXXXLen) {
	//USHORT usUniXXXLen, usWcsXXXLen;
	ULONG ulSubXXXLen;
	PWSTR wcsPos;

	if (!pwcsXXX || !ulXXXLen)
		return FALSE;

	ulSubXXXLen = wcslen(wcsSubXXX);

	if (ulSubXXXLen > ulXXXLen || !ulSubXXXLen)
		return FALSE;

#ifdef DBG
	if (!pwcsXXX[ulXXXLen - 1]) {
		KdPrintEx((DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "in IsWcsEndWithWcs, 0-term: %ws\r\n", pwcsXXX));
	}
#endif
	wcsPos = pwcsXXX + (ulXXXLen - ulSubXXXLen);

	if (_wcsnicmp(wcsPos, wcsSubXXX, ulSubXXXLen)) {
		return FALSE;
	}

	return TRUE;
}
*/



VOID MyProbeForWrite(IN PVOID pAddr,IN ULONG ulLen,IN ULONG ulAlignment,IN ULONG ulMemType)
/*++

Routine Description:

	�ж�ĳ�ڴ��ַ�Ƿ��д


Arguments:

	pAddr - �ڴ��ַ

	ulLen - �ڴ泤��

	ulAlignment - ����ֵ

	ulMemType - MEM_TYPE_KERNELMODE �� MEM_TYPE_USERMODE


Return Value:

	None.


Comments:

	��ָ���ڴ治��дʱ�����׳��쳣�����÷�Ӧ�ý��˺���������try���С�


Author:

	fypher

--*/

{
	// PAGED_CODE();

	PVOID pBegPage, pEndPage;

	if (ulLen == 0)
		return;

	if (((ULONG_PTR)pAddr & (ulAlignment - 1)) != 0) {
		ExRaiseDatatypeMisalignment();
		return;
	}

	if (((ULONG_PTR)pAddr + ulLen) < (ULONG_PTR)pAddr) {
		ExRaiseAccessViolation();
		return;
	}

	// pBegPage = (PVOID)(((ULONG_PTR)pAddr) & (~(PAGE_SIZE - 1)));
	// pEndPage = (PVOID)(((ULONG_PTR)pAddr + ulLen) & (~(PAGE_SIZE - 1)))
	pBegPage = (PVOID)(((ULONG_PTR)pAddr / PAGE_SIZE) * PAGE_SIZE);
	pEndPage = (PVOID)((((ULONG_PTR)pAddr + ulLen) / PAGE_SIZE) * PAGE_SIZE);

	if (pAddr >= MmSystemRangeStart)			// kernel space
	{
		if (!(ulMemType & MEM_TYPE_KERNELMODE)) {
			ExRaiseAccessViolation();
			return;
		}

		while (pBegPage <= pEndPage) {
			if (!MmIsAddressValid(pBegPage)) {
				ExRaiseAccessViolation();
				return;
			}
			pBegPage = (PVOID)((ULONG_PTR)pBegPage + PAGE_SIZE);
		}
	}
	else										// user space
	{
		if (!(ulMemType & MEM_TYPE_USERMODE)) {
			ExRaiseAccessViolation();
			return;
		}

		while (pBegPage <= pEndPage) {
			BYTE btTmp = *(PBYTE)pBegPage;
			*(PBYTE)pBegPage = btTmp;
			pBegPage = (PVOID)((ULONG_PTR)pBegPage + PAGE_SIZE);
		}
	}
}




VOID MyProbeForRead(IN PVOID pAddr, IN ULONG ulLen, IN ULONG ulAlignment, IN ULONG ulMemType)
/*++

Routine Description:

	�ж�ĳ�ڴ��ַ�Ƿ�ɶ�


Arguments:

	pAddr - �ڴ��ַ

	ulLen - �ڴ泤��

	ulAlignment - ����ֵ

	ulMemType - MEM_TYPE_KERNELMODE �� MEM_TYPE_USERMODE


Return Value:

	None.


Comments:

	��ָ���ڴ治�ɶ�ʱ�����׳��쳣�����÷�Ӧ�ý��˺���������try���С�


Author:

	fypher

--*/
{
	// PAGED_CODE();

	PVOID pBegPage, pEndPage;

	if (ulLen == 0)
		return;

	if (((ULONG_PTR)pAddr & (ulAlignment - 1)) != 0) {
		ExRaiseDatatypeMisalignment();
		return;
	}

	if (((ULONG_PTR)pAddr + ulLen) < (ULONG_PTR)pAddr) {
		ExRaiseAccessViolation();
		return;
	}

	// pBegPage = (PVOID)(((ULONG_PTR)pAddr) & (~(PAGE_SIZE - 1)));
	// pEndPage = (PVOID)(((ULONG_PTR)pAddr + ulLen) & (~(PAGE_SIZE - 1)))
	pBegPage = (PVOID)(((ULONG_PTR)pAddr / PAGE_SIZE) * PAGE_SIZE);
	pEndPage = (PVOID)((((ULONG_PTR)pAddr + ulLen) / PAGE_SIZE) * PAGE_SIZE);

	if (pAddr >= MmSystemRangeStart)			// kernel space
	{
		if (!(ulMemType & MEM_TYPE_KERNELMODE)) {
			ExRaiseAccessViolation();
			return;
		}

		while (pBegPage <= pEndPage) {
			if (!MmIsAddressValid(pBegPage)) {
				ExRaiseAccessViolation();
				return;
			}
			pBegPage = (PVOID)((ULONG_PTR)pBegPage + PAGE_SIZE);
		}
	}
	else										// user space
	{
		if (!(ulMemType & MEM_TYPE_USERMODE)) {
			ExRaiseAccessViolation();
			return;
		}

		while (pBegPage <= pEndPage) {
			BYTE btTmp = *(PBYTE)pBegPage;
			pBegPage = (PVOID)((ULONG_PTR)pBegPage + PAGE_SIZE);
		}
	}
}


//#pragma PAGEDCODE
//BOOLEAN GetHash(PBYTE pBuffer, ULONG ulSize, PULONG pulHash)
//{
//	PAGED_CODE();
//
//	ULONG i;
//	ULONG ulStep = ulSize / sizeof(ULONG);
//	if (ulStep == 0)
//		return FALSE;
//
//	*pulHash = 0;
//
//	for (i = 0; i <sizeof(ULONG); i++) {
//
//		*pulHash |= ((*pBuffer & 1) << i);
//
//		pBuffer += ulStep;
//	}
//
//	KdPrintEx((DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "GetHash: %p\r\n", *pulHash));
//	return TRUE;
//}


_declspec (naked) void __stdcall MyMemCpy_NoPaged(PVOID pDst, PVOID pSrc, ULONG count) {
	__asm {
		pushfd;
		pushad;
		cld;
		mov edi, [esp + 0x4 + 0x24];
		mov esi, [esp + 0x8 + 0x24];
		mov ecx, [esp + 0xC + 0x24];
		mov eax, CR0;
		mov ebx, eax;
		and eax, 0xFFFEFFFF;
		cli;
		mov CR0, eax;
		rep movsb;
		mov CR0, ebx;
		sti;
		popad;
		popfd;
		retn 0xC;
	}
}



