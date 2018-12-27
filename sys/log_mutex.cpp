#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "log_mutex.h"

CLogMutex::CLogMutex()
{
}

CLogMutex::~CLogMutex()
{
}

int
CLogMutex::Lock()
{
	return _mutex.lock();
}

int
CLogMutex::UnLock()
{
	return _mutex.unlock();
}

