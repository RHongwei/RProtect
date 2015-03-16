#ifndef _QUERY_STATUS_H_
#define _QUERY_STATUS_H_

#define NODE_TYPE_WHITE		0
#define NODE_TYPE_BLACK		1


#define IOCTL_GET_MAJOR_PROTECTED_INFO					\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0xa03, 				\
	METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_SET_MAJOR_PROTECTED						\
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0xa04, 				\
	METHOD_BUFFERED, FILE_WRITE_DATA)

typedef struct _USER_MAJOR_PROTECTED_INFO_ {
	ULONG ulCrimeType;
	ULONG ulIsProtected;		// not BOOLEAN, sizeof(BOOLEAN) may be different in ring0/ring3
} USER_MAJOR_PROTECTED_INFO, *PUSER_MAJOR_PROTECTED_INFO, *PPUSER_MAJOR_PROTECTED_INFO;


BOOL IsMonOn(ULONG ulCrimeType);

// ���º���ȫ����XXXIntelnal���߳����������⿨������
BOOL SetMon(ULONG ulCrimeType, BOOL bIsOn);
DWORD WINAPI SetMonInternal(PUSER_MAJOR_PROTECTED_INFO pUsrMjProInfo);

PTSTR QueryWhiteBlackList(ULONG ulCrimeType, ULONG ulNodeType);
DWORD WINAPI QueryWhiteBlackListInternal(PUSER_MAJOR_PROTECTED_INFO pUsrMjProInfo);

BOOLEAN AddToWhiteBlackList(PTSTR strNodeName, ULONG ulCrimeType, ULONG ulNodeType);
DWORD WINAPI AddToWhiteBlackListInternal(PUSER_MAJOR_PROTECTED_INFO pUsrMjProInfo);


#endif
