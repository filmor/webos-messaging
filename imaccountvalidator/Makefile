# Device makefile

BUILD_TYPE := release
PLATFORM := linux-arm
OBJDIR := $(BUILD_TYPE)-$(PLATFORM)
LOCAL_INCLUDES := -I$(QPEDIR)/include/mojodb \
					-I./inc \
					-I$(STAGING_INCDIR) \
					-I.
LOCAL_CFLAGS := $(CFLAGS) -Wall -Werror -DMOJ_LINUX -DBOOST_NO_TYPEID $(LOCAL_INCLUDES) $(shell pkg-config --cflags glib-2.0 purple)
LOCAL_CPPFLAGS := $(CPPFLAGS) -fno-rtti

include Makefile.inc
