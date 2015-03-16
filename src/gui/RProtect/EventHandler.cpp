#include "Common.h"
#include "RProtect.h"
#include "Forms.h"
#include "EventHandler.h"

USEREVENTDATA g_UsrEvtData;

DWORD WINAPI EventHandler(LPVOID lpParameter) {

	WaitForSingleObject(g_hEvtUIReady, INFINITE);	// ���������������Ƿ�ɹ�

	while (!g_bOver) {
		JUDGMENTDATA JudgData;
		DWORD dwRet;

		BOOL bRet = DeviceIoControl(g_hDev,
									IOCTL_GET_EVENT,
									NULL,
									0,
									&g_UsrEvtData,
									sizeof(USEREVENTDATA),
									&dwRet,
									NULL
								);
		if (!bRet) {
			continue;
		}
		DeviceNameToDosName(g_UsrEvtData.wzCriminal);
		DeviceNameToDosName(g_UsrEvtData.wzVictim);

		JudgData.ulEvtData = g_UsrEvtData.ulEvtData;
		// +++
		JudgData.ulJudgment = (ULONG)DialogBox(g_hInst, MAKEINTRESOURCE(IDD_EVENT), NULL, EventDlg_Proc);
		// ---

		DeviceIoControl(	g_hDev,
							IOCTL_GIVE_JUDGMENT,
							&JudgData,
							sizeof(JUDGMENTDATA),
							NULL,
							0,
							&dwRet,
							NULL
						);
	}
	return 0;
}

VOID DeviceNameToDosName(PTSTR tcsDevName)
{

	if (!tcsDevName)
		return;

	TCHAR tzTemp[512] = TEXT("");

	if (GetLogicalDriveStrings(512 - 1, tzTemp))
	{
		TCHAR tzName[MAX_PATH];
		TCHAR tzDrive[3] = TEXT(" :");
		BOOL bFound = FALSE;
		TCHAR* p = tzTemp;

		do
		{
			// Copy the drive letter to the template string
			*tzDrive = *p;

			// Look up each device name
			if (QueryDosDevice(tzDrive, tzName, 512))
            {
				ULONG ulNameLen = (ULONG)_tcslen(tzName);
				if (_tcslen(tzName) < MAX_PATH)
				{
					bFound = (_tcsnicmp(tcsDevName, tzName, ulNameLen) == 0);
					if (bFound)
					{
						// Reconstruct pszFilename using szTempFile
						// Replace device path with DOS path
						TCHAR tzTempFile[MAX_PATH];
						_stprintf_s(tzTempFile, MAX_PATH, TEXT("%s%s"), tzDrive, tcsDevName + ulNameLen);
						_tcscpy_s(tcsDevName, MAX_PATH, tzTempFile);
					}
				}
			}
			while (*p++);
		} while (!bFound && *p); // end of string
	}
}


VOID GetDetail(ULONG ulCrimeType, PTSTR tcsMsg, ULONG usMsgLen)
{
	switch (ulCrimeType)  {
		////////////////////////////////////////////////////////////
		//
		//		CRIME_FILE
		//
		case CRIME_MINOR_NtCreateFile:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�����ܱ������ļ���%s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtOpenFile:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�����ܱ������ļ���%s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtDeleteFile:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼɾ���ܱ������ļ���%s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtSetInformationFile:
			if (FILE_INFORMATION_CLASS(g_UsrEvtData.ulExtraInfo) == FileLinkInformation)
			{
				_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�����ܱ����ļ� %s ��Ӳ����"),
							g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
						);
			}
			else if (FILE_INFORMATION_CLASS(g_UsrEvtData.ulExtraInfo) == FileRenameInformation)
			{
				_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�����ܱ����ļ� %s ���ļ���"),
							g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
						);
			}
			else if (FILE_INFORMATION_CLASS(g_UsrEvtData.ulExtraInfo) == FileShortNameInformation)
			{
				_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�����ܱ����ļ� %s �Ķ���"),
							g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
						);
			}
			break;
		////////////////////////////////////////////////////////////
		//
		//		CRIME_REG
		//

		////////////////////////////////////////////////////////////
		//
		//		CRIME_PROC
		//
		//case CRIME_MINOR_NtCreateSection:
		case CRIME_MINOR_NtCreateUserProcess:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�������̣�%s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtCreateThread:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�� %s �д����߳�"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtSuspendThread:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ���� %s ���߳�"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtSuspendProcess:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ���� %s �Ľ���"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtGetContextThread:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��ȡ %s �е��̼߳Ĵ�������"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtSetContextThread:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��д %s �е��̼߳Ĵ�������"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtTerminateProcess:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��ֹ���̣�%s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtTerminateThread:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��ֹ %s �е��߳�"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtReadVirtualMemory:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��ȡ %s �е��ڴ��ַ��0x%p"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim, g_UsrEvtData.ulExtraInfo
					);
			break;
		case CRIME_MINOR_NtWriteVirtualMemory:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��д %s �е��ڴ��ַ��0x%p"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim, g_UsrEvtData.ulExtraInfo
					);
			break;
		case CRIME_MINOR_NtProtectVirtualMemory:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�ı� %s �У�0x%p ����ҳ�����Ȩ��"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim, g_UsrEvtData.ulExtraInfo
					);
			break;
		case CRIME_MINOR_NtOpenThread:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��ȡ %s �е��߳̾��"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtOpenProcess:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��ȡ %s �еĽ��̾��"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtAssignProcessToJobObject:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ�� %s ���Ϊ��ҵ"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		////////////////////////////////////////////////////////////
		//
		//		CRIME_SYS
		//
		case CRIME_MINOR_NtLoadDriver:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ���������� %s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtUnloadDriver:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼж�������� %s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtSetSystemInformation:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ���������� %s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtOpenSection:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ���������ڴ����"),
						g_UsrEvtData.wzCriminal/*, g_UsrEvtData.wzVictim*/
					);
			break;
		case CRIME_MINOR_NtCreateSymbolicLinkObject:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ������������ %s"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);
			break;
		case CRIME_MINOR_NtSystemDebugControl:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ����Σ�պ�����ZwSystemDebugControl"),
						g_UsrEvtData.wzCriminal/*, g_UsrEvtData.wzVictim*/
					);
			break;
		case CRIME_MINOR_NtUserSetWindowsHookEx:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ����xx���ӣ���������ģ�飺%s�����Ӻ�����ַ��0x%p"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim,
						g_UsrEvtData.ulExtraInfo
					);
			break;
		case CRIME_MINOR_NtDuplicateObject:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("%s ��ͼ��ȡ %s �еľ��"),
						g_UsrEvtData.wzCriminal, g_UsrEvtData.wzVictim
					);			break;
		////////////////////////////////////////////////////////////
		//
		//		CRIME_UNKNOWN
		//
		default:
			_stprintf_s(tcsMsg, usMsgLen, TEXT("δ֪"));
			break;
	}
}
