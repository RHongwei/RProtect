#include "Common.h"
#include "Forms.h"
#include "EventHandler.h"

HINSTANCE g_hInst = NULL;
HANDLE g_hDev = NULL;
BOOL g_bOver = FALSE;

int WINAPI _tWinMain(HINSTANCE hinstExe, HINSTANCE, PTSTR pszCmdLine, int) {

	g_hInst = hinstExe;

	MessageBox(NULL, TIPS, TEXT("RProtect"), MB_OK);

	g_hDev = CreateFile(TEXT("\\\\.\\RProtect"),
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL
					);

	if (g_hDev == INVALID_HANDLE_VALUE) {
		MessageBox(	NULL,
					TEXT("��������δ���أ�\r\n")
					TEXT("������Ϊ���Ʒ��ʾ�棬��û�а�װ�����Ĺ��ܣ���ʹ�õ���������������������������б�����\r\n")
					TEXT("���⣺��������������û������ǩ�����뿪��F8��������ǩ�����ٽ��С�\r\n"),
					TEXT("error"),
					MB_OK
				);
		return 0;
	}


	g_hEvtUIReady = CreateEvent(NULL, FALSE, FALSE, NULL);

	HANDLE hThread = BEGINTHREADEX(	NULL,
									0,
									EventHandler,
									NULL,
									0,
									NULL
									);
	if (!hThread)
		MessageBox(NULL, TEXT("�޷�����ͨѶ�̣߳�"), TEXT("err"), MB_OK);

	DialogBox(hinstExe, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlg_Proc);

	// terminate communication thread
	g_bOver = TRUE;
	WaitForSingleObject(hThread, INFINITE);
	//TerminateThread(hThread, 0);

	CloseHandle(hThread);
	CloseHandle(g_hDev);
	return 0;
}

//int WINAPI _tWinMain(HINSTANCE hinstExe, HINSTANCE, PTSTR pszCmdLine, int) {
//
//	MessageBox(	NULL,
//				TEXT("ע�⣺\r\n")
//				TEXT("������Ϊ��ʾ�汾����û�а�װ�����Ĺ��ܣ���ʹ��\r\n")
//				TEXT("����������������������������б�����\r\n"),
//				TEXT("RProtect"),
//				MB_OK
//			);
//	DialogBox(hinstExe, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlg_Proc);
//
//
//	return 0;
//}