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
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <sys/sysinfo.h>

#include "ledmgr.h"
#include "ledmgrlogger.h"

#ifdef __cplusplus
extern "C"{
#endif
#include "PRO_file.h"
#include "conf_sec.h"
#include "sysUtils.h"
#include "secure_wrapper.h"
#include "connectivity_finder.h"
#ifdef __cplusplus
}
#endif

#define DEF_USER_ADMIN_NAME              "administrator"
#define WIFI_PAUSE_STATE_FILE            "/tmp/.wifipaused"

logDest logdestination=logDest_Rdk;
logLevel loglevelspecify=logLevel_Debug;

#ifdef ENABLE_RTMESSAGE
/* RtMessage */
#include "rtLog.h"
#include "rtConnection.h"
#include "rtMessage.h"
#include "rtError.h"
#define RTROUTED_ADDRESS "tcp://127.0.0.1:10001"
#endif

#include "ledmgr_rtmsg.h"

/*! Event states associated with WiFi connection  */
typedef enum _WiFiStatusCode_t {
    WIFI_UNINSTALLED = 0,                                           /* !< The device was in an installed state, and was uninstalled */
    WIFI_DISABLED,                                                      /* !< The device is installed (or was just installed) and has not yet been enabled*/
    WIFI_DISCONNECTED,                                          /* !< The device is not connected to a network */
    WIFI_PAIRING,                                                       /* !< The device is not connected to a network, but not yet connecting to a network*/
    WIFI_CONNECTING,                                            /* !< The device is attempting to connect to a network */
    WIFI_CONNECTED,                                                     /* !< The device is successfully connected to a network */
    WIFI_FAILED                                                         /* !< The device has encountered an unrecoverable error with the wifi adapter */
} WiFiStatusCode_t;
 
static ledMgrState_t bootup_st_handler(void);
static ledMgrState_t incorrect_xw_st_handler(void);
static ledMgrState_t ready_to_pair_st_handler(void);
static ledMgrState_t not_provisioned_st_handler(void);
static ledMgrState_t trouble_connect_st_handler(void);
static ledMgrState_t working_normally_st_handler(void);
static ledMgrState_t two_way_voice_st_handler(void);
static ledMgrState_t factory_dw_mode_st_handler(void);
static ledMgrState_t wifi_pause_st_handler(void);

static void logLevelFromString(char const* s);
static void logDestinationFromString(char const* s);

int bt_state = 0;
int audio_state = 0;

MODE state;
int pause_count = 0;
int disconnect_count = 0;

