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
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "ledhal.h"
#include "sc_tool.h"
#include "i2c_test.h"

#include "ledmgrlogger.h"

extern "C"
{
#include "secure_wrapper.h"
}

//config file path
#define LED_CONFIG_FILE_PATH "/mnt/ramdisk/tmp/"
#define LED_CONFIG_FILE_PREFIX ".LED_config_id_"
#define LED_CONFIG_FILE_NAME_LENGTH 128

//some features of LP5562
#define LED_LP5562_WAIT_CMD_MAX_STEP 63
#define LED_LP5562_WAIT_CMD_STEP_TIME 156 //15.6ms * 10
#define LED_LP5562_BRANCH_CMD_MAX_LOOP 63
//16*2+1,every channel support 16 commands, every command consist of 2 bytes, 
//and we use HEX to represent value of every byte,and the command is a string, so reserve 1 byte for end of the string.
#define LED_LP5562_COMMAND_LEN	65

//LED default state
#define LED_CONFIG_DEF_CAMERA_FRONT_PANEL_STATE 1
#define LED_CONFIG_DEF_XW_FRONT_PANEL_STATE 1
#define LED_CONFIG_DEF_IRLED_STATE 1

//CAMERA_FRONT_PANEL LED default current and pwm
#define LED_CONFIG_DEF_CAMERA_FRONT_PANEL_R_CURRENT 255
#define LED_CONFIG_DEF_CAMERA_FRONT_PANEL_R_PWM 53
#define LED_CONFIG_DEF_CAMERA_FRONT_PANEL_G_CURRENT 198
#define LED_CONFIG_DEF_CAMERA_FRONT_PANEL_G_PWM 80
#define LED_CONFIG_DEF_CAMERA_FRONT_PANEL_B_CURRENT 160
#define LED_CONFIG_DEF_CAMERA_FRONT_PANEL_B_PWM 60

//XW_FRONT_PANEL LED deafult current and pwm
#define LED_CONFIG_DEF_XW_FRONT_PANEL_R_CURRENT 255
#define LED_CONFIG_DEF_XW_FRONT_PANEL_R_PWM 53
#define LED_CONFIG_DEF_XW_FRONT_PANEL_G_CURRENT 198
#define LED_CONFIG_DEF_XW_FRONT_PANEL_G_PWM 80
#define LED_CONFIG_DEF_XW_FRONT_PANEL_B_CURRENT 160
#define LED_CONFIG_DEF_XW_FRONT_PANEL_B_PWM 60

//IR LED default brightness
#define LED_CONFIG_DEF_IRLED_BRIGHTNESS 80

//LP5562 device path
#define LED_LP5562_DEVICE_PATH "/sys/devices/e8000000.apb/e8007000.i2c/i2c-2/2-0030"
#define LED_LP5562_I2C_DEVICE "/dev/i2c-2"

//AW210XX device path
#define LEDS_CHIP_AW210XX_FILE "/tmp/.led_aw210xx"
#define RGBCOLOR  "/sys/devices/e8000000.apb/e8007000.i2c/i2c-2/2-0020/leds/aw210xx_led/rgbcolor"
#define REG  "/sys/devices/e8000000.apb/e8007000.i2c/i2c-2/2-0020/leds/aw210xx_led/reg"

//current register address of LP5562
#define LED_LP5562_R_CURRENT_REG 0x07
#define LED_LP5562_G_CURRENT_REG 0x06
#define LED_LP5562_B_CURRENT_REG 0x05

//IR LED brightness file
#define LED_IRLED_BRIGHTNESS_FILE "/sys/class/backlight/0.pwm_bl/brightness"

#define LED_ERROR_MSG_MAX_LENGTH 128
static char error_msg[LED_ERR_UNKNOWN+1][LED_ERROR_MSG_MAX_LENGTH] = {{0}};

#define LED_HAL_API_VERSION "V1.0.01"

/**
 * @Led action type
 * There are 4 action types, Led_ON, Led_OFF, Led_BLINK and Led_SEQ_BLINK
 * Led_ON        :  turn on Led
 * Led_OFF       :  turn off Led
 * Led_BLINK     :  Led take on/off blink
 * Led_SEQ_BLINK :  firstly, Led repeat on/off1 blink as required times, and then turn off Led off2 milliseconds, and then repeat the whole process
*/
typedef enum Action_Type {Led_ON,Led_OFF,Led_BLINK,Led_SEQ_BLINK} Action_Type;

/**
 * @brief Led action structure
 * This structure define Led action, the definition include action type, on time, off time for action blink, on time, off1 time, count and off2 time for action
 * sequence blink.
 * @member varible act_type  : action type
 * @member varible on_time   : Led on time for action blink and sequence blink
 * @member varible off_time  : Led off time for action blink
 * @member varible off1_time : Led off1 time for sequence blink
 * @member varible count     : repeat number of on_time:off1_time
 * @member varible off2_time : Led off2 time
*/
typedef struct Led_Action{
	Action_Type act_type;
	uint32_t on_time;
	uint32_t off_time;
	uint32_t off1_time;
	uint32_t count;
	uint32_t off2_time;
}Led_Action;

/**
 * @brief channel of LED chip LP5562
 * This structure define a description of a LP5562's channel.
 * @member variable current  : current of the channel
 * @member variable pwm      : pwm of the channel
*/
typedef struct LEDCHIP_Channel{
    uint8_t current;
    uint8_t pwm;
}LEDCHIP_Channel;

