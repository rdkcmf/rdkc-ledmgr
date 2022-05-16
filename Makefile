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

# Add dependent libraries
USE_SYSUTILS = yes
SUPPORT_MXML = yes
USE_DATAPROVIDER = no
USE_RTMESSAGE = yes
USE_LIBSYSWRAPPER = yes
USE_BREAKPAD = yes
USE_TELEMETRY_2 = yes

include ${RDK_PROJECT_ROOT_PATH}/utility/AppsRule.mak
LDFLAGS = $(LIBFLAGS)
CXXFLAGS = $(CFLAGS)

BUILD_PLATFORM := PLATFORM_RDKC

LEDMGR_ROOT_DIR  := .
BUILD_ROOT_DIR  := $(LEDMGR_ROOT_DIR)/..

# Cross Compiler Path for RDKC
ifeq ($(BUILD_PLATFORM), PLATFORM_RDKC)
CC = @echo " [CXX] $<" ; $(CXX)
endif
#If platform is not RDKC, assign respective Cross Compiler path to CC

SRCS = ledmgr.c ledmgrlogger.c ledhal.c ledmgr_rtmsg.c

OBJDIR=obj
OBJS=$(patsubst %.c, $(OBJDIR)/%.o, $(SRCS))

XWCLIENT_DIR_INC=$(BUILD_ROOT_DIR)/xwclient/

CXXFLAGS += -g -std=c++11 -I$(XWCLIENT_DIR_INC) -DENABLE_RTMESSAGE

LED_MGR_SRC=ledtest.c
LED_MGR_OBJS=$(patsubst %.c, $(OBJDIR)/%.o, $(LED_MGR_SRC))

LED_MAIN_SRC=ledmgrmain.c
LED_MAIN_OBJS=$(patsubst %.c, $(OBJDIR)/%.o, $(LED_MAIN_SRC))

all: ledtest ledmgrmain
LDFLAGS += -lcurl

$(OBJDIR)/%.o: %.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) -c -g -std=c++11 -Wextra $(CXXFLAGS) $< -o $@

libledmgr.so: $(OBJS)
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) -shared $(OBJS) $(LDFLAGS) -o $@

ledtest: $(LED_MGR_OBJS) libledmgr.so
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) $(LED_MGR_OBJS) -L. $(LDFLAGS) -lledmgr -o $@

ledmgrmain: $(LED_MAIN_OBJS) libledmgr.so
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) $(LED_MAIN_OBJS) -L. $(LDFLAGS) -lledmgr -o $@

clean:
	rm -rf $(OBJDIR)
	rm -f ledtest
	rm -rf ledmgrmain
	rm -f libledmgr.so

.PHONY: all clean