#ifdef ENABLE_RTMESSAGE
static rtConnection led_con = NULL;
void rtConnection_init();
static void* rtMessage_StatusReceive(void * arg);
void rtConnection_destroy();
static void onWiFiMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
static void onBTActive(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
static void onTwoWayAudio(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure);
#endif

static bool is_camera_connected();
static bool is_camera_pairing_mode();
static bool is_camera_not_online();
static bool is_xw_incompatible();
static bool is_camera_not_provisioned_onboard();
static bool is_camera_not_provisioned();
static bool is_camera_booting_up();
static bool is_voice_out();
static bool is_wifi_paused();
//static bool is_xw_connected();

static WiFiStatusCode_t wifiState = WIFI_UNINSTALLED;

ledMgrState_t (*get_next_state[LED_MGR_STATE_UNKNOWN])(void) = {
                    bootup_st_handler,            //LED_MGR_STATE_BOOT_UP
                    incorrect_xw_st_handler,      //LED_MGR_STATE_INCORRECT_XW
                    ready_to_pair_st_handler,     //LED_MGR_STATE_READY_TO_PAIR
                    not_provisioned_st_handler,   //LED_MGR_STATE_NOT_PROVISIONED
                    trouble_connect_st_handler,   //LED_MGR_STATE_TROUBLE_CONNECTING
                    wifi_pause_st_handler,	  //LED_MGR_STATE_WIFI_PAUSED
                    working_normally_st_handler,  //LED_MGR_STATE_WORKING_NORMALLY
                    two_way_voice_st_handler,     //LED_MGR_STATE_2_WAY_VOICE
                    factory_dw_mode_st_handler    //LED_MGR_STATE_FACTORY_DOWNLOAD_MODE
                };

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

#ifdef ENABLE_RTMESSAGE
void rtConnection_init()
{
  rtLog_SetLevel(RT_LOG_INFO);
  rtLog_SetOption(rdkLog);

  rtConnection_Create(&led_con, "LEDMGR", RTROUTED_ADDRESS);
  rtConnection_AddListener(led_con, "RDKC.WIFI.STATE", onWiFiMessage, led_con);
  rtConnection_AddListener(led_con, "RDKC.PRVNMGR", onBTActive, led_con);
  rtConnection_AddListener(led_con, "RDKC.TWOWAYAUDIO", onTwoWayAudio, led_con);
}

void rtConnection_destroy()
{
  rtConnection_Destroy(led_con);
}

void onWiFiMessage(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  rtConnection led_con = (rtConnection) closure;

  rtMessage msg;
  int wifi_state;
  rtMessage_FromBytes(&msg, buff, n);
  rtMessage_GetInt32(msg, "wifi_status", &wifi_state);
  LEDMGR_LOG_INFO("WiFi Status is : %d", wifi_state);
  wifiState = (WiFiStatusCode_t)wifi_state;
  LEDMGR_LOG_DEBUG("WiFi Status after assigning : %d", wifiState);
  rtMessage_Release(msg); 
}

void onBTActive(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  rtConnection led_con = (rtConnection) closure;

  rtMessage msg;
  rtMessage_FromBytes(&msg, buff, n);

  rtMessage_GetInt32(msg, "bt_status", &bt_state);
  LEDMGR_LOG_INFO("BT Status is : %d", bt_state);
  rtMessage_Release(msg);
}

void onTwoWayAudio(rtMessageHeader const* hdr, uint8_t const* buff, uint32_t n, void* closure)
{
  rtConnection led_con = (rtConnection) closure;

  rtMessage msg;
  rtMessage_FromBytes(&msg, buff, n);

  rtMessage_GetInt32(msg, "audio_status", &audio_state);
  LEDMGR_LOG_INFO("Audio Status is : %d", audio_state);
  rtMessage_Release(msg);
}

void* rtMessage_StatusReceive(void* arg)
{
  while (1)
  {
    rtError err = rtConnection_Dispatch(led_con);
    if (err != RT_OK)
    {
      LEDMGR_LOG_DEBUG("Dispatch Error: %s", rtStrError(err));
    }
    //Adding sleep to avoid logs flooding in case of rtrouted bad state
    usleep(10000);
  }
}
#endif

static ledMgrState_t bootup_st_handler(void)
{   
    ledMgrState_t state = LED_MGR_STATE_BOOT_UP;

    if(is_xw_incompatible())
    {
        state = LED_MGR_STATE_INCORRECT_XW;
    }
    else if(is_camera_booting_up())
    {
        state = LED_MGR_STATE_BOOT_UP;
    }
    else if(is_camera_pairing_mode() && is_camera_not_provisioned())
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(!is_camera_not_provisioned() && is_camera_not_online())
    {
	state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }
    else if(is_camera_connected() && !is_camera_not_provisioned())
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }

    return state;
}

static ledMgrState_t incorrect_xw_st_handler(void)
{
    return LED_MGR_STATE_INCORRECT_XW;
}

static ledMgrState_t ready_to_pair_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_READY_TO_PAIR;

    if(bt_state || (is_camera_pairing_mode() && is_camera_not_provisioned_onboard()))
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(bt_state && is_wifi_paused() && !is_camera_not_provisioned())
    {
        state = LED_MGR_STATE_WIFI_PAUSED;
    }
    else if(!is_camera_not_provisioned_onboard() && is_camera_not_online())
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }
    else if(is_camera_connected() && !is_camera_not_provisioned_onboard()) 
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }

    return state;
}

static ledMgrState_t not_provisioned_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_NOT_PROVISIONED;

    if(is_camera_pairing_mode())
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(is_camera_connected() && is_camera_not_provisioned())
    {
        state = LED_MGR_STATE_NOT_PROVISIONED;
    }
    else if(is_camera_connected() && !is_camera_not_provisioned())
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    else if(is_camera_not_online())
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }

    return state;
}