/**
 * @brief LED chip LP5562
 * This structure define a descrption of LED chip LP5562.
 * LP5562 has 3 channels, every channel control different color.
 * we set channel[0] control Red, channel[1] control Green, channel[2] control Blue.
 * @member variable channel : channels of LP5562
*/
typedef struct Led_CHIP{
    LEDCHIP_Channel channel[3]; //LED chip LP5562 has 3 channels
}Led_CHIP;

/**
 * @brief LED IR LED
 * This structure define a description of IR LED
 * @member variable brightness  : brightness of IR LED, 0~255
 * @member variable command     : command of IR LED, 0 -- turn on, 1 -- turn off
*/
typedef struct Led_IRLED{
    uint8_t brightness;
}Led_IRLED;

/**
 * @brief Config of LED
 * This structure define config of LED
 * member variable state       : state of LED, 0 -- disable, 1 -- disable, if LED is disabled, you cannot set it's config
 * member variable led_chip  : config of LED chip  
 * member variable led_irled   : config of IR LED
*/
typedef struct Led_Config{
    int state; //0:disable, 1:enable
    Led_CHIP led_chip;
    Led_IRLED led_irled;
	Led_Action	action; 
}Led_Config;

/**
 * @brief Lock LED config.
 * This function used to add a lock on LED config file.
 *
 * @param [in]  config_fd:  LED config file descriptor.
 * @param [out]          :  None.
 *
 * @return               :  0 success, other value failed.
 */
static int lock_led_config(int config_fd)
{
    int try_times = 3;
    int ret = 0;
  
    //try to lock
    do
    {
        ret = flock(config_fd,LOCK_EX|LOCK_NB);
        if (0 != ret)
        {
            if (EWOULDBLOCK != errno)
            {
                break;
            }
            usleep(5000);
        }
        try_times--;
    }while(ret && (try_times > 0));

    return ret;
}

/**
 * @brief read LED config.
 * This function used to read LED config file.
 *
 * @param [in]  config_fd    :  LED config file descriptor.
 * @param [out] p_led_config :  pointer of LED config.
 *
 * @return                   :  0 success, other value failed.
 */
static int read_led_config(int config_fd, Led_Config* p_led_config)
{
    int read_len = 0;
    int total_len = 0;

    //try to read fully
    lseek(config_fd, 0, SEEK_SET);
    do
    {
        read_len = read(config_fd, p_led_config + total_len, sizeof(Led_Config) - total_len);
        if (-1 == read_len)
        {
            if (EINTR == errno)
            {
                continue;
            }
            else
            {
                return -1;
            }
        }
        else if (0 == read_len)
        {
            break;
        }
        else
        {
            total_len += read_len;
        }
    }while(total_len < sizeof(Led_Config));

    return 0;
}

/**
 * @brief write LED config.
 * This function used to write LED config file.
 *
 * @param [in]  config_fd    :  LED config file descriptor.
 * @param [in]  p_led_config :  pointer of LED config.
 * @param [out]              :  None
 *
 * @return                   :  0 success, other value failed.
 */
static int write_led_config(int config_fd, Led_Config* p_led_config)
{
    int write_len = 0;
    int total_len = 0;

    //try to write fully
    lseek(config_fd, 0, SEEK_SET);
    do
    {
        write_len = write(config_fd, p_led_config + total_len, sizeof(Led_Config) - total_len);
        if (-1 == write_len)
        {
            if (EINTR == errno)
            {
                continue;
            }
            else
            {
                return -1;
            }
        }
        else
        {
            total_len += write_len;
        }
    }while(total_len < sizeof(Led_Config));

    return 0;
}

ledError_t led_init(ledId_t id)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    Led_Config led_config;
    int config_fd = -1;
    int ret = 0;
    struct stat config_file_state;

    LEDMGR_LOG_DEBUG(" %s id: %d\n",__FUNCTION__, id);
  
    //check parameter
    if ((LED_ID_CAMERA_FRONT_PANEL != id) &&(LED_ID_XW_FRONT_PANEL != id) && (LED_ID_CAMERA_IR != id))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
        return LED_ERR_INVALID_PARAM;
    }

    //check if the config file exist or not, and check led config has been writen in the file or not
	snprintf(led_config_file,LED_CONFIG_FILE_NAME_LENGTH,"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    if (!access(led_config_file,F_OK))
    {
        ret = stat(led_config_file,&config_file_state);
        if (0 != ret)
        {
            snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, get config file stat error\n",__FUNCTION__,__LINE__);
            return LED_ERR_UNKNOWN;
        }
        else
        {
            if (sizeof(Led_Config) == config_file_state.st_size)
            {
                //LED config has been created
                return LED_ERR_NONE;
            }
        }
    }
    
    if (LED_ID_CAMERA_FRONT_PANEL == id)
    {
        led_config.state = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_STATE;
        //R channel
        led_config.led_chip.channel[0].current = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_R_CURRENT;
        led_config.led_chip.channel[0].pwm = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_R_PWM;
        //G channel
        led_config.led_chip.channel[1].current = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_G_CURRENT;
        led_config.led_chip.channel[1].pwm = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_G_PWM;
        //B channel
        led_config.led_chip.channel[2].current = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_B_CURRENT;
        led_config.led_chip.channel[2].pwm = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_B_PWM;
		//default, turn on xCam2 front Led
		led_config.action.act_type = Led_ON;
    }
    else if (LED_ID_XW_FRONT_PANEL == id)
    {
        led_config.state = LED_CONFIG_DEF_XW_FRONT_PANEL_STATE;
        //R channel
        led_config.led_chip.channel[0].current = LED_CONFIG_DEF_XW_FRONT_PANEL_R_CURRENT;
        led_config.led_chip.channel[0].pwm = LED_CONFIG_DEF_XW_FRONT_PANEL_R_PWM;
        //G channel
        led_config.led_chip.channel[1].current =  LED_CONFIG_DEF_XW_FRONT_PANEL_G_CURRENT;
        led_config.led_chip.channel[1].pwm = LED_CONFIG_DEF_XW_FRONT_PANEL_G_PWM;
        //B channel
        led_config.led_chip.channel[2].current =  LED_CONFIG_DEF_XW_FRONT_PANEL_B_CURRENT;
        led_config.led_chip.channel[2].pwm = LED_CONFIG_DEF_XW_FRONT_PANEL_B_PWM;
		//defalut, turn on XW4 front Led
		led_config.action.act_type = Led_ON;
    }
    else
    {
        led_config.state = LED_CONFIG_DEF_IRLED_STATE;
        led_config.led_irled.brightness = LED_CONFIG_DEF_IRLED_BRIGHTNESS;
        //default, turn on IR Led;
		led_config.action.act_type = Led_ON;
    }
  
    //open LED config file
    config_fd = open(led_config_file,O_WRONLY|O_CREAT|O_EXCL|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }

    //lock
    if(lock_led_config(config_fd))
    {
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, close config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    } 

    //write init config    
    ret = write_led_config(config_fd,&led_config);
    //unlock
    flock(config_fd,LOCK_UN);
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

ledError_t led_reset(ledId_t id)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    Led_Config led_config;
    int config_fd = -1;
    int ret = 0;

    //check parameter
    if ((LED_ID_CAMERA_FRONT_PANEL != id) &&(LED_ID_XW_FRONT_PANEL != id) && (LED_ID_CAMERA_IR != id))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
        return LED_ERR_INVALID_PARAM;
    }
    
    //default config
    if (LED_ID_CAMERA_FRONT_PANEL == id)
    {
        led_config.state = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_STATE;
        //R channel
        led_config.led_chip.channel[0].current = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_R_CURRENT;
        led_config.led_chip.channel[0].pwm = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_R_PWM;
        //G channel
        led_config.led_chip.channel[1].current = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_G_CURRENT;
        led_config.led_chip.channel[1].pwm = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_G_PWM;
        //B channel
        led_config.led_chip.channel[2].current = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_B_CURRENT;
        led_config.led_chip.channel[2].pwm = LED_CONFIG_DEF_CAMERA_FRONT_PANEL_B_PWM;
		//default, turn on xCam2 front Led
		led_config.action.act_type = Led_ON;
    }
    else if (LED_ID_XW_FRONT_PANEL == id)
    {
        led_config.state = LED_CONFIG_DEF_XW_FRONT_PANEL_STATE;
        //R channel
        led_config.led_chip.channel[0].current = LED_CONFIG_DEF_XW_FRONT_PANEL_R_CURRENT;
        led_config.led_chip.channel[0].pwm = LED_CONFIG_DEF_XW_FRONT_PANEL_R_PWM;
        //G channel
        led_config.led_chip.channel[1].current =  LED_CONFIG_DEF_XW_FRONT_PANEL_G_CURRENT;
        led_config.led_chip.channel[1].pwm = LED_CONFIG_DEF_XW_FRONT_PANEL_G_PWM;
        //B channel
        led_config.led_chip.channel[2].current =  LED_CONFIG_DEF_XW_FRONT_PANEL_B_CURRENT;
        led_config.led_chip.channel[2].pwm = LED_CONFIG_DEF_XW_FRONT_PANEL_B_PWM;
		//default, turn on XW4 front Led
		led_config.action.act_type = Led_ON;
    }
    else
    {
        led_config.state = LED_CONFIG_DEF_IRLED_STATE;
        led_config.led_irled.brightness = LED_CONFIG_DEF_IRLED_BRIGHTNESS;
        //default, turn on IR Led
		led_config.action.act_type = Led_ON;
    }

    //open LED config file
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_WRONLY|O_CREAT|O_TRUNC);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
  
    //lock
    if(lock_led_config(config_fd))
    {
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    
    //write LED config
    ret = write_led_config(config_fd,&led_config);
    //unlock
    flock(config_fd,LOCK_UN);
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

ledError_t led_resetAll()
{
    int id = LED_ID_CAMERA_FRONT_PANEL;
    ledError_t ret = LED_ERR_NONE;

    LEDMGR_LOG_DEBUG(" %s \n",__FUNCTION__);
    for (id = LED_ID_CAMERA_FRONT_PANEL; id < LED_ID_MAX; id++)
    {
        ret = led_reset((ledId_t)id);
        if (LED_ERR_NONE != ret)
        {
            LEDMGR_LOG_ERROR(" %s id: %d reset failed, Error %d\n",__FUNCTION__, id, ret);
            return ret;
        }
    }  

    return LED_ERR_NONE;
}

ledError_t led_setEnable(ledId_t id, int enable)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    int config_fd = -1;
    Led_Config led_config;
    int ret = -1;

    LEDMGR_LOG_DEBUG(" %s id: %d enable: %d\n",__FUNCTION__, id, enable);

    //check parameter
    if ((LED_ID_CAMERA_FRONT_PANEL != id) &&(LED_ID_XW_FRONT_PANEL != id) && (LED_ID_CAMERA_IR != id))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
        return LED_ERR_INVALID_PARAM;
    }

    if ((1 != enable) && (0 != enable))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] enable value %d is illegal\n",__FUNCTION__,__LINE__,enable);
        return LED_ERR_INVALID_PARAM;
    }

    //open LED config file
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_RDWR); 
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //lock
    if (lock_led_config(config_fd))
    {
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //read LED config
    if (read_led_config(config_fd,&led_config))
    {
        flock(config_fd, LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, read config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //update LED config
    led_config.state = enable;
    //write LED config
    ret = write_led_config(config_fd,&led_config);
    //unlock
    flock(config_fd,LOCK_UN);
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

ledError_t led_setColor(ledId_t id, uint8_t R, uint8_t G, uint8_t B)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    int config_fd = -1;
    Led_Config led_config;
    int ret = -1;
    
    LEDMGR_LOG_DEBUG(" %s id: %d R: %d G: %d B: %d\n",__FUNCTION__, id, R, G, B);
    
    //check parameter
    if ((LED_ID_CAMERA_FRONT_PANEL != id) &&(LED_ID_XW_FRONT_PANEL != id))
    {
        if (LED_ID_CAMERA_IR == id)
        {
            snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d does not support set color\n",__FUNCTION__,__LINE__,id);
            return LED_ERR_OPERATION_NOT_SUPPORTED;
        }
        else
        {
            snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
            return LED_ERR_INVALID_PARAM;
        } 
    }

    //open LED config file
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_RDWR);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //lock
    if (lock_led_config(config_fd))
    {
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //read LED config
    if (read_led_config(config_fd,&led_config))
    {
        flock(config_fd,LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, read config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //check state
    if (!led_config.state)
    {
        flock(config_fd,LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] led is disabled, does not support configure\n",__FUNCTION__,__LINE__);
        return LED_ERR_OPERATION_NOT_SUPPORTED;
    }
    //update LED config
    led_config.led_chip.channel[0].current = R;
    led_config.led_chip.channel[1].current = G;
    led_config.led_chip.channel[2].current = B;
    //write LED config
    ret = write_led_config(config_fd,&led_config);
    flock(config_fd,LOCK_UN);//unlock
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

ledError_t led_setBrightness(ledId_t id, uint8_t R, uint8_t G, uint8_t B)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    int config_fd = -1;
    Led_Config led_config;
    int ret = -1;

    LEDMGR_LOG_DEBUG(" %s id: %d R: %d G: %d B: %d\n",__FUNCTION__, id, R, G, B);

     //check parameter
    if ((LED_ID_CAMERA_FRONT_PANEL != id) &&(LED_ID_XW_FRONT_PANEL != id) && (LED_ID_CAMERA_IR != id))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
        return LED_ERR_INVALID_PARAM;
    }

    //open LED config file
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_RDWR);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //lock
    if (lock_led_config(config_fd))
    {
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //read LED config
    if (read_led_config(config_fd,&led_config))
    {
        flock(config_fd, LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, read config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //check LED state
    if (!led_config.state)
    {
        flock(config_fd, LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] led is disabled, does not support configure\n",__FUNCTION__,__LINE__);

        return LED_ERR_OPERATION_NOT_SUPPORTED;
    }
    //update LED config
    if ((LED_ID_CAMERA_FRONT_PANEL == id) || (LED_ID_XW_FRONT_PANEL == id))
    {
        led_config.led_chip.channel[0].pwm = R;
        led_config.led_chip.channel[1].pwm = G;
        led_config.led_chip.channel[2].pwm = B;
    }
    else
    {
        led_config.led_irled.brightness = R;
    }
    //write LED config
    ret = write_led_config(config_fd,&led_config);
    flock(config_fd, LOCK_UN);
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

ledError_t led_setBlink(ledId_t id, uint32_t ontime, uint32_t offtime)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    int config_fd = -1;
    Led_Config led_config;
    int ret = -1;

    LEDMGR_LOG_DEBUG(" %s id: %d ontime: %d offtime: %d\n",__FUNCTION__, id, ontime, offtime);

    //check parameter
    if ((LED_ID_CAMERA_FRONT_PANEL != id) && (LED_ID_XW_FRONT_PANEL != id))
    {
        if (LED_ID_CAMERA_IR == id)
        {
            snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d does not support blink action\n",__FUNCTION__,__LINE__,id);
            return LED_ERR_OPERATION_NOT_SUPPORTED;
        }
        else
        {
            snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
            return LED_ERR_INVALID_PARAM;
        }
    }

    if ((ontime*10) > (LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_BRANCH_CMD_MAX_LOOP))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] ontime value %d is illegal\n",__FUNCTION__,__LINE__,ontime);
        return LED_ERR_INVALID_PARAM;
    } 
    if ((offtime*10) > (LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_BRANCH_CMD_MAX_LOOP))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] offtime value %d is illegal\n",__FUNCTION__,__LINE__,offtime);
        return LED_ERR_INVALID_PARAM;
    }

    //open config file
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_RDWR);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //lock
    if (lock_led_config(config_fd))
    {
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //read LED config
    if (read_led_config(config_fd,&led_config))
    {
        flock(config_fd,LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, read config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //check LED state
    if (!led_config.state)
    {
        flock(config_fd,LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] led is disabled, does not support configure\n",__FUNCTION__,__LINE__);
        return LED_ERR_OPERATION_NOT_SUPPORTED;
    }

	led_config.action.act_type = Led_BLINK;
    led_config.action.on_time = ontime;
	led_config.action.off_time = offtime;

	//write config
    ret = write_led_config(config_fd, &led_config);
    flock(config_fd,LOCK_UN);
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

ledError_t led_setBlinkSequence(ledId_t id, uint32_t ontime, uint32_t offtime1, uint32_t count, uint32_t offtime2)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    int config_fd = -1;
    Led_Config led_config;
    int ret = -1;

    LEDMGR_LOG_DEBUG(" %s id: %d ontime: %d offtime1: %d count: %d offtime2: %d\n",__FUNCTION__, id, ontime, offtime1, count, offtime2);
    //check parameters
    if ((LED_ID_CAMERA_FRONT_PANEL != id) && (LED_ID_XW_FRONT_PANEL != id))
    {
        if (LED_ID_CAMERA_IR == id)
        {
            snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d does not support set sequence blink\n",__FUNCTION__,__LINE__,id);
            return LED_ERR_OPERATION_NOT_SUPPORTED;
        }
        else
        {
            snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
            return LED_ERR_INVALID_PARAM;
        }
    }

    if ((ontime*10) > (LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_BRANCH_CMD_MAX_LOOP)) 
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] ontime value %d is illegal\n",__FUNCTION__,__LINE__,ontime);
        return LED_ERR_INVALID_PARAM;
    }

    if ((offtime1*10) > (LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_BRANCH_CMD_MAX_LOOP))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] offtime1 value %d is illegal\n",__FUNCTION__,__LINE__,offtime1);
        return LED_ERR_INVALID_PARAM;
    } 

    if ((offtime2*10) > (LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_BRANCH_CMD_MAX_LOOP))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] offtime2 value %d is illegal\n",__FUNCTION__,__LINE__,offtime2);
        return LED_ERR_INVALID_PARAM;
    }

    if(count > (LED_LP5562_BRANCH_CMD_MAX_LOOP+1))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] count value %d is illegal\n",__FUNCTION__,__LINE__,count);
        return LED_ERR_INVALID_PARAM;
    }
  
    //open LED config file
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_RDWR);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //lock
    if (lock_led_config(config_fd))
    {
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //read LED config file
    if (read_led_config(config_fd,&led_config))
    {
        flock(config_fd, LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, read config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    //check LED state
    if (!led_config.state)
    {
        flock(config_fd, LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] led is disabled, does not support configure\n",__FUNCTION__,__LINE__);

        return LED_ERR_OPERATION_NOT_SUPPORTED;
    }

	led_config.action.act_type = Led_SEQ_BLINK;
	led_config.action.on_time = ontime;
	led_config.action.off1_time = offtime1;
	led_config.action.count = count;
	led_config.action.off2_time = offtime2;
    
	ret = write_led_config(config_fd, &led_config);
    flock(config_fd,LOCK_UN);
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

ledError_t led_setOnOff(ledId_t id, const char* onoff)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    int config_fd = -1;
    Led_Config led_config;
    int ret = -1;

    LEDMGR_LOG_DEBUG(" %s id: %d onoff: %s \n",__FUNCTION__, id, onoff);
    //check parameter
    if ((LED_ID_CAMERA_FRONT_PANEL != id) &&(LED_ID_XW_FRONT_PANEL != id) && (LED_ID_CAMERA_IR != id))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
        return LED_ERR_INVALID_PARAM;
    }

    if ((strcmp(onoff,"on")) && (strcmp(onoff,"off")))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] onoff value %s is illegal\n",__FUNCTION__,__LINE__,onoff);
        return LED_ERR_INVALID_PARAM;
    }

    //open LED config file
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_RDWR);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }

    //lock LED config file
    if (lock_led_config(config_fd))
    {
        //lock failed
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }

    //read LED config
    if (read_led_config(config_fd,&led_config))
    {
        //read LED config failed
        flock(config_fd,LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, read config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }

    //check LED state
    if (!led_config.state)
    {
        //disable state, do not support change config
        flock(config_fd,LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_OPERATION_NOT_SUPPORTED],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] led is disabled, does not support configure\n",__FUNCTION__,__LINE__);

        return LED_ERR_OPERATION_NOT_SUPPORTED;;
    }
    
    //update config
    if (!strcmp(onoff,"on"))
    {
        //on
		led_config.action.act_type = Led_ON;
    }
    else
    {
        //off
		led_config.action.act_type = Led_OFF;
    }
    //write LED config file
    ret = write_led_config(config_fd,&led_config);
    flock(config_fd,LOCK_UN);
    close(config_fd);
    if (0 == ret)
    {
        return LED_ERR_NONE;
    }
    else
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, write config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
}

static int led_apply_irled_setting(Led_Config *led_config)
{
    char bightness_buf[8] = {0};
    int brightness_fd = -1;
    int try_times = 3;
    int ret = -1;

    //open brightness file
    do
    {
        brightness_fd = open(LED_IRLED_BRIGHTNESS_FILE,O_WRONLY);
        if (brightness_fd < 0)
        {
            usleep(5000);
        }
        try_times--;
    }while((brightness_fd < 0) && (try_times > 0));

    if (brightness_fd < 0)
    {
        //open brightness file failed
        return -1;
    }

    if (Led_ON == led_config->action.act_type)
    {
        snprintf(bightness_buf,sizeof(bightness_buf),"%d",led_config->led_irled.brightness);
    }
    else
    {
        snprintf(bightness_buf,sizeof(bightness_buf),"0");
    }

    //write brightness file
    ret = write(brightness_fd,bightness_buf,strlen(bightness_buf));
    close(brightness_fd);

    if (strlen(bightness_buf) != ret)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static void led_transfer_command_to_lp5562_program(Led_Config *led_config, char program[3][LED_LP5562_COMMAND_LEN])
{
	int index = 0;
	uint16_t command_num = 0;
	uint16_t on_wait_loop_time = 0;
	uint16_t on_wait_remainder_step = 0;
	uint16_t off_wait_loop_time = 0;
	uint16_t off_wait_remainder_step = 0;
	uint16_t off_wait1_loop_time = 0;
	uint16_t off_wait1_remainder_step = 0;
	uint16_t off_wait2_loop_time = 0;
	uint16_t off_wait2_remainder_step = 0;
	uint16_t count_repeat = 0;
	uint16_t loop_num = 0;

	memset(program,0,sizeof(char)*3*LED_LP5562_COMMAND_LEN);

	if (Led_BLINK == led_config->action.act_type)
	{
		//transfer blink to LP5562 command
    	on_wait_loop_time = (led_config->action.on_time*10) / (LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_WAIT_CMD_STEP_TIME);
    	on_wait_remainder_step = ((led_config->action.on_time*10) % (LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_WAIT_CMD_STEP_TIME)) / LED_LP5562_WAIT_CMD_STEP_TIME;

    	off_wait_loop_time = (led_config->action.off_time*10) / (LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_WAIT_CMD_STEP_TIME);
    	off_wait_remainder_step = ((led_config->action.off_time*10) % (LED_LP5562_WAIT_CMD_MAX_STEP * LED_LP5562_WAIT_CMD_STEP_TIME)) / LED_LP5562_WAIT_CMD_STEP_TIME;
    	for (index = 0; index < 3; index++)
    	{
			command_num = 0;

        	snprintf(program[index],LED_LP5562_COMMAND_LEN,"40%02X",led_config->led_chip.channel[index].pwm);//turn on
        	command_num++;
        	if (on_wait_loop_time > 0)
        	{
            	//wait
            	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",LED_LP5562_WAIT_CMD_MAX_STEP | (1<<6));
            	command_num++;
            	if (on_wait_loop_time > 1)
            	{
                	//loop
                	loop_num = on_wait_loop_time;
                	loop_num = (loop_num - 1) << 7;
                	loop_num = loop_num | 0xA000 | (command_num -1);
                	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%04X",loop_num);
                	command_num++;
            	}
        	}
        	if (on_wait_remainder_step > 0)
        	{
            	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",on_wait_remainder_step | (1<<6));
            	command_num++;
        	}

        	//turn off
        	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"4000");
        	command_num++;
        	if (off_wait_loop_time > 0)
        	{
            	//wait
            	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",LED_LP5562_WAIT_CMD_MAX_STEP | (1<<6));
            	command_num++;
            	if (off_wait_loop_time > 1)
            	{
                	loop_num = off_wait_loop_time;

                	loop_num = (loop_num - 1) << 7;
                	loop_num = loop_num | 0xA000 | (command_num -1);
                	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%04X",loop_num);
                	command_num++;
            	}
        	}
        	if (off_wait_remainder_step > 0)
        	{
            	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",off_wait_remainder_step | (1<<6));
            	command_num++;
        	}

        	//goto start
        	snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"A000D000");
    	}		
	}
	else if (Led_SEQ_BLINK == led_config->action.act_type)
	{
		on_wait_loop_time = (led_config->action.on_time * 10)/(LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP);
		on_wait_remainder_step = ((led_config->action.on_time * 10)%(LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP)) / LED_LP5562_WAIT_CMD_STEP_TIME;
		off_wait1_loop_time = (led_config->action.off1_time * 10)/(LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP);
		off_wait1_remainder_step = ((led_config->action.off1_time * 10)%(LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP)) / LED_LP5562_WAIT_CMD_STEP_TIME;

		off_wait2_loop_time = (led_config->action.off2_time * 10)/(LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP);
		off_wait2_remainder_step = ((led_config->action.off2_time * 10)%(LED_LP5562_WAIT_CMD_STEP_TIME * LED_LP5562_WAIT_CMD_MAX_STEP)) / LED_LP5562_WAIT_CMD_STEP_TIME;
		for (index = 0; index < 3; index++)
		{

			command_num = 0;
			snprintf(program[index],LED_LP5562_COMMAND_LEN,"40%02X",led_config->led_chip.channel[index].pwm);//turn on
			command_num++;
			if (on_wait_loop_time > 0)
			{
				//wait
				snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",LED_LP5562_WAIT_CMD_MAX_STEP | (1<<6));
				command_num++;
				if (on_wait_loop_time > 1)
				{
					//loop wait command
					loop_num = on_wait_loop_time;
					loop_num = (loop_num - 1) << 7;
					loop_num = loop_num | 0xA000 | (command_num - 1);
					snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%04X",loop_num);
					command_num++;
				}
			}

			if (on_wait_remainder_step)
			{
				snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",on_wait_remainder_step | (1<<6));
				command_num++;
			}

			//turn off1
			snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"4000");
			command_num++;
			if (off_wait1_loop_time > 0)
			{
				//wait
				snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",LED_LP5562_WAIT_CMD_MAX_STEP | (1<<6));
				command_num++;
				if (off_wait1_loop_time > 1)
				{
					loop_num = off_wait1_loop_time;
					loop_num = (loop_num - 1) << 7;
					loop_num = loop_num | 0xA000 | (command_num - 1);
					snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%04X",loop_num);
					command_num++;
				}
			}

			if (off_wait1_remainder_step > 0)
			{
				snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",off_wait1_remainder_step | (1<<6));
				command_num++;
			}

			//count
			if (led_config->action.count > 1)
			{
				count_repeat = (uint16_t)(led_config->action.count-1);
				count_repeat = count_repeat << 7;
				count_repeat = count_repeat | 0xA000;
				snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%04X",count_repeat);
				command_num++;
			}

			//turn off2
			snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"4000");
			command_num++;
			if (off_wait2_loop_time > 0)
			{
				//wait
				snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",LED_LP5562_WAIT_CMD_MAX_STEP | (1<<6));
				command_num++;
				if (off_wait2_loop_time > 1)
				{
					loop_num = off_wait2_loop_time;

					loop_num = (loop_num - 1) << 7;
					loop_num = loop_num | 0xA000 | (command_num - 1);
					snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%04X",loop_num);
					command_num++;
				}
			}
			if (off_wait2_remainder_step > 0)
			{
				snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"%02X00",off_wait2_remainder_step | (1<<6));
				command_num++;
			}

			//goto start
			snprintf(program[index]+strlen(program[index]),LED_LP5562_COMMAND_LEN-strlen(program[index]),"A000D000");
		}
	}
	else if (Led_OFF == led_config->action.act_type)
	{
		for (index = 0; index < 3; index++)
		{
			snprintf(program[index],LED_LP5562_COMMAND_LEN,"4000D000");
		}
	}
	else
	{
		for (index = 0; index < 3; index++)
		{
			snprintf(program[index],LED_LP5562_COMMAND_LEN,"40%02XD000",led_config->led_chip.channel[index].pwm);
		}
	}

	return;
}

static int led_apply_lp5562_setting(Led_Config *led_config)
{
    char command[512] = {0};
	char lp5562_program[3][LED_LP5562_COMMAND_LEN] = {{0}};
    int lp5562_fd = -1;
    long funcs = 0;
    int try_times = 3;
    int i = 0;

	//transfer command to LP5562's program
	led_transfer_command_to_lp5562_program(led_config,lp5562_program);

	//apply current to LP5562
    do
    {
        lp5562_fd = open(LED_LP5562_I2C_DEVICE,O_RDWR);
        if (lp5562_fd < 0)
        {
            usleep(5000);
        }
        try_times--;
    }while((lp5562_fd < 0) && (try_times > 0));

    if (lp5562_fd < 0)
    {
        LEDMGR_LOG_ERROR("open  led I2C error\n");
        return -1;
    }

    if ((ioctl(lp5562_fd, I2C_FUNCS, &funcs) >=0) && (ioctl(lp5562_fd, I2C_SLAVE_FORCE, 0x30) >= 0))
    {
        i2c_smbus_write_byte_data(lp5562_fd, LED_LP5562_R_CURRENT_REG, led_config->led_chip.channel[0].current);
        i2c_smbus_write_byte_data(lp5562_fd, LED_LP5562_G_CURRENT_REG, led_config->led_chip.channel[1].current);
        i2c_smbus_write_byte_data(lp5562_fd, LED_LP5562_B_CURRENT_REG, led_config->led_chip.channel[2].current);
    }
    else
    {
        close(lp5562_fd);
        return -1;
    }

    //apply commands to lp5562 chip
    //firstly stop engine
    snprintf(command,sizeof(command),"/bin/echo 0 > %s/run_engine",LED_LP5562_DEVICE_PATH);
    system(command);
    for (i = 0; i < 3; i++)
    {
        //select engine
        snprintf(command,sizeof(command),"/bin/echo %d > %s/select_engine",i+1,LED_LP5562_DEVICE_PATH);
        system(command);
        //set engine mod
        snprintf(command,sizeof(command),"/bin/echo \"RGB\" > %s/engine_mux",LED_LP5562_DEVICE_PATH);
        system(command);
        //loading command
        snprintf(command,sizeof(command),"/bin/echo 1 > %s/firmware/lp5562/loading",LED_LP5562_DEVICE_PATH);
        system(command);
        //snprintf(command,sizeof(command),"/bin/echo \"%s\" > %s/firmware/lp5562/data",lp5562_program[i],LED_LP5562_DEVICE_PATH);
        //system(command);
        v_secure_system("/bin/echo %s > "LED_LP5562_DEVICE_PATH"/firmware/lp5562/data",lp5562_program[i]);
        //v_secure_system("/bin/echo %s > %s/firmware/lp5562/data",lp5562_program[i],LED_LP5562_DEVICE_PATH);
        //end loading
        snprintf(command,sizeof(command),"/bin/echo 0 > %s/firmware/lp5562/loading",LED_LP5562_DEVICE_PATH);
        system(command);
    }
    //run engine
    snprintf(command,sizeof(command),"/bin/echo 1 > %s/run_engine",LED_LP5562_DEVICE_PATH);
    system(command);
    close(lp5562_fd);

    return 0;
}

