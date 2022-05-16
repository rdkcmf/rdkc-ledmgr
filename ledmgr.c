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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h> 
#include <pthread.h>
#include <unistd.h>

#include "ledmgrlogger.h"
#include "ledmgr.h"
#include "ledmgr_rtmsg.h"

#ifdef __cplusplus
extern "C"{
#endif
#include "sc_tool.h"
#include "PRO_file.h" 
#include "conf_sec.h"
#include "sysUtils.h" 
#ifdef __cplusplus
}
#endif

#define INVALID_TIME                      (-1)
#define LEDMGR_ASSERT_NOT_NULL(P)       if ((P) == NULL) return LED_MGR_ERR_GENERAL
#define DEF_USER_ADMIN_NAME              "administrator"
#define XW_INIT_MAX_RETRY                 25

/* Telemetry 2.0 */
#include "telemetry_busmessage_sender.h"

typedef struct ledRGBColor{
  ledMgrColor_t color;
  uint8_t cR;     /* Red color */
  uint8_t bR;     /* Brightness of Red color */
  uint8_t cG;     /* Green color */
  uint8_t bG;     /* Brightness of Green color */
  uint8_t cB;     /* Blue color */
  uint8_t bB;     /* Brightness of Blue color */
}ledRGBColor;

/* Array of all colors and default RGB values */
ledRGBColor g_ledColorVal[LED_MGR_COLOR_MAX] = {
  {LED_MGR_COLOR_AMBER, 63, 255, 63, 153, 0, 0},
  {LED_MGR_COLOR_WHITE, 65, 255, 85, 233, 85, 181},
  {LED_MGR_COLOR_RED, 255, 115, 0, 0, 0, 0},
  {LED_MGR_COLOR_GREEN, 0, 0, 255, 122, 0, 0},
  {LED_MGR_COLOR_BLUE, 0, 0, 0, 0, 255, 150},
};

/*
ledRGBColor g_ledColorVal[LED_MGR_COLOR_MAX] = {
  {LED_MGR_COLOR_WHITE, 255, 255, 255, 255, 255, 255},
  {LED_MGR_COLOR_BLUE, 0, 0, 0, 0, 255, 255},
  {LED_MGR_COLOR_AMBER, 255, 255, 194, 255, 0, 0},
  {LED_MGR_COLOR_GREEN, 0, 0, 255, 255, 0, 0},
  {LED_MGR_COLOR_RED, 255, 255, 0, 0, 0, 0}
};
*/

/* Array of all colors and default xw RGB values */
ledRGBColor g_xwledColorVal[LED_MGR_COLOR_MAX] = {
  {LED_MGR_COLOR_AMBER, 63, 255, 63, 153, 0, 0},
  {LED_MGR_COLOR_WHITE, 65, 255, 85, 233, 85, 181},
  {LED_MGR_COLOR_RED, 255, 115, 0, 0, 0, 0},
  {LED_MGR_COLOR_GREEN, 0, 0, 255, 122, 0, 0},
  {LED_MGR_COLOR_BLUE, 0, 0, 0, 0, 255, 150},
};

typedef struct ledOp{
  ledMgrOp_t op;      /* operation */
  int32_t ontime;    /* on time in ms */
  int32_t offtime;   /* off time in ms */
  int32_t longofftime; /* long off time in ms */
}ledOp;

/* Array of all operations and default on/off time values */
ledOp g_ledOpVal[LED_MGR_OP_MAX] = {
  {LED_MGR_OP_SOLID_LIGHT, INVALID_TIME, INVALID_TIME, INVALID_TIME},
  {LED_MGR_OP_BLINK, 500, 1000, INVALID_TIME},
  {LED_MGR_OP_SLOW_BLINK, 200, 400, INVALID_TIME},
  {LED_MGR_OP_DOUBLE_BLINK, 200, 100, 1000},
  {LED_MGR_OP_FAST_BLINK, 100, 100, INVALID_TIME},
  {LED_MGR_OP_NO_LIGHT, INVALID_TIME, INVALID_TIME, INVALID_TIME}   
};

/* Static functions */
static const ledOp* getOpVal(ledMgrOp_t op);
static const ledRGBColor* getColorVal(ledMgrColor_t color);
static const ledRGBColor* getxwColorVal(ledMgrColor_t color);
static ledMgrErr_t led_xw_init(int retry);
static ledMgrErr_t led_xw_setOp(ledId_t id, ledMgrOp_t op, ledMgrColor_t color);

static pthread_mutex_t ledinitmutex = PTHREAD_MUTEX_INITIALIZER;

/* Function to get default RGB color values */
static const ledRGBColor* getColorVal(ledMgrColor_t color)
{
  int i;
  
  for(i = 0; i < LED_MGR_COLOR_MAX; i++){
    if(g_ledColorVal[i].color == color)
      break;    
  }
  
  if(LED_MGR_COLOR_MAX != i)
    return &g_ledColorVal[i];
  else
    return NULL;
}

/* Function to get default xw RGB color values */
static const ledRGBColor* getxwColorVal(ledMgrColor_t color)
{
  int i;

  for(i = 0; i < LED_MGR_COLOR_MAX; i++){
    if(g_xwledColorVal[i].color == color)
      break;
  }

  if(LED_MGR_COLOR_MAX != i)
    return &g_xwledColorVal[i];
  else
    return NULL;
}


/* Function to get default values based on operation */
static const ledOp* getOpVal(ledMgrOp_t op)
{
  int i;

  for(i = 0; i < LED_MGR_OP_MAX; i++){
    if(g_ledOpVal[i].op == op)
      break;
  }
  
  if(LED_MGR_OP_MAX != i)
    return &g_ledOpVal[i];
  else
    return NULL;
}

/* API to initialize xw ledmgr */
ledMgrErr_t led_xw_init(int retry)
{
 //TODO find a clearner way to do this
  int count = 1;
  static bool led_xw_color_init = false;
 
  if (led_xw_color_init)
    return LED_MGR_ERR_NONE;

  while (count <= retry)
  {
    /* To avoid reading xw system.conf on every boot, we store it locally */
    if (access("/opt/usr_config/xwsystem.conf", F_OK) != 0)
    {

      int retval=0;
      retval = xw_file_get();
      if(retval != 1)
      {
       LEDMGR_LOG_INFO("system.conf not available check again count : %d", count);
       count++;
       continue;
      }
      if(retval ==1)
          LEDMGR_LOG_INFO("\nsystem.conf copied from xw sucessfully ");
    }
    else
    {
      LEDMGR_LOG_INFO("Creating xw_led_conf");
      exec_sys_command("grep -r led_color1 /opt/usr_config/xwsystem.conf | cut -d = -f2 > /tmp/xw_led_conf", NULL);
      exec_sys_command("grep -r led_color2 /opt/usr_config/xwsystem.conf | cut -d = -f2 >> /tmp/xw_led_conf", NULL);
      exec_sys_command("grep -r led_color3 /opt/usr_config/xwsystem.conf | cut -d = -f2 >> /tmp/xw_led_conf", NULL);
      exec_sys_command("grep -r led_color4 /opt/usr_config/xwsystem.conf | cut -d = -f2 >> /tmp/xw_led_conf", NULL);
      exec_sys_command("grep -r led_color5 /opt/usr_config/xwsystem.conf | cut -d = -f2 >> /tmp/xw_led_conf", NULL);
      break;
    }
  }
  
  FILE * fp;
  int index = 0;
  char content[50];
  fp = fopen ("/tmp/xw_led_conf", "r");
  if (fp != NULL)
  {
    while(index < LED_MGR_COLOR_MAX)
    {
      g_xwledColorVal[index].color = (ledMgrColor_t)index;
      fgets(content, 50, fp);

      //current R
      LEDMGR_LOG_INFO("led_color from system.conf color %d, value %s\n", index, content);
      char *col = strtok(content, ":");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_xwledColorVal[index].cR = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_xwledColorVal[index].cR);

      //pwm R
      col = strtok(NULL, ",");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_xwledColorVal[index].bR = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_xwledColorVal[index].bR);

      //current G
      col = strtok(NULL, ":");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_xwledColorVal[index].cG = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_xwledColorVal[index].cG);

      //pwm G
      col = strtok(NULL, ",");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_xwledColorVal[index].bG = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_xwledColorVal[index].bG);

      //current B
      col = strtok(NULL, ":");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_xwledColorVal[index].cB = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_xwledColorVal[index].cB);

      //pwm B
      col = strtok(NULL, ",");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_xwledColorVal[index].bB = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d", g_xwledColorVal[index].bB);

      index++;
    }
    fclose(fp);
  }
  else
  {
    /* use default values */
    LEDMGR_LOG_ERROR("Unable to read led color values from xw system.conf.");
    return LED_MGR_ERR_GENERAL;
  }
  
  led_xw_color_init = true;
  return LED_MGR_ERR_NONE;
}

