/**
 * @section License
 *
 * The MIT License (MIT)
 * 
 * Copyright (c) 2014-2016, Erik Moqvist
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the Simba project.
 */

/**
 * This file was generated by settings.py 1.0 2016-08-08 14:50 CEST.
 */

#include "simba.h"

const FAR struct setting_t settings[]
__attribute__ ((weak)) = {
    { NULL, 0, 0, 0 }
};

#if defined(ARCH_AVR)

uint8_t settings_area[CONFIG_SETTINGS_AREA_SIZE]
__attribute__ ((section (".eeprom"), weak)) = {
    0xff,
};

const FAR uint8_t settings_default_area[CONFIG_SETTINGS_AREA_SIZE]
__attribute__ ((weak)) = {
    0xff,
};

#elif defined(ARCH_ARM)

uint8_t settings_area[2][CONFIG_SETTINGS_AREA_SIZE]
__attribute__ ((section (".settings"), weak)) = {
    {
        0xff,
    },
    {
        0xff,
    }
};

uint8_t settings_default_area[CONFIG_SETTINGS_AREA_SIZE]
__attribute__ ((section (".settings"), weak)) = {
    0xff,
};

#else

const uint8_t settings_default_area[CONFIG_SETTINGS_AREA_SIZE]
__attribute__ ((weak)) = {
    0xff,
};

#endif
