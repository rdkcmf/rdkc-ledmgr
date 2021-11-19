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
 
#ifndef __LED_MGR__
#define __LED_MGR__

#ifdef __cplusplus
extern "C"
{
#endif

#include "ledhal.h"

/* Led colors */
typedef enum _ledMgrColor_t{
  LED_MGR_COLOR_AMBER = 0,
  LED_MGR_COLOR_WHITE,
  LED_MGR_COLOR_RED,
  LED_MGR_COLOR_GREEN,
  LED_MGR_COLOR_BLUE,
  LED_MGR_COLOR_MAX
}ledMgrColor_t;

/* Led states */
typedef enum _ledMgrState_t{
  LED_MGR_STATE_BOOT_UP = 0,
  LED_MGR_STATE_INCORRECT_XW,
  LED_MGR_STATE_READY_TO_PAIR,
  LED_MGR_STATE_NOT_PROVISIONED,
  LED_MGR_STATE_TROUBLE_CONNECTING,
  LED_MGR_STATE_WORKING_NORMALLY,
  LED_MGR_STATE_2_WAY_VOICE,
  LED_MGR_STATE_FACTORY_DOWNLOAD_MODE,
  LED_MGR_STATE_UNKNOWN
}ledMgrState_t;

/* Operation types */
typedef enum _ledMgrOp_t{
  LED_MGR_OP_SOLID_LIGHT = 0,
  LED_MGR_OP_BLINK,
  LED_MGR_OP_SLOW_BLINK,
  LED_MGR_OP_DOUBLE_BLINK,
  LED_MGR_OP_FAST_BLINK,
  LED_MGR_OP_NO_LIGHT,
  LED_MGR_OP_MAX
}ledMgrOp_t;

/* Error codes */
typedef enum _ledMgrErr_t {
  LED_MGR_ERR_NONE = 0,
  LED_MGR_ERR_BUSY,
  LED_MGR_ERR_GENERAL,
  LED_MGR_ERR_INVALID_PARAM,
  LED_MGR_ERR_OPERATION_NOT_SUPPORTED,
  LED_MGR_ERR_UNKNOWN,
}ledMgrErr_t;

/**
 * @brief Initialize led mgr
 * This API to be called to initialize ledmgr
 *
 * @param [in]  id   :  None
 * @param [out]      :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledMgrErr_t ledmgr_init(void);

/**
 * @brief Set led state
 * This API to be called to set the state of led like boot up, fw update etc
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [out]      :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledMgrErr_t ledmgr_setState(ledMgrState_t state);

/**
 * @brief Set led operation
 * This API to be called to set operation of led like solid/slow/fast/double blink etc
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [out]      :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledMgrErr_t ledmgr_setOp(ledId_t id, ledMgrOp_t op, ledMgrColor_t color);

//int ledmgr_setColor(ledId_t id, ledColor_t color);

#ifdef __cplusplus
}
#endif

#endif //__LED_MGR__

