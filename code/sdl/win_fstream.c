#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include "../client/client.h"
#include "../qcommon/qcommon.h"

/*
========================================================================

BACKGROUND FILE STREAMING

========================================================================
*/

#if 1

void Sys_InitStreamThread(void) {
}

void Sys_ShutdownStreamThread(void) {
}

void Sys_BeginStreamedFile(fileHandle_t f, int readAhead) {
}

void Sys_EndStreamedFile(fileHandle_t f) {
}

int Sys_StreamedRead(void *buffer, int size, int count, fileHandle_t f) {
	return FS_Read(buffer, size * count, f);
}

void Sys_StreamSeek(fileHandle_t f, int offset, int origin) {
	FS_Seek(f, offset, origin);
}


#else

typedef struct {
	fileHandle_t	file;
	byte	*buffer;
	qboolean	eof;
	qboolean	active;
	int		bufferSize;
	int		streamPosition;	// next byte to be returned by Sys_StreamRead
	int		threadPosition;	// next byte to be read from file
} streamsIO_t;

typedef struct {
	HANDLE				threadHandle;
	int					threadId;
	CRITICAL_SECTION	crit;
	streamsIO_t			sIO[MAX_FILE_HANDLES];
} streamState_t;

streamState_t	stream;

/*
===============
Sys_StreamThread

A thread will be sitting in this loop forever
================
*/
void Sys_StreamThread(void) {
	int		buffer;
	int		count;
	int		readCount;
	int		bufferPoint;
	int		r, i;

	while (1) {
		Sleep(10);
		//		EnterCriticalSection (&stream.crit);

		for (i = 1; i < MAX_FILE_HANDLES; i++) {
			// if there is any space left in the buffer, fill it up
			if (stream.sIO[i].active && !stream.sIO[i].eof) {
				count = stream.sIO[i].bufferSize - (stream.sIO[i].threadPosition - stream.sIO[i].streamPosition);
				if (!count) {
					continue;
				}

				bufferPoint = stream.sIO[i].threadPosition % stream.sIO[i].bufferSize;
				buffer = stream.sIO[i].bufferSize - bufferPoint;
				readCount = buffer < count ? buffer : count;

				r = FS_Read(stream.sIO[i].buffer + bufferPoint, readCount, stream.sIO[i].file);
				stream.sIO[i].threadPosition += r;

				if (r != readCount) {
					stream.sIO[i].eof = qtrue;
				}
			}
		}
		//		LeaveCriticalSection (&stream.crit);
	}
}

/*
===============
Sys_InitStreamThread

================
*/
void Sys_InitStreamThread(void) {
	int i;

	InitializeCriticalSection(&stream.crit);

	// don't leave the critical section until there is a
	// valid file to stream, which will cause the StreamThread
	// to sleep without any overhead
//	EnterCriticalSection( &stream.crit );

	stream.threadHandle = CreateThread(
		NULL,	// LPSECURITY_ATTRIBUTES lpsa,
		0,		// DWORD cbStack,
		(LPTHREAD_START_ROUTINE)Sys_StreamThread,	// LPTHREAD_START_ROUTINE lpStartAddr,
		0,			// LPVOID lpvThreadParm,
		0,			//   DWORD fdwCreate,
		&stream.threadId);
	for (i = 0; i < MAX_FILE_HANDLES; i++) {
		stream.sIO[i].active = qfalse;
	}
}

/*
===============
Sys_ShutdownStreamThread

================
*/
void Sys_ShutdownStreamThread(void) {
}


/*
===============
Sys_BeginStreamedFile

================
*/
void Sys_BeginStreamedFile(fileHandle_t f, int readAhead) {
	if (stream.sIO[f].file) {
		Sys_EndStreamedFile(stream.sIO[f].file);
	}

	stream.sIO[f].file = f;
	stream.sIO[f].buffer = Z_Malloc(readAhead);
	stream.sIO[f].bufferSize = readAhead;
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;
	stream.sIO[f].active = qtrue;

	// let the thread start running
//	LeaveCriticalSection( &stream.crit );
}

/*
===============
Sys_EndStreamedFile

================
*/
void Sys_EndStreamedFile(fileHandle_t f) {
	if (f != stream.sIO[f].file) {
		Com_Error(ERR_FATAL, "Sys_EndStreamedFile: wrong file");
	}
	// don't leave critical section until another stream is started
	EnterCriticalSection(&stream.crit);

	stream.sIO[f].file = 0;
	stream.sIO[f].active = qfalse;

	Z_Free(stream.sIO[f].buffer);

	LeaveCriticalSection(&stream.crit);
}


/*
===============
Sys_StreamedRead

================
*/
int Sys_StreamedRead(void *buffer, int size, int count, fileHandle_t f)
{
	int		available;
	int		remaining;
	int		sleepCount;
	int		copy;
	int		bufferCount;
	int		bufferPoint;
	byte	*dest;

	if (stream.sIO[f].active == qfalse) {
		Com_Error(ERR_FATAL, "Streamed read with non-streaming file");
	}

	dest = (byte *)buffer;
	remaining = size * count;

	if (remaining <= 0) {
		Com_Error(ERR_FATAL, "Streamed read with non-positive size");
	}

	sleepCount = 0;
	while (remaining > 0) {
		available = stream.sIO[f].threadPosition - stream.sIO[f].streamPosition;
		if (!available) {
			if (stream.sIO[f].eof) {
				break;
			}
			if (sleepCount == 1) {
				Com_DPrintf("Sys_StreamedRead: waiting\n");
			}
			if (++sleepCount > 100) {
				Com_Error(ERR_FATAL, "Sys_StreamedRead: thread has died");
			}
			Sleep(10);
			continue;
		}

		EnterCriticalSection(&stream.crit);

		bufferPoint = stream.sIO[f].streamPosition % stream.sIO[f].bufferSize;
		bufferCount = stream.sIO[f].bufferSize - bufferPoint;

		copy = available < bufferCount ? available : bufferCount;
		if (copy > remaining) {
			copy = remaining;
		}
		memcpy(dest, stream.sIO[f].buffer + bufferPoint, copy);
		stream.sIO[f].streamPosition += copy;
		dest += copy;
		remaining -= copy;

		LeaveCriticalSection(&stream.crit);
	}

	return (count * size - remaining) / size;
}

/*
===============
Sys_StreamSeek

================
*/
void Sys_StreamSeek(fileHandle_t f, int offset, int origin) {

	// halt the thread
	EnterCriticalSection(&stream.crit);

	// clear to that point
	FS_Seek(f, offset, origin);
	stream.sIO[f].streamPosition = 0;
	stream.sIO[f].threadPosition = 0;
	stream.sIO[f].eof = qfalse;

	// let the thread start running at the new position
	LeaveCriticalSection(&stream.crit);
}

#endif
