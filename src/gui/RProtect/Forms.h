#ifndef _FORMS_H_
#define _FORMS_H_


#define TIPS																											\
	TEXT("��ӭ���� RProtec �������������ע�����\r\n\r\n")															\
	TEXT("1�� �����ֻ�ڸɾ��� Windows 7 Professional x64 ϵͳ�ϲ��Թ��������������²���֤�������У�\r\n\r\n")			\
	TEXT("2�� �����������ʾ���о����ݲ�������װж�ع��ܡ�ʹ�ñ����ʱ��ѡ�õ��������߼������������г���ж�ر�����")	\
	TEXT("ʱ��رձ������ʹ�õ���������ж��������\r\n\r\n")															\
	TEXT("3�� �����ʹ�ù����в�������κ���־�ļ���ע�����Ϣ�Ȳ������������Ҳ���ᱣ���û�ʹ�ù����е�������Ϣ����")	\
	TEXT("��ڰ������ȡ������ʹ�ã�\r\n\r\n")																			\
	TEXT("4�� ������Ľ���Ŀǰ�ǳ��������������׼����\r\n\r\n")


#define HANDLE_DLGMSG(hwnd, message, fn)                 \
   case (message): return (SetDlgMsgResult(hwnd, uMsg,     \
   HANDLE_##message((hwnd), (wParam), (lParam), (fn))))

extern HWND g_hMain;

extern HWND g_hTab1_Setup;
extern HWND g_hTab2_Proc;
extern HWND g_hTab3_File;
extern HWND g_hTab4_Reg;
extern HWND g_hTab5_Sys;

extern HANDLE g_hEvtUIReady;

VOID DlgPageInit();
VOID DlgPageOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
INT_PTR WINAPI DlgPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI MainDlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR WINAPI EventDlg_Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