static int led_apply_aw21009_setting(Led_Config *led_config)
{
    char command[512] = {0};
    char led_color[10] = {0};
    char led_brightness[10] = {0};

    snprintf(led_color,sizeof(led_color),"0x%02x%02x%02x", led_config->led_chip.channel[0].current, led_config->led_chip.channel[1].current, led_config->led_chip.channel[2].current);
    snprintf(led_brightness,sizeof(led_brightness),"0x%02x%02x%02x", led_config->led_chip.channel[0].pwm, led_config->led_chip.channel[1].pwm, led_config->led_chip.channel[2].pwm);

    if (Led_ON == led_config->action.act_type)
    {
        snprintf(command,sizeof(command),"/bin/echo 0x00 %s %s > %s &", led_color, led_brightness, RGBCOLOR);
        system(command);
    }
    if (Led_OFF == led_config->action.act_type)
    {
        snprintf(command,sizeof(command),"echo 0x00 0x000000 0x000000 > %s &", RGBCOLOR);
        system(command);
    }
    if (Led_BLINK == led_config->action.act_type)
    {
        snprintf(command,sizeof(command),"/etc/led_functions.sh brightness_blink %d:%d,%d:%d,%d:%d %d %d &", led_config->led_chip.channel[0].current, led_config->led_chip.channel[0].pwm, led_config->led_chip.channel[1].current, led_config->led_chip.channel[1].pwm, led_config->led_chip.channel[2].current, led_config->led_chip.channel[2].pwm, (led_config->action.on_time)*1000, (led_config->action.off_time)*1000);
        system(command);
    }
    if (Led_SEQ_BLINK == led_config->action.act_type)
    {
        snprintf(command,sizeof(command),"/etc/led_functions.sh sequence_blink %d:%d,%d:%d,%d:%d  %d %d %d %d &",led_config->led_chip.channel[0].current, led_config->led_chip.channel[0].pwm, led_config->led_chip.channel[1].current, led_config->led_chip.channel[1].pwm, led_config->led_chip.channel[2].current, led_config->led_chip.channel[2].pwm, (led_config->action.on_time)*1000, (led_config->action.off1_time)*1000, led_config->action.count, (led_config->action.off2_time)*1000);
        system(command);
    }

    return 0;
}

