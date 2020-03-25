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
#include "ledhal.h"

ledError_t led_init(ledId_t id)
{
  printf(" %s id: %d\n",__FUNCTION__, id);
  return LED_ERR_NONE;
}

ledError_t led_reset(ledId_t id)
{
  printf(" %s id: %d\n",__FUNCTION__, id);
  return LED_ERR_NONE;
}

ledError_t led_resetAll()
{
  printf(" %s \n",__FUNCTION__);
  return LED_ERR_NONE;
}

ledError_t led_setEnable(ledId_t id, int enable)
{
  printf(" %s id: %d enable: %d\n",__FUNCTION__, id, enable);
  return LED_ERR_NONE;
}

ledError_t led_setColor(ledId_t id, uint8_t R, uint8_t G, uint8_t B)
{
  printf(" %s id: %d R: %d G: %d B: %d\n",__FUNCTION__, id, R, G, B);
  return LED_ERR_NONE;
}

ledError_t led_setBrightness(ledId_t id, uint8_t R, uint8_t G, uint8_t B)
{
  printf(" %s id: %d R: %d G: %d B: %d\n",__FUNCTION__, id, R, G, B);
  return LED_ERR_NONE;
}

ledError_t led_setBlink(ledId_t id, uint32_t ontime, uint32_t offtime)
{
  printf(" %s id: %d ontime: %d offtime: %d\n",__FUNCTION__, id, ontime, offtime);
  return LED_ERR_NONE;
}

ledError_t led_setBlinkSequence(ledId_t id, uint32_t ontime, uint32_t offtime1, uint32_t count, uint32_t offtime2)
{
  printf(" %s id: %d ontime: %d offtime1: %d count: %d offtime2: %d\n",__FUNCTION__, id, ontime, offtime1, count, offtime2);
  return LED_ERR_NONE;
}

ledError_t led_setOnOff(ledId_t id, const char* onoff)
{
  printf(" %s id: %d onoff: %s \n",__FUNCTION__, id, onoff);
  return LED_ERR_NONE;
}

ledError_t led_applySettings(ledId_t id)
{
  printf(" %s id: %d \n",__FUNCTION__, id);
  return LED_ERR_NONE;
}

ledError_t led_applyAllSettings()
{
  printf(" %s \n",__FUNCTION__);
  return LED_ERR_NONE;
}


const char* led_getErrorMsg(ledError_t err)
{
  printf(" %s err: %d\n",__FUNCTION__,err);
  return "stub error message";
}

const char* led_getVersion()
{
  printf(" %s \n",__FUNCTION__);
  return "stub version1";
}
