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

 #include <stdio.h>
 #include <stdlib.h>
 #include <getopt.h>
 #include <string.h>
 #include "ledmgrlogger.h"
 #include "ledmgr.h"

logDest logdestination=logDest_Rdk;
logLevel loglevelspecify=logLevel_Info;

//Static Function declarations
static ledId_t ledIdFromString(char const* s);
static ledMgrState_t ledStateFromString(char const* s);
static ledMgrOp_t ledOperationFromString(char const* s);
static ledMgrColor_t ledColorFromString(char const* s);
static void logLevelFromString(char const* s);
static void logDestinationFromString(char const* s);
static void print_usage(void);

static ledId_t ledIdFromString(char const* s)
{
  if(strcmp(s, "CAMERA") == 0)
    return LED_ID_CAMERA_FRONT_PANEL;
  if(strcmp(s, "XW") == 0)
    return LED_ID_XW_FRONT_PANEL;
  if(strcmp(s, "IR") == 0)
    return LED_ID_CAMERA_IR;

  return LED_ID_MAX;  
}

static ledMgrState_t ledStateFromString(char const* s)
{
  if(strcmp(s, "BOOTUP") == 0)
    return LED_MGR_STATE_BOOT_UP;
  if(strcmp(s, "READYTOPAIR") == 0)
    return LED_MGR_STATE_READY_TO_PAIR;
  if(strcmp(s, "TROUBLE_CONN") == 0)
    return LED_MGR_STATE_TROUBLE_CONNECTING;
  if(strcmp(s, "WORKING_NORMALLY") == 0)
    return LED_MGR_STATE_WORKING_NORMALLY;
  if(strcmp(s, "2_WAY_VOICE") == 0)
    return LED_MGR_STATE_2_WAY_VOICE;
  if(strcmp(s,"FACTORY_MODE") == 0)
    return LED_MGR_STATE_FACTORY_DOWNLOAD_MODE;
  if(strcmp(s,"INCORRECT_XW") == 0)
    return LED_MGR_STATE_INCORRECT_XW;
  if(strcmp(s,"NOT_PROVISIONED") == 0)
    return LED_MGR_STATE_NOT_PROVISIONED;

  return LED_MGR_STATE_UNKNOWN;
}

static ledMgrOp_t ledOperationFromString(char const* s)
{
  if(strcmp(s, "SOLID_LIGHT") == 0)
    return LED_MGR_OP_SOLID_LIGHT;
  if(strcmp(s, "SLOW_BLINK") == 0)
    return LED_MGR_OP_SLOW_BLINK;
  if(strcmp(s, "BLINK") == 0)
    return LED_MGR_OP_BLINK;
  if(strcmp(s, "DOUBLE_BLINK") == 0)
    return LED_MGR_OP_DOUBLE_BLINK;
  if(strcmp(s, "FAST_BLINK") == 0)
    return LED_MGR_OP_FAST_BLINK;
  if(strcmp(s, "NO_LIGHT") == 0)
    return LED_MGR_OP_NO_LIGHT;

  return LED_MGR_OP_MAX;
}

static ledMgrColor_t ledColorFromString(char const* s)
{
  if(strcmp(s, "WHITE") == 0)
    return LED_MGR_COLOR_WHITE;
  if(strcmp(s, "BLUE") == 0)
    return LED_MGR_COLOR_BLUE;
  if(strcmp(s, "AMBER") == 0)
    return LED_MGR_COLOR_AMBER;
  if(strcmp(s, "GREEN") == 0)
    return LED_MGR_COLOR_GREEN;
  if(strcmp(s, "RED") == 0)
    return LED_MGR_COLOR_RED;

  return LED_MGR_COLOR_MAX;
}

static void logLevelFromString(char const* s)
{
  if(strcmp(s, "CRITICAL") == 0)
    loglevelspecify = logLevel_Critical;
  if(strcmp(s, "ERROR") == 0)
    loglevelspecify = logLevel_Error;
  if(strcmp(s, "WARNING") == 0)
    loglevelspecify = logLevel_Warning;
  if(strcmp(s, "INFO") == 0)
    loglevelspecify = logLevel_Info;
  if(strcmp(s, "DEBUG") == 0)
    loglevelspecify = logLevel_Debug;
  setLevel(loglevelspecify);
}

