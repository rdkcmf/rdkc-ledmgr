#include "rtConnection.h"
#include "rtLog.h"
#include "rtMessage.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

static rtConnection con=NULL;

void rtConnection_Init();

void rtConnection_leddestroy();

int xw_isconnected();

int xw_file_get();

int xw_led_init(int ledId);

int xw_led_reset(int ledId);

int xw_led_resetAll();

int xw_led_setEnable(int ledid,int enable);

int xw_led_setColor(int ledid,uint8_t R, uint8_t G, uint8_t B);

int xw_led_setBrightness(int ledid,uint8_t R, uint8_t G, uint8_t B);

int xw_led_setBlink(int ledid,uint32_t offtime, uint32_t ontime);

int xw_led_setBlinkSequence(int ledid,uint32_t ontime, uint32_t offtime1, uint32_t count, uint32_t offtime2);

int xw_led_setOnOff(int ledid,const char* onoff);

int xw_led_applySettings(int ledid);

int xw_led_applyAllSettings();

int xw_led_getErrorMsg();

int xw_led_getVersion();

