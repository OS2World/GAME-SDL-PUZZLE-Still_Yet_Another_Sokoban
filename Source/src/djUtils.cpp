//---------------------------------------------------------------------------
// djUtils.cpp
//
// Copyright (C) 2002-2004 David Joffe
// http://www.scorpioncity.com/
//---------------------------------------------------------------------------

#include "djUtils.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#endif

char *djStrDup(const char *szSource)
{
	if (szSource)
	{
		char *szRet = new char[strlen(szSource)+1];
		strcpy(szRet, szSource);
		return szRet;
	}
	return NULL;
}

FILE *g_pLog = NULL;

void djLog(const char *szFormat, ...)
{
	if (g_pLog==NULL)
	{
		g_pLog = fopen("syasokobanlog.txt", "w");
	}
	char szBuf[1024];
	va_list arg;
	va_start(arg, szFormat);
	vsprintf(szBuf, szFormat, arg);
	strcat(szBuf, "\n");
	va_end(arg);
#ifdef WIN32
	::OutputDebugString(szBuf);
#endif
	fprintf(g_pLog, "%s", szBuf);
	fflush(g_pLog);
}

void djAssert(int nExpression, const char *szFile, int nLine, const char *szExpression)
{
	if (nExpression==0)
	{
		char szMsg[16384];
		sprintf(szMsg, "Assertion failed (%s) in file %s at line %d", szExpression, szFile, nLine);
		djLog("ASSERT: [%s]", szMsg);
		#ifdef WIN32
			// Save assertion to an assertion log file
			FILE *pOut = fopen("Log_Assertions.txt", "a");
			if (pOut)
			{
				fprintf(pOut, "%s\n", szMsg);
				fclose(pOut);
			}

			// Tell user about this
			::MessageBox(NULL, szMsg, "Program error!", MB_OK);
		#else
			printf("Error: %s", szMsg);
		#endif

		// fixme[h]: application needs a chance to handle this, e.g. to clean up SDL

#ifdef _DEBUG
		abort();
#else
		exit(-1);
#endif
	}
}