/* API to initialize ledmgr */
ledMgrErr_t ledmgr_init(void)
{  
  int ret = 0; 
  int led_mode = 0; 
  ret = PRO_SetInt(SEC_SYS, SYS_LED_MODE, led_mode, SYSTEM_CONF); // success return 0, others return value mean something error 
  if (ret != LED_ERR_NONE)
    LEDMGR_LOG_INFO("Error setting led more");

  SYS_INFO systeminfo;
  int status = 0;
  int index = 0;
  
  /* Populate led current and pwm values from system.conf  */
  status = SYSINFO_ReadConfigData(&systeminfo);
  
  /* systeminfo.led_color_val[0..5] contains current and pwm values of
   * AMBER, WHITE, RED, GREEN and BLUE respectively */ 

   if(status == SYSTEM_OK)
   {
    /* parse current and pwm values from systeminfo 
     * Example string. led_color1=39:255,10:204,0:0 */
    
    while(index < LED_MGR_COLOR_MAX)
    {
      g_ledColorVal[index].color = (ledMgrColor_t)index;
      
      //current R
      LEDMGR_LOG_INFO("led_color from system.conf color %d, value %s\n", index, systeminfo.led_color_val[index]);
      char *col = strtok(systeminfo.led_color_val[index], ":");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_ledColorVal[index].cR = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_ledColorVal[index].cR);

      //pwm R
      col = strtok(NULL, ",");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_ledColorVal[index].bR = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_ledColorVal[index].bR);

      //current G
      col = strtok(NULL, ":");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_ledColorVal[index].cG = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_ledColorVal[index].cG);

      //pwm G
      col = strtok(NULL, ",");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_ledColorVal[index].bG = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_ledColorVal[index].bG);

      //current B
      col = strtok(NULL, ":");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_ledColorVal[index].cB = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d, ", g_ledColorVal[index].cB);

      //pwm B
      col = strtok(NULL, ",");
      LEDMGR_ASSERT_NOT_NULL(col);
      g_ledColorVal[index].bB = (uint8_t)atoi(col);
      LEDMGR_LOG_DEBUG("%d", g_ledColorVal[index].bB);

      index++;
    }
    led_xw_init(XW_INIT_MAX_RETRY);
   }
   else
   {
    /* use default values */
    LEDMGR_LOG_ERROR("Unable to read led color values from system.conf.");
    led_xw_init(XW_INIT_MAX_RETRY);
    return LED_MGR_ERR_GENERAL;
   }   
   return LED_MGR_ERR_NONE;
}

