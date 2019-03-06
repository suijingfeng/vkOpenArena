/*
===========================================================================
Copyright (C) 2006 Tony J. White (tjw@tjw.org)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "client.h"

#ifndef CURL_MAX_READ_SIZE
	#define CURL_MAX_READ_SIZE 524288
#endif

qboolean CL_cURL_Init()
{
	clc.cURLEnabled = qtrue;
	return qtrue;
}

/*
=================
CL_cURL_Shutdown
=================
*/
void CL_cURL_Shutdown( void )
{
	CL_cURL_Cleanup();
}

void CL_cURL_Cleanup(void)
{
	if(clc.downloadCURLM) {
		CURLMcode result;

		if(clc.downloadCURL) {
			result = qcurl_multi_remove_handle(clc.downloadCURLM,
				clc.downloadCURL);
			if(result != CURLM_OK) {
				Com_DPrintf("qcurl_multi_remove_handle failed: %s\n", qcurl_multi_strerror(result));
			}
			qcurl_easy_cleanup(clc.downloadCURL);
		}
		result = qcurl_multi_cleanup(clc.downloadCURLM);
		if(result != CURLM_OK) {
			Com_DPrintf("CL_cURL_Cleanup: qcurl_multi_cleanup failed: %s\n", qcurl_multi_strerror(result));
		}
		clc.downloadCURLM = NULL;
		clc.downloadCURL = NULL;
	}
	else if(clc.downloadCURL) {
		qcurl_easy_cleanup(clc.downloadCURL);
		clc.downloadCURL = NULL;
	}
}

static int CL_cURL_CallbackProgress( void *dummy, double dltotal, double dlnow,
	double ultotal, double ulnow )
{
	clc.downloadSize = (int)dltotal;
	Cvar_SetValue( "cl_downloadSize", clc.downloadSize );
	clc.downloadCount = (int)dlnow;
	Cvar_SetValue( "cl_downloadCount", clc.downloadCount );
	return 0;
}

static size_t CL_cURL_CallbackWrite(void *buffer, size_t size, size_t nmemb,
	void *stream)
{
	FS_Write( buffer, size*nmemb, ((fileHandle_t*)stream)[0] );
	return size*nmemb;
}

CURLcode qcurl_easy_setopt_warn(CURL *curl, CURLoption option, ...)
{
	CURLcode result;

	va_list argp;
	va_start(argp, option);

	if(option < CURLOPTTYPE_OBJECTPOINT) {
		long longValue = va_arg(argp, long);
		result = qcurl_easy_setopt(curl, option, longValue);
	} else if(option < CURLOPTTYPE_OFF_T) {
		void *pointerValue = va_arg(argp, void *);
		result = qcurl_easy_setopt(curl, option, pointerValue);
	} else {
		curl_off_t offsetValue = va_arg(argp, curl_off_t);
		result = qcurl_easy_setopt(curl, option, offsetValue);
	}

	if(result != CURLE_OK) {
		Com_DPrintf("qcurl_easy_setopt failed: %s\n", qcurl_easy_strerror(result));
	}
	va_end(argp);

	return result;
}

