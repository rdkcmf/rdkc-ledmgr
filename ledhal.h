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
 
#ifndef __LED_HAL__
#define __LED_HAL__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Error codes */
typedef enum _ledError_t {
  LED_ERR_NONE = 0,
  LED_ERR_GENERAL,
  LED_ERR_INVALID_PARAM,
  LED_ERR_OPERATION_NOT_SUPPORTED,
  LED_ERR_UNKNOWN,
}ledError_t;

typedef enum _ledId_t {
  LED_ID_CAMERA_FRONT_PANEL = 1,
  LED_ID_XW_FRONT_PANEL,
  LED_ID_CAMERA_IR,
  LED_ID_MAX
}ledId_t;

/**
 * @brief Initialize led.
 * This function should be call once before the functions in this API can be used.
 *
 * @param [in]  :  Identifier of a led.
 * @param [out] :  None.
 *
 * @return Error Code:  If error code is returned then initialization has failed.
 */
ledError_t led_init(ledId_t id);

/**
 * @brief Reset a led to default.
 *
 * @param [in]  :  Identifier of a led.
 * @param [out] :  None.
 *
 * @return Error Code:  If error code is returned then reset failed.
 */
ledError_t led_reset(ledId_t id);

/**
 * @brief Reset all leds to default.
 *
 * @param [in]  :  Identifier of a led.
 * @param [out] :  None.
 *
 * @return Error Code:  If error code is returned then resetall failed.
 */
ledError_t led_resetAll();

/**
 * @brief Enable or disable a led.
 *
 * @param [in]  id  :  Identifier of a led.
 * @param [in]  enable:  1 - enable, 0 - disable
 * @param [out]     :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_setEnable(ledId_t id, int enable);

/**
 * @brief Set color to a led.
 *
 * @param [in]  id:  Identifier of a led.
 * @param [in]  R :  0-255 Red value
 * @param [in]  G :  0-255 Green value
 * @param [in]  B :  0-255 Blue value
 * @param [out]   :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_setColor(ledId_t id, uint8_t R, uint8_t G, uint8_t B);

/**
 * @brief Set brightness value.
 *
 * @param [in]  id:  Identifier of a led.
 * @param [in]  R :  0-255 Red value
 * @param [in]  G :  0-255 Green value
 * @param [in]  B :  0-255 Blue value
 * @param [out]   :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_setBrightness(ledId_t id, uint8_t R, uint8_t G, uint8_t B);

/**
 * @brief Set led to blink.
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [in]  ontime :  on time in milliseconds (ms)
 * @param [in]  offtime:  off time in milliseconds (ms)
 * @param [out]    :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_setBlink(ledId_t id, uint32_t ontime, uint32_t offtime);

/**
 * @brief Set led blink sequence. This API to be used to blink a led in a sequence like double blink, triple blink etc
 *
 * @param [in]  id    :  Identifier of a led.
 * @param [in]  ontime  :  blink on time in milliseconds (ms)
 * @param [in]  offtime1:  blink off time in milliseconds (ms)
 * @param [in]  count   :  number of times to repeat (double, triple etc)
 * @param [in]  offtime1:  long wait time after blink in milliseconds (ms)
 * @param [out]     :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_setBlinkSequence(ledId_t id, uint32_t ontime, uint32_t offtime1, uint32_t count, uint32_t offtime2);

//ledError_t led_setEngineMode(unsigned int index, char* mode); //mode is RGB/W

/**
 * @brief Set led on or off
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [in]  onoff:  "on" or "off"
 * @param [out]    :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_setOnOff(ledId_t id, const char* onoff);

/**
 * @brief Apply configured settings for a led.
 * This API to be called after doing required configuration for led and this will apply/run the engine.
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [out]    :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_applySettings(ledId_t id);

/**
 * @brief Apply configured settings for all led's
 * This API to be called after doing required configuration for all leds and this will apply/run the engine.
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [out]    :  None.
 *
 * @return Error Code:  If error code is returned then failed.
 */
ledError_t led_applyAllSettings();

/**
 * @brief Gets error message for an error code
 * This API to be called to get error message for an error code
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [out]    :  Error message.
 *
 * @return Error Code:  If error code is returned then failed.
 */
const char* led_getErrorMsg(ledError_t err);

/**
 * @brief Gets led hal version
 * This API to be called to get version of led hal
 *
 * @param [in]  id   :  Identifier of a led.
 * @param [out]    :  Error message.
 *
 * @return Error Code:  If error code is returned then failed.
 */
const char* led_getVersion();

#ifdef __cplusplus
}
#endif

#endif //__LED_HAL__