/* API to set led state */
ledMgrErr_t ledmgr_setState(ledMgrState_t state)
{
  switch(state)
  {
    case LED_MGR_STATE_BOOT_UP:
      /* boot up state */
      LEDMGR_LOG_INFO("Led set white with ledHalApi during bootup");
      //ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_WHITE);
      //led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_WHITE);
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_BOOT_UP");
      break;
    case LED_MGR_STATE_INCORRECT_XW:
      /* incompatible xw */
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_INCORRECT_XW");
      while (true)
      {
        ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_AMBER);
        led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_AMBER);
        sleep (1);
        ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_RED);
        led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_RED);
        sleep (1);
      }
      break;
    case LED_MGR_STATE_READY_TO_PAIR:
      /* ready to pair state */
      ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_WHITE);
      led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_WHITE);
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_READY_TO_PAIR");
      break;
    case LED_MGR_STATE_TROUBLE_CONNECTING:
      /* trouble connecting state */
      ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_AMBER);
      led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_AMBER);
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_TROUBLE_CONNECTING");
      t2_event_d("LED_INFO_CONNBad", 1);
      break;
    case LED_MGR_STATE_NOT_PROVISIONED:
      /* camera not provisioned */
      ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_AMBER);
      led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_AMBER);
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_NOT_PROVISIONED");
      break;
    case LED_MGR_STATE_WORKING_NORMALLY:
      /* working normally */
      ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_BLUE);
      led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_SOLID_LIGHT, LED_MGR_COLOR_BLUE);
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_WORKING_NORMALLY");
      t2_event_d("LED_INFO_CONNGood", 1);
      break;
    case LED_MGR_STATE_2_WAY_VOICE:
      /* two way voice state */
      ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_BLUE);
      led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_BLUE);
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_2_WAY_VOICE");
      break;
    case LED_MGR_STATE_FACTORY_DOWNLOAD_MODE:
      /* factory download mode state */
      ledmgr_setOp(LED_ID_CAMERA_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_GREEN);
      led_xw_setOp(LED_ID_XW_FRONT_PANEL, LED_MGR_OP_BLINK, LED_MGR_COLOR_GREEN);
      LEDMGR_LOG_INFO("Led State = LED_MGR_STATE_FACTORY_DOWNLOAD_MODE");
      break;
    case LED_MGR_STATE_UNKNOWN:
    default:
      /* invalid state */
      LEDMGR_LOG_ERROR("Invalid state %d", state);
      return LED_MGR_ERR_INVALID_PARAM;
      break;
  }

  return LED_MGR_ERR_NONE;
}

/* API to set led operation and color */
ledMgrErr_t ledmgr_setOp(ledId_t id, ledMgrOp_t op, ledMgrColor_t color)
{
  const ledRGBColor* pColor;
  const ledOp*     pOp;
  int   err = 0;

  if (id == LED_ID_XW_FRONT_PANEL) {
    return led_xw_setOp(id, op, color);
  }  
  pOp = (ledOp*) getOpVal(op);
  pColor = (ledRGBColor*) getColorVal(color);
  if(pOp == NULL || pColor == NULL){
    LEDMGR_LOG_ERROR("Unable to get color or operation %d %d", op, color);
    return LED_MGR_ERR_INVALID_PARAM;
  }

  err = pthread_mutex_trylock(&ledinitmutex);
  if(err != 0){
    LEDMGR_LOG_ERROR("Unable to acquire mutex err: %s", strerror(err));
    LEDMGR_LOG_ERROR("Led manager is busy. Try again later...");
    return LED_MGR_ERR_BUSY;
  }

  /* initialize led hal */
  led_init(id);
  switch(op)
  {
    case LED_MGR_OP_SOLID_LIGHT:
      led_reset(id);
      led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      led_setOnOff(id, "on");
      break;
    case LED_MGR_OP_BLINK:
      led_reset(id);
      led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      led_setBlink(id, pOp->ontime, pOp->offtime);
      break;
    case LED_MGR_OP_SLOW_BLINK:
      led_reset(id);
      led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      led_setBlink(id, pOp->ontime, pOp->offtime);    
      break;
    case LED_MGR_OP_DOUBLE_BLINK:
      led_reset(id);
      led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      led_setBlinkSequence(id, pOp->ontime, pOp->offtime, 2, pOp->longofftime);
      break;
    case LED_MGR_OP_FAST_BLINK:
      led_reset(id);
      led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      led_setBlink(id, pOp->ontime, pOp->offtime);
      break;
    case LED_MGR_OP_NO_LIGHT:
      led_setOnOff(id, "off");
      break;
    case LED_MGR_OP_MAX:
    default:
      LEDMGR_LOG_ERROR("Invalid operation %d", op);
      err = LED_MGR_ERR_INVALID_PARAM;
      break;  
  }
  /* apply configuration */
  if(err == LED_MGR_ERR_NONE)
    led_applySettings(id);

  err = pthread_mutex_unlock(&ledinitmutex);
  if(err != 0){
    LEDMGR_LOG_ERROR("Led manager unlock mutex failed with err: %s", strerror(err));
  }

  return (ledMgrErr_t)err;
}

