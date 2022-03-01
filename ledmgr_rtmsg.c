#include "ledmgr_rtmsg.h"
#include "ledhal.h"
#include <string.h>
#include "ledmgrlogger.h"

void rtConnection_Init()
{
	LEDMGR_LOG_INFO("Led Communicates via secure path \n");
	rtConnection_Create(&con, "XCAM_LED", "tcp://127.0.0.1:10001");
}

void rtConnection_leddestroy()
{
	rtConnection_Destroy(con);
}

int xw_isconnected()
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=1;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_isconnected");
	err = rtConnection_SendRequest(con, req, "XW4.ISCONNECTED", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetString(res,"state",&state);
		rtMessage_GetInt32(res,"retval",&retval);
		rtLog_Debug("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_file_get()
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_file_get");
	err = rtConnection_SendRequest(con, req, "XW4.FILEGET", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		if(retval == 1)
		{
			FILE *ptr = fopen("/opt/usr_config/xwsystem.conf","w");
			for(int i=0;i<5;i++)
			{
				rtMessage temp;
				char const *val = NULL;
				rtMessage_GetMessageItem(res,"ledarr",i,&temp);
				rtMessage_GetString(temp,"led",&val);
				rtLog_Debug("val:%s \n",val);
				fputs(val, ptr);
			}
			fclose(ptr);
		}
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}


int xw_led_init(int ledId)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_init");
	rtMessage_SetInt32(req, "ledid",ledId);
	err = rtConnection_SendRequest(con, req, "XW4.LEDINIT", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetString(res,"state",&state);
		rtMessage_GetInt32(res,"retval",&retval);
		rtLog_Debug("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_reset(int ledId)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_reset");
	rtMessage_SetInt32(req, "ledid",ledId);
	err = rtConnection_SendRequest(con, req, "XW4.LEDRESET", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Info("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_resetAll()
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_resetAll");
	err = rtConnection_SendRequest(con, req, "XW4.LEDRESETALL", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetString(res,"state",&state);
		rtMessage_GetInt32(res,"retval",&retval);
		rtLog_Debug("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_setEnable(int ledid,int enable)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_setEnable");
	rtMessage_SetInt32(req, "ledid", ledid);
	rtMessage_SetInt32(req, "enable",enable);
	err = rtConnection_SendRequest(con, req, "XW4.LEDSETENABLE", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_setColor(int ledid,uint8_t R, uint8_t G, uint8_t B)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_Color");
	rtMessage_SetInt32(req, "ledid",ledid);
	rtMessage_SetInt32(req, "R", R);
	rtMessage_SetInt32(req, "G", G);
	rtMessage_SetInt32(req, "B", B);
	err = rtConnection_SendRequest(con, req, "XW4.LEDSETCOLOR", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);

	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_setBrightness(int ledid,uint8_t R, uint8_t G, uint8_t B)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_setBrightness");
	rtMessage_SetInt32(req, "ledid",ledid);
	rtMessage_SetInt32(req, "R", R);
	rtMessage_SetInt32(req, "G", G);
	rtMessage_SetInt32(req, "B", B);
	err = rtConnection_SendRequest(con, req, "XW4.LEDSETBRIGHTNESS", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);

	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_setBlink(int ledid,uint32_t offtime, uint32_t ontime)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_setBlink");
	rtMessage_SetInt32(req, "ledid",ledid);
	rtMessage_SetInt32(req, "ontime", ontime);
	rtMessage_SetInt32(req, "offtime", offtime);
	err = rtConnection_SendRequest(con, req, "XW4.LEDSETBLINK", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);

	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_setBlinkSequence(int ledid,uint32_t ontime, uint32_t offtime1, uint32_t count, uint32_t offtime2)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_setBlinkSequence");
	rtMessage_SetInt32(req, "ledid",ledid);
	rtMessage_SetInt32(req, "ontime", ontime);
	rtMessage_SetInt32(req, "count", count);
	rtMessage_SetInt32(req, "offtime", offtime1);
	rtMessage_SetInt32(req, "offtime", offtime2);
	err = rtConnection_SendRequest(con, req, "XW4.LEDSETBLINKSEQUENCE", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_setOnOff(int ledid,const char* onoff)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0,ret=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_setOnOff");
	rtMessage_SetInt32(req, "ledid",ledid);
	rtMessage_SetString(req,"onoff",onoff);
	err = rtConnection_SendRequest(con, req, "XW4.LEDSETONOFF", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req); 
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}


int xw_led_applySettings(int ledid)
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_applySettings");
	rtMessage_SetInt32(req, "ledid",ledid);
	err = rtConnection_SendRequest(con, req, "XW4.LEDAPPLYSETTINGS", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);

	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_applyAllSettings()
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_led_applyAllSettings");
	err = rtConnection_SendRequest(con, req, "XW4.LEDAPPLYALLSETTINGS", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);

	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

int xw_led_getErrorMsg()
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	int errval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_ledgetErrorMsg");
	err = rtConnection_SendRequest(con, req, "XW4.LEDGETERRORMSG", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetInt32(res,"errval",&errval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s %d \n",retval,state,errval);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return errval;
}

int xw_led_getVersion()
{
	rtMessage res=NULL;
	rtMessage req=NULL;
	rtError err;
	int retval=0;
	char const* state = NULL;
	rtMessage_Create(&req);
	rtMessage_SetString(req, "fname", "xw_ledgetVersion");
	err = rtConnection_SendRequest(con, req, "XW4.LEDGETVERSION", &res, 2000);
	rtLog_Debug("SendRequest:%s", rtStrError(err));
	if (err == RT_OK)
	{
		char* p = NULL;
		uint32_t len = 0;

		rtMessage_ToString(res, &p, &len);
		rtLog_Debug("\tres:%.*s\n", len, p);
		free(p);
		rtMessage_GetInt32(res,"retval",&retval);
		rtMessage_GetString(res,"state",&state);
		rtLog_Debug("returnval: %d %s \n",retval,state);
	}
	rtMessage_Release(req);
	if(res != NULL)
		rtMessage_Release(res);
	return retval;
}

