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
#ifndef __LEDLOGGER_H__
#define __LEDLOGGER_H__

#include <stdio.h>

enum logLevel
{
  logLevel_Critical,
  logLevel_Error,
  logLevel_Warning,
  logLevel_Info,
  logLevel_Debug
};

enum logDest
{
  logDest_Stdout,
  logDest_Syslog,
  logDest_Rdk
};

void log(logLevel level, char const* file, int line, char const* msg, ...);
bool isLevelEnabled(logLevel level);
void setLevel(logLevel level);
void setDestination(logDest dest);
char const* level_to_string(logLevel level);

#define LEDMGR_LOG(LEVEL, FORMAT, ...) \
    do { if (isLevelEnabled(LEVEL)) { \
        log(LEVEL, __func__ , __LINE__ - 2, FORMAT, ##__VA_ARGS__); \
    } } while (0)

#define LEDMGR_LOG_DEBUG(FORMAT, ...) LEDMGR_LOG(logLevel_Debug, FORMAT, ##__VA_ARGS__)
#define LEDMGR_LOG_INFO(FORMAT, ...) LEDMGR_LOG(logLevel_Info, FORMAT, ##__VA_ARGS__)
#define LEDMGR_LOG_WARN(FORMAT, ...) LEDMGR_LOG(logLevel_Warning, FORMAT, ##__VA_ARGS__)
#define LEDMGR_LOG_ERROR(FORMAT, ...) LEDMGR_LOG(logLevel_Error, FORMAT, ##__VA_ARGS__)
#define LEDMGR_LOG_CRITICAL(FORMAT, ...) LEDMGR_LOG(logLevel_Critical, FORMAT, ##__VA_ARGS__)

#endif
