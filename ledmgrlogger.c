/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
########################################################################## 
*/
#include "ledmgrlogger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdexcept>

#include <unistd.h>
#include <sys/syslog.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include <rdk_debug.h>

extern logDest logdestination;
extern logLevel loglevelspecify;

int opensyslog=0;
int initrdklog=0;

char const* kLogName = "ledmgr";

struct LevelMapping
{
  char const* s;
  logLevel l;
};

LevelMapping mappings[] = 
{
  { "CRIT",   logLevel_Critical },
  { "ERROR",  logLevel_Error },
  { "WARN",   logLevel_Warning },
  { "INFO",   logLevel_Info },
  { "DEBUG",  logLevel_Debug }
};

int const kNumMappings = sizeof(mappings) / sizeof(LevelMapping);
char const* kUnknownMapping = "UNKNOWN";

int toSyslogPriority(logLevel level)
{
  int p = LOG_EMERG;
  switch (level)
  {
    case logLevel_Critical:
      p = LOG_CRIT;
      break;
    case logLevel_Error:
      p = LOG_ERR;
      break;
    case logLevel_Warning:
      p = LOG_WARNING;
      break;
    case logLevel_Info:
      p = LOG_INFO;
      break;
    case logLevel_Debug:
      p = LOG_DEBUG;
      break;
  }
  return LOG_MAKEPRI(LOG_FAC(LOG_LOCAL4), LOG_PRI(p));
}

rdk_LogLevel toRdklogPriority(logLevel level)
{
  rdk_LogLevel rdklevel = RDK_LOG_TRACE1;
  switch (level)
  {
    case logLevel_Critical:
      rdklevel = RDK_LOG_FATAL;
      break;
    case logLevel_Error:
      rdklevel = RDK_LOG_ERROR;
      break;
    case logLevel_Warning:
      rdklevel = RDK_LOG_WARN;
      break;
    case logLevel_Info:
      rdklevel = RDK_LOG_INFO;
      break;
    case logLevel_Debug:
      rdklevel = RDK_LOG_DEBUG;
      break;
  }
  return rdklevel;
}

char const* level_to_string(logLevel level)
{
  char const *ret = NULL;
  switch(level)
  {
    case logLevel_Critical:
      ret = "FATAL";
      break;
    case logLevel_Info:
      ret = "INFO";
      break;
    case logLevel_Warning:
      ret = "WARNING";
      break;
    case logLevel_Error:
      ret = "ERROR";
      break;
    case logLevel_Debug:
      ret = "DEBUG";
      break;
    default:
      ret = "INFO";
      break;
  }
  return ret;
}

void log(logLevel level, char const* function, int line, char const* format, ...)
{
  va_list args;
  char buff[2048];
  memset(buff,0,sizeof(buff));
  va_start(args, format);
  if(logdestination == logDest_Rdk)
  {
    if(initrdklog == 0)
    {
      rdk_logger_init("/etc/debug.ini");
      initrdklog = 1;
    }
    int n = vsnprintf(buff, sizeof(buff), format, args);
    buff[sizeof(buff) - 1] = '\0';
    rdk_LogLevel priority = toRdklogPriority(level);
    if (n < sizeof(buff))
    {
      RDK_LOG(priority, "LOG.RDK.LEDMGR", "%s\n", buff);
    }
    else
    {
      RDK_LOG(priority, "LOG.RDK.LEDMGR", "%s...\n", buff);
    }
  }
  else
  {
    vsnprintf(buff, sizeof(buff), format, args);
    time_t t = time(NULL);
    struct tm ltm = *localtime(&t);
    printf("%d-%d-%d %d:%d:%d ", ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
    printf("[%s](%s)[%s:%d] %s\n" , "LEDMGR", level_to_string(level),function, line, buff);
  }
  va_end(args);
}

void setLevel(logLevel level)
{
  loglevelspecify = level;
}

void setDestination(logDest dest)
{
  logdestination = dest;
}

bool isLevelEnabled(logLevel level)
{ 
  return level <= loglevelspecify;
}
