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
#include "ledmgr.h"
 
static ledMgrState_t bootup_st_handler(void);
static ledMgrState_t incorrect_xw_st_handler(void);
static ledMgrState_t ready_to_pair_st_handler(void);
static ledMgrState_t not_provisioned_st_handler(void);
static ledMgrState_t trouble_connect_st_handler(void);
static ledMgrState_t working_normally_st_handler(void);
static ledMgrState_t two_way_voice_st_handler(void);
static ledMgrState_t incorrect_xw_st_handler(void);
static ledMgrState_t factory_dw_mode_st_handler(void);

static bool is_camera_connected();
static bool is_camera_pairing_mode();
static bool is_camera_not_online();
static bool is_xw_incompatible();
static bool is_camera_not_provisioned();
static bool is_camera_booting_up();
static bool is_voice_out();

ledMgrState_t (*get_next_state[LED_MGR_STATE_UNKNOWN])(void) = {
                    bootup_st_handler,            //LED_MGR_STATE_BOOT_UP
                    incorrect_xw_st_handler,      //LED_MGR_STATE_INCORRECT_XW
                    ready_to_pair_st_handler,     //LED_MGR_STATE_READY_TO_PAIR
                    not_provisioned_st_handler,   //LED_MGR_STATE_NOT_PROVISIONED
                    trouble_connect_st_handler,   //LED_MGR_STATE_TROUBLE_CONNECTING
                    working_normally_st_handler,  //LED_MGR_STATE_WORKING_NORMALLY
                    two_way_voice_st_handler,     //LED_MGR_STATE_2_WAY_VOICE
                    factory_dw_mode_st_handler    //LED_MGR_STATE_FACTORY_DOWNLOAD_MODE
                };

static ledMgrState_t bootup_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_BOOT_UP;

    if(is_xw_incompatible() == true)
    {
        state = LED_MGR_STATE_INCORRECT_XW;
    }
    else if(is_camera_booting_up() == true)
    {
        state = LED_MGR_STATE_BOOT_UP;
    }
    else if(is_camera_pairing_mode() == true)
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(is_camera_connected() == true)
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    else if(is_camera_not_online() == true)
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
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

    if(is_camera_pairing_mode() == true)
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else if(is_camera_connected() && is_camera_not_provisioned())
    {
        state = LED_MGR_STATE_NOT_PROVISIONED;
    }
    else if(is_camera_connected() == true)
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    else if(is_camera_not_online() == true)
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }

    return state;
}

static ledMgrState_t not_provisioned_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_NOT_PROVISIONED;

    if(is_camera_not_provisioned())
    {
        state = LED_MGR_STATE_NOT_PROVISIONED;
    }
    else if(is_camera_connected())
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

    if(is_camera_connected())
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    else if(is_camera_pairing_mode())
    {
        state = LED_MGR_STATE_READY_TO_PAIR;
    }
    else
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }
    return state;
}

static ledMgrState_t working_normally_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_WORKING_NORMALLY;
    
    if(is_camera_not_online())
    {
        state = LED_MGR_STATE_TROUBLE_CONNECTING;
    }
    else if(is_voice_out())
    {
        state = LED_MGR_STATE_2_WAY_VOICE;
    }
    else
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    
    return state;
}

static ledMgrState_t two_way_voice_st_handler(void)
{
    ledMgrState_t state = LED_MGR_STATE_2_WAY_VOICE;

    if(is_camera_connected())
    {
        state = LED_MGR_STATE_WORKING_NORMALLY;
    }
    else if(is_camera_not_online())
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
    return false;   
}

static bool is_camera_pairing_mode()
{
    return false;
}

static bool is_camera_not_online()
{
    return false;
}

static bool is_xw_incompatible()
{
    return false;
}

static bool is_camera_not_provisioned()
{
    return false;
}

static bool is_camera_booting_up()
{
    return false;
}

static bool is_voice_out()
{
    return false;
}

int main(int argc, char* argv[])
{
  int loop = 1;
  ledMgrState_t next_state = LED_MGR_STATE_BOOT_UP;
  ledMgrState_t cur_state = LED_MGR_STATE_UNKNOWN;
  ledMgrErr_t err;

  do
  {
    if(next_state != cur_state)
    {
        err = ledmgr_setState(next_state);
        //handle error 
        cur_state = next_state;
    }
    
    next_state = (*get_next_state[cur_state])();
    printf("next state %d \n", next_state);

    sleep(1); //one second sleep
  }while(loop == 1);

  return 0;
}


