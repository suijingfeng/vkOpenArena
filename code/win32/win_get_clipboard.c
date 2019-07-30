#include <windows.h>
#include "../client/client.h"
#include "../qcommon/qcommon.h"
/*
================
Sys_GetClipboardData

================
*/
char * Sys_GetClipboardData(void)
{
	char *data = NULL;

	if (OpenClipboard(NULL) != 0)
	{
		HANDLE hClipboardData;

		if ((hClipboardData = GetClipboardData(CF_TEXT)) != 0)
		{
			char *cliptext;
			if ((cliptext = (char*)GlobalLock(hClipboardData)) != 0) {
				data = (char*)Z_Malloc(GlobalSize(hClipboardData) + 1);
				Q_strncpyz(data, cliptext, GlobalSize(hClipboardData));
				GlobalUnlock(hClipboardData);

				strtok(data, "\n\r\b");
			}
		}
		CloseClipboard();
	}
	return data;
}