#
# @file mcus/esp8266/mcu.mk
#
# @section License
# Copyright (C) 2014-2016, Erik Moqvist
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# This file is part of the Simba project.
#

ESP8266_RTOS_SDK_ROOT ?= $(SIMBA_ROOT)/3pp/ESP8266_RTOS_SDK

INC += $(SIMBA_ROOT)/src/mcus/esp8266 \
       $(ESP8266_RTOS_SDK_ROOT)/extra_include \
       $(ESP8266_RTOS_SDK_ROOT)/include \
       $(ESP8266_RTOS_SDK_ROOT)/include/espressif \
       $(ESP8266_RTOS_SDK_ROOT)/include/espressif/esp8266 \
       $(ESP8266_RTOS_SDK_ROOT)/include/lwip \
       $(ESP8266_RTOS_SDK_ROOT)/include/lwip/ipv4 \
       $(ESP8266_RTOS_SDK_ROOT)/include/lwip/ipv6

SRC += $(SIMBA_ROOT)/src/mcus/esp8266/mcu.c \
       $(SIMBA_ROOT)/src/mcus/esp8266/esp8266.c

LIBPATH += "$(SIMBA_ROOT)/src/mcus/esp8266/ld"

LDFLAGS += -Wl,-T$(LINKER_SCRIPT)

LIB_MINIC ?= minic

LIB += \
	hal \
	gcc \
	phy \
	pp \
	net80211 \
	wpa \
	crypto \
	main \
	freertos \
	lwip \
	$(LIB_MINIC)

F_CPU = 80000000

ARCH = esp
FAMILY = esp

MCU_HOMEPAGE = "http://www.espressif.com"
MCU_NAME = "Espressif ESP8266"
MCU_DESC = "Espressif ESP8266 @ 80 MHz, 82 kB dram, 4 MB flash"

SIZE_SUMMARY_CMD ?= $(SIMBA_ROOT)/bin/memory_usage.py \
			--ram-section .data \
			--ram-section .rodata \
			--ram-section .bss \
			--rom-section .text \
			--rom-section .irom0.text \
			${EXE}

include $(SIMBA_ROOT)/make/$(TOOLCHAIN)/esp.mk