static void logDestinationFromString(char const* s)
{
  if (strcmp(s, "RDKLOG") == 0)
    logdestination = logDest_Rdk;
  if (strcmp(s, "STDOUT") == 0 || strcmp(s, "-") == 0)
    logdestination = logDest_Stdout;
  setDestination(logdestination);
}

static struct option long_options[] =
{
  { "ledid",        required_argument, 0, 'l' },
  { "state",        required_argument, 0, 's' },
  { "operation",    required_argument, 0, 'o' },
  { "color",        required_argument, 0, 'c' },
  { "help",         no_argument,       0, 'h' },
  { "log-level",    required_argument, 0, 'e' },
  { "logger",       required_argument, 0, 'g' },
  { 0, 0, 0, 0 }
};

static void print_usage(void)
{
  printf("\n");
  printf("usage ledmgr [options]\n");
  printf("\n");
  printf("\t--ledId        -l    LedId's supported\n"
         "\t                     CAMERA | XW | IR \n");
  printf("\t--state        -s    Led states supported\n"
         "\t                     BOOTUP | INCORRECT_XW | READYTOPAIR | TROUBLE_CONN | NOT_PROVISIONED | WORKING_NORMALLY | 2_WAY_VOICE | FACTORY_MODE \n");
  printf("\t--operation    -o    Led Operations supported\n"
         "\t                     SOLID_LIGHT | BLINK | SLOW_BLINK | DOUBLE_BLINK | FAST_BLINK | NO_LIGHT \n");
  printf("\t--color        -c    Led Colors supported\n"
         "\t                     WHITE | BLUE | AMBER | GREEN | RED \n");
  printf("\t--help         -h    Print this help and exit\n");
  printf("\t--log-level    -e    Logging level\n"
         "\t                     CRITICAL | ERROR | WARNING | INFO | DEBUG \n");
  printf("\t--logger       -g    Logger\n"
         "\t                     RDKLOG | STDOUT\n");
}

int main(int argc, char* argv[])
{
  ledmgr_init();
  ledId_t id = LED_ID_MAX;
  ledMgrState_t state = LED_MGR_STATE_UNKNOWN;
  ledMgrOp_t op = LED_MGR_OP_MAX;
  ledMgrColor_t color = LED_MGR_COLOR_MAX;
  while (true)
  {
    int option_index = 0;
    int c = getopt_long(argc, argv, "l:s:o:c:e:g:h", long_options, &option_index);
    if (c == -1)
      break;
    switch (c)
    {
      case 'l':
        id = ledIdFromString(optarg);
        if(id < LED_ID_CAMERA_FRONT_PANEL || id > LED_ID_CAMERA_IR) {
          printf("Invalid LedId. Try again...\n");
          id = LED_ID_MAX;
        }
        break;

      case 's':
        state = ledStateFromString(optarg);
        if(state < LED_MGR_STATE_BOOT_UP || state >= LED_MGR_STATE_UNKNOWN){
          printf("Invalid State. Try again...\n");
          state = LED_MGR_STATE_UNKNOWN;
        }
        break;
      
      case 'o':
        op = ledOperationFromString(optarg);
        if(op < LED_MGR_OP_SOLID_LIGHT || op > LED_MGR_OP_NO_LIGHT) {
          printf("Invalid Operation. Try again...\n");
          op = LED_MGR_OP_MAX;
        }
        break;

      case 'c':
        color = ledColorFromString(optarg);
        if(color < LED_MGR_COLOR_AMBER || color > LED_MGR_COLOR_BLUE) {
          printf("Invalid Color. Try again...\n");
          color = LED_MGR_COLOR_MAX;
        }
        break;
  
      case 'e':
        logLevelFromString(optarg);
        break;

      case 'g': 
        logDestinationFromString(optarg);
        break;

      default:
        break;
    }
  }
 
  if(state != LED_MGR_STATE_UNKNOWN) {
    ledmgr_setState(state);
  }
  else if (op != LED_MGR_OP_MAX && color != LED_MGR_COLOR_MAX){
    ledmgr_setOp(id, op, color);
  }
  else if(id == LED_ID_CAMERA_IR){
    //TODO : Need to support IR led's
    printf("IR leds not supported yet.\n");
  } 
  else {
    print_usage();
  }

  return 0;
}