ledError_t led_applySettings(ledId_t id)
{
    char led_config_file[LED_CONFIG_FILE_NAME_LENGTH] = {0};
    Led_Config led_config;
    int config_fd = -1;
    int ret = -1;

    LEDMGR_LOG_DEBUG(" %s id: %d \n",__FUNCTION__, id);

    //check parameter 
    if ((LED_ID_CAMERA_FRONT_PANEL != id) &&(LED_ID_XW_FRONT_PANEL != id) && (LED_ID_CAMERA_IR != id))
    {
        snprintf(error_msg[LED_ERR_INVALID_PARAM],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] id %d is illegal\n",__FUNCTION__,__LINE__,id);
        return LED_ERR_INVALID_PARAM;
    }

    //open LED config
    snprintf(led_config_file,sizeof(led_config_file),"%s%s%d",LED_CONFIG_FILE_PATH,LED_CONFIG_FILE_PREFIX,id);
    config_fd = open(led_config_file,O_RDONLY);
    if (config_fd < 0)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, open config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }

    //lock LED config file
    if (lock_led_config(config_fd))
    {
        //lock failed
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, lock config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }

    //read LED config file
    if (read_led_config(config_fd,&led_config))
    {
        //read  failed
        flock(config_fd,LOCK_UN);
        close(config_fd);
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, read config file error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    
    //read success, unlock and close LED config file
    flock(config_fd,LOCK_UN);
    close(config_fd);
   
	//take action 
    if ((LED_ID_CAMERA_FRONT_PANEL == id) || (LED_ID_XW_FRONT_PANEL == id))
    {
        if (0 == access(LEDS_CHIP_AW210XX_FILE, F_OK))
        {
            system("kill -9 $(ps | grep led_functions | grep -v grep | awk -F ' ' '{print $1}') > /dev/null 2> /dev/null");
            ret = led_apply_aw21009_setting(&led_config);
        }
        else
        {
            ret = led_apply_lp5562_setting(&led_config);
        }
    }
    else
    {
        ret = led_apply_irled_setting(&led_config);
    }

    if (ret)
    {
        snprintf(error_msg[LED_ERR_UNKNOWN],LED_ERROR_MSG_MAX_LENGTH,"[%s:%d] Unknown Error, apply setting to device error\n",__FUNCTION__,__LINE__);
        return LED_ERR_UNKNOWN;
    }
    else
    {
        return LED_ERR_NONE;
    }
}

ledError_t led_applyAllSettings()
{
    int id = LED_ID_CAMERA_FRONT_PANEL;
    ledError_t ret = LED_ERR_NONE;
 
    LEDMGR_LOG_DEBUG(" %s \n",__FUNCTION__);
    for (id = LED_ID_CAMERA_FRONT_PANEL; id < LED_ID_MAX; id++)
    {
        ret = led_applySettings((ledId_t)id);
        if (LED_ERR_NONE != ret)
        {
            return ret;
        }
    }
    return LED_ERR_NONE;
}


const char* led_getErrorMsg(ledError_t err)
{
    return error_msg[err];
}

const char* led_getVersion()
{
    return LED_HAL_API_VERSION;
}
