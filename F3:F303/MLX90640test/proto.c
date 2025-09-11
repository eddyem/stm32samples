/*
 * This file is part of the mlxtest project.
 * Copyright 2025 Edward V. Emelianov <edward.emelianoff@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stm32f3.h>
#include <string.h>

#include "mlx90640.h"
#include "strfunc.h"
#include "usb_dev.h"
#include "version.inc"

const char *helpstring =
        "https://github.com/eddyem/stm32samples/tree/master/F3:F303/USB_template build#" BUILD_NUMBER " @ " BUILD_DATE "\n"
        "T - print current Tms\n"
        "R - run MLX test\n"
;

const char *parse_cmd(const char *buf){
    if(buf[1]) return buf; // echo wrong data
    switch(*buf){
        case 'T':
            USB_sendstr("T=");
            USB_sendstr(u2str(Tms));
            newline();
        break;
        case 'R':
            if(MLXtest()) USB_sendstr("error!\n");
        break;
        default: // help
            USB_sendstr(helpstring);
        break;
    }
    return NULL;
}