static ledMgrState_t trouble_connect_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_TROUBLE_CONNECTING;

    if (bt_state)
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(is_wifi_paused())
    {
        state = LED_MGR_STATE_WIFI_PAUSED;
    }
    else if(!is_wifi_paused() && is_camera_connected())
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }

    return state;
}

static ledMgrState_t wifi_pause_st_handler(void)
{
    //Turn off IR Led, Speaker, Microphone and touch pause state file
    if (access(WIFI_PAUSE_STATE_FILE, F_OK) != 0)
      system("sh /lib/rdk/wifi_pause_state.sh 1 &");

    ledMgrState_t state = LED_MGR_STATE_WIFI_PAUSED;

    if (bt_state)
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(!is_wifi_paused() && is_camera_not_online())
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }
    else if(!is_wifi_paused() && is_camera_connected())
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    else if(!is_wifi_paused() && is_voice_out())
    {
        state = LED_MGR_STATE_2_WAY_VOICE;
    }

    return state;
}

static ledMgrState_t working_normally_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_WORKING_NORMALLY;
 
    //sleep(1);
    if (bt_state)
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(is_wifi_paused() && !is_voice_out())
    {
        state = LED_MGR_STATE_WIFI_PAUSED;
    }
    else if(is_camera_not_online() && !is_voice_out())
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }
    else if(!is_wifi_paused() && is_voice_out())
    {
        state = LED_MGR_STATE_2_WAY_VOICE;
    }
    
    return state;
}

static ledMgrState_t two_way_voice_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_2_WAY_VOICE;

    if(is_wifi_paused() && !is_voice_out())
    {
        state = LED_MGR_STATE_WIFI_PAUSED;
    }
    else if(is_camera_connected() && !is_voice_out())
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    else if(!is_camera_not_provisioned() && is_camera_not_online())
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }
    else
    {
        state = LED_MGR_STATE_2_WAY_VOICE;
    }
    return state;
}

static ledMgrState_t factory_dw_mode_st_handler(void)
{
    return LED_MGR_STATE_FACTORY_DOWNLOAD_MODE;
}

static bool is_camera_connected()
{
    if (state == CONNECTED_STATE)
    {
      LEDMGR_LOG_DEBUG("Camera is connected");
      // Remove wifi pause state file,turn speaker, microphone on when wifi is unpaused
      if (access(WIFI_PAUSE_STATE_FILE, F_OK) != -1)
        system("sh /lib/rdk/wifi_pause_state.sh 0 &");
      if (pause_count != 0)
        pause_count = 0;
      if (disconnect_count != 0)
        disconnect_count = 0;
    }
    else if (state == DISCONNECTED_STATE)
    {
      if (pause_count != 0)
        pause_count = 0;
      if (disconnect_count >= 5)
      {
        LEDMGR_LOG_DEBUG("Camera is not connected");
        return false;
      }
      else
        disconnect_count++;
    }
    return true;
}

static bool is_wifi_paused()
{
    if (state == PAUSE_STATE)
    {
      if (disconnect_count != 0)
        disconnect_count = 0;
      if (pause_count >= 5)
      {
        LEDMGR_LOG_DEBUG("Wifi is paused");
        return true;
      }
      else
        pause_count++;
    }
    else
    {
      if (pause_count != 0)
        pause_count = 0;
      LEDMGR_LOG_DEBUG("Wifi is not paused");
      // Remove wifi pause state file ,turn speaker, microphone on once wifi is unpaused
      if (access(WIFI_PAUSE_STATE_FILE, F_OK) != -1)
        system("sh /lib/rdk/wifi_pause_state.sh 0 &");
    }
    return false;
}

static bool is_camera_pairing_mode()
{
    bool wps = false;
    //wps pairing
    if (access("/tmp/.wps_state_working", F_OK) != -1)
    {
      LEDMGR_LOG_DEBUG("Camera is in wps mode");
      wps = true;
    }
    //ble pairing
    if (access("/tmp/.prvn_ble_pairing", F_OK) != -1)
    {
      LEDMGR_LOG_DEBUG("Camera is in ble pairing mode");
      wps = true;
    }

    //if (wps || wifiState == WIFI_CONNECTING)
    if (wps)
       return true;
    else
    {
       LEDMGR_LOG_DEBUG("Camera is not in pairing mode");
       return false;
    }
}