void CL_cURL_BeginDownload( const char *localName, const char *remoteURL )
{
	CURLMcode result;

	clc.cURLUsed = qtrue;
	Com_Printf("URL: %s\n", remoteURL);
	Com_DPrintf("***** CL_cURL_BeginDownload *****\n"
		"Localname: %s\n"
		"RemoteURL: %s\n"
		"****************************\n", localName, remoteURL);
	CL_cURL_Cleanup();
	Q_strncpyz(clc.downloadURL, remoteURL, sizeof(clc.downloadURL));
	Q_strncpyz(clc.downloadName, localName, sizeof(clc.downloadName));
	Com_sprintf(clc.downloadTempName, sizeof(clc.downloadTempName),
		"%s.tmp", localName);

	// Set so UI gets access to it
	Cvar_Set("cl_downloadName", localName);
	Cvar_Set("cl_downloadSize", "0");
	Cvar_Set("cl_downloadCount", "0");
	Cvar_SetValue("cl_downloadTime", cls.realtime);

	clc.downloadBlock = 0; // Starting new file
	clc.downloadCount = 0;

	clc.downloadCURL = qcurl_easy_init();
	if(!clc.downloadCURL) {
		Com_Error(ERR_DROP, "CL_cURL_BeginDownload: qcurl_easy_init() "
			"failed");
		return;
	}
	clc.download = FS_SV_FOpenFileWrite(clc.downloadTempName);
	if(!clc.download) {
		Com_Error(ERR_DROP, "CL_cURL_BeginDownload: failed to open "
			"%s for writing", clc.downloadTempName);
		return;
	}

	if(com_developer->integer)
		qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_VERBOSE, 1);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_URL, clc.downloadURL);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_TRANSFERTEXT, 0);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_REFERER, va("ioQ3://%s",
		NET_AdrToString(clc.serverAddress)));
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_USERAGENT, va("%s %s",
		Q3_VERSION, qcurl_version()));
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_WRITEFUNCTION,
		CL_cURL_CallbackWrite);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_WRITEDATA, &clc.download);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_NOPROGRESS, 0);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_PROGRESSFUNCTION,
		CL_cURL_CallbackProgress);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_PROGRESSDATA, NULL);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_FAILONERROR, 1);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_FOLLOWLOCATION, 1);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_MAXREDIRS, 5);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_PROTOCOLS,
		CURLPROTO_HTTP | CURLPROTO_HTTPS | CURLPROTO_FTP | CURLPROTO_FTPS);
	qcurl_easy_setopt_warn(clc.downloadCURL, CURLOPT_BUFFERSIZE, CURL_MAX_READ_SIZE);
	clc.downloadCURLM = qcurl_multi_init();	
	if(!clc.downloadCURLM) {
		qcurl_easy_cleanup(clc.downloadCURL);
		clc.downloadCURL = NULL;
		Com_Error(ERR_DROP, "CL_cURL_BeginDownload: qcurl_multi_init() "
			"failed");
		return;
	}
	result = qcurl_multi_add_handle(clc.downloadCURLM, clc.downloadCURL);
	if(result != CURLM_OK) {
		qcurl_easy_cleanup(clc.downloadCURL);
		clc.downloadCURL = NULL;
		Com_Error(ERR_DROP,"CL_cURL_BeginDownload: qcurl_multi_add_handle() failed: %s", qcurl_multi_strerror(result));
		return;
	}

	if(!(clc.sv_allowDownload & DLF_NO_DISCONNECT) &&
		!clc.cURLDisconnected) {

		CL_AddReliableCommand("disconnect", qtrue);
		CL_WritePacket();
		CL_WritePacket();
		CL_WritePacket();
		clc.cURLDisconnected = qtrue;
	}
}

void CL_cURL_PerformDownload(void)
{
	CURLMcode res;
	CURLMsg *msg;
	int c;
	int i = 0;

	res = qcurl_multi_perform(clc.downloadCURLM, &c);
	while(res == CURLM_CALL_MULTI_PERFORM && i < 100) {
		res = qcurl_multi_perform(clc.downloadCURLM, &c);
		i++;
	}
	if(res == CURLM_CALL_MULTI_PERFORM)
		return;
	msg = qcurl_multi_info_read(clc.downloadCURLM, &c);
	if(msg == NULL) {
		return;
	}
	FS_FCloseFile(clc.download);
	if(msg->msg == CURLMSG_DONE && msg->data.result == CURLE_OK) {
		FS_SV_Rename(clc.downloadTempName, clc.downloadName, qfalse);
		clc.downloadRestart = qtrue;
	}
	else {
		long code;

		qcurl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE,
			&code);	
		Com_Error(ERR_DROP, "Download Error: %s Code: %ld URL: %s",
			qcurl_easy_strerror(msg->data.result),
			code, clc.downloadURL);
	}

	CL_NextDownload();
}
