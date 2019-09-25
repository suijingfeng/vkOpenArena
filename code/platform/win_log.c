#include <stdio.h>
#include <time.h>


#include "../client/client.h"

// need to complete ...
static FILE* log_fp;
static int isLoggingAlreadyEnabled = 0;

static void fnToggleLogging_f(void);

void win_getLocalTimeStr(char * const pBuffer, unsigned int size)
{
	time_t aclock;
	time(&aclock);
	// Convert time_t to tm as local time
	// Uses the value pointed by timer to fill a tm structure with the values
	// that represent the corresponding time, expressed for the local timezone.
	// The time_t value sourceTime represents the seconds elapsed since 
	// midnight (00:00:00), January 1, 1970, UTC. This value is usually obtained
	// from the time function.
	struct tm *newtime = localtime(&aclock);
	// Convert tm structure to string
	// Interprets the contents of the tm structure pointed by timeptr as a 
	// calendar time and converts it to a C - string containing a human readable 
	// version of the corresponding date and time.
	snprintf(pBuffer, size, "%s", asctime(newtime));
}



void win_InitLoging(void)
{
	Cmd_AddCommand("toggleLogging", fnToggleLogging_f);
}


void win_EndLoging(void)
{
	if (log_fp != NULL)
	{
		fclose(log_fp);

		log_fp = NULL;
	}

	Cmd_RemoveCommand("toggleLogging");
}


void fnToggleLogging_f(void)
{
	// return if we're already active
	if (isLoggingAlreadyEnabled == 1)
	{
		// toggled
		isLoggingAlreadyEnabled = 0;

		if (log_fp)
		{
			fprintf(log_fp, "*** CLOSING LOG ***\n");
			fclose(log_fp);
			log_fp = NULL;
		}

		Com_Printf(" Logging Disabled! \n");
		// return if we're already disabled
		return;
	}
	else
	{
		isLoggingAlreadyEnabled = 1;

		if (!log_fp)
		{
			char buffer[256] = { 0 };
			win_getLocalTimeStr(buffer, sizeof(buffer));
			log_fp = fopen("oa_debug.log", "wt");
			fprintf(log_fp, "%s\n", buffer);
			Com_Printf(" Logging Enabled! \n");
		}
	}
}


void FileSys_Logging(const char * const comment)
{
	if (log_fp) {
		fprintf(log_fp, "%s", comment);
	}
}