/* API to set led operation and color */
static ledMgrErr_t led_xw_setOp(ledId_t id, ledMgrOp_t op, ledMgrColor_t color)
{
  const ledRGBColor* pColor;
  const ledOp*     pOp;
  int   err = 0;

  led_xw_init(1);

  pOp = (ledOp*) getOpVal(op);
  pColor = (ledRGBColor*) getxwColorVal(color);
  if(pOp == NULL || pColor == NULL){
    LEDMGR_LOG_ERROR("Unable to get color or operation %d %d", op, color);
    return LED_MGR_ERR_INVALID_PARAM;
  }

  err = pthread_mutex_trylock(&ledinitmutex);
  if(err != 0){
    LEDMGR_LOG_ERROR("Unable to acquire mutex err: %s", strerror(err));
    LEDMGR_LOG_ERROR("Led manager is busy. Try again later...");
    return LED_MGR_ERR_BUSY;
  }
  /* initialize led hal */
  xw_led_init(id);
  switch(op)
  {
    case LED_MGR_OP_SOLID_LIGHT:
      xw_led_reset(id);
      xw_led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      xw_led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      break;
    case LED_MGR_OP_BLINK:
      xw_led_reset(id);
      xw_led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      xw_led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      xw_led_setBlink(id, pOp->ontime, pOp->offtime);
      break;
    case LED_MGR_OP_SLOW_BLINK:
      xw_led_reset(id);
      xw_led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      xw_led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      xw_led_setBlink(id, pOp->ontime, pOp->offtime);
      break;
    case LED_MGR_OP_DOUBLE_BLINK:
      xw_led_reset(id);
      xw_led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      xw_led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      xw_led_setBlinkSequence(id, pOp->ontime, pOp->offtime, 2, pOp->longofftime);
      break;
    case LED_MGR_OP_FAST_BLINK:
      xw_led_reset(id);
      xw_led_setBrightness(id, pColor->bR, pColor->bG, pColor->bB);
      xw_led_setColor(id, pColor->cR, pColor->cG, pColor->cB);
      xw_led_setBlink(id, pOp->ontime, pOp->offtime);
      break;
    case LED_MGR_OP_NO_LIGHT:
      xw_led_setOnOff(id, "off");
      break;
    case LED_MGR_OP_MAX:
    default:
      LEDMGR_LOG_ERROR("Invalid operation %d", op);
      err = LED_MGR_ERR_INVALID_PARAM;
      break;
  }
  /* apply configuration */
  if(err == LED_MGR_ERR_NONE)
    xw_led_applySettings(id);
  
  err = pthread_mutex_unlock(&ledinitmutex);
  if(err != 0){
    LEDMGR_LOG_ERROR("Led manager unlock mutex failed with err: %s", strerror(err));
  }
  return (ledMgrErr_t)err;
} 