static bool is_camera_not_online()
{
  return !is_camera_connected();
}

static bool is_xw_incompatible()
{
    if (access("/tmp/.incorrect_hardware", F_OK) != -1)
    {
      return true;
    }
    return false;
}

static bool is_camera_not_provisioned_onboard()
{
  bool prov_flag = true;

  if ((access("/opt/.prvn_complete", F_OK) == 0) || (access("/opt/.prvn_icontrol_complete", F_OK) == 0))
    {
      LEDMGR_LOG_DEBUG("Camera is provisioned");
      prov_flag = false;
    }
  else
      LEDMGR_LOG_DEBUG("Camera is not provisioned");
  return prov_flag;
}


static bool is_camera_not_provisioned()
{
    FILE *fp = NULL;
    char admin_name[100] = DEF_USER_ADMIN_NAME;
    bool prov_flag = true;

    //open system config
    fp = fopen(SYSTEM_CONF,"r");
    if (NULL != fp){
       int ret = PRO_GetStr(SEC_USER, USER_ADMIN_NAME, admin_name, sizeof(admin_name),fp);
       if (ret != LED_ERR_NONE)
          LEDMGR_LOG_ERROR("Unable to get User Admin Name");
       //close system config
       fclose(fp);
       fp = NULL;
    }else{
      LEDMGR_LOG_ERROR("Unable to open system conf file");
    }

    if ((strcmp(admin_name, DEF_USER_ADMIN_NAME) != 0) || (access("/opt/.prvn_complete", F_OK) == 0) || (access("/opt/.prvn_icontrol_complete", F_OK) == 0))
    {
      LEDMGR_LOG_DEBUG("Camera is provisioned");
      prov_flag = false;
    }
    else
      LEDMGR_LOG_DEBUG("Camera is not provisioned");

    return prov_flag;
}

static bool is_camera_booting_up()
{
    if (access("/tmp/.bootup_complete", F_OK) != -1)
    {
      return false;
    }
    LEDMGR_LOG_DEBUG("Camera is booting up");
    return true;
}

static bool is_voice_out()
{
    return audio_state;
}

/*static bool is_xw_connected()
{
    if ((system("ping -c 3 169.254.99.8 -W 5 > /dev/null") == 0))
      return true;
    LEDMGR_LOG_DEBUG("xw is not connected");
    return false;
}*/

int main(int argc, char* argv[])
{
  system("GetConfigFile /tmp/xw4.juc");
  int loop = 1;
  ledMgrState_t next_state = LED_MGR_STATE_BOOT_UP;
  ledMgrState_t cur_state = LED_MGR_STATE_UNKNOWN;
  ledMgrErr_t err;
  rtConnection_Init();

  int xw_current_state = xw_isconnected();
  int xw_next_state = 0;

  ledmgr_init();

 #ifdef ENABLE_RTMESSAGE
  rtConnection_init();

  pthread_t wifiStateThread;
  if( 0 != pthread_create(&wifiStateThread, NULL, &rtMessage_StatusReceive, NULL))
    { 
      LEDMGR_LOG_ERROR("Can't create thread.");
    }
 #endif

  do
  {
    xw_next_state = xw_isconnected();
    /* Captive portal call to check wifi status */
    state = stateFinder();
    if ((next_state != cur_state) || (xw_current_state != xw_next_state))
    {
        LEDMGR_LOG_INFO("Current state %d Next state %d ", cur_state, next_state);
        LEDMGR_LOG_INFO("xw current state %d next state %d", xw_current_state, xw_next_state);
        err = ledmgr_setState(next_state);
        //handle error 
        cur_state = next_state;
        xw_current_state = xw_next_state;
    }
    next_state = (*get_next_state[cur_state])();
    sleep(2); //one second sleep
  }while(loop == 1);

#ifdef ENABLE_RTMESSAGE  
  pthread_kill(wifiStateThread, 0);
  rtConnection_destroy();
#endif
  
  rtConnection_leddestroy();

  return 0;
}

