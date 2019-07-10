/*
 * This file is part of the chronometer project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>.
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

#include "flash.h"
#include "lidar.h"
#include "usart.h"

uint16_t last_lidar_dist = 0;
uint16_t last_lidar_stren = 0;
uint16_t lidar_triggered_dist = 0;

void parse_lidar_data(char *txt){
    static int triggered = 0;
    last_lidar_dist = txt[2] | (txt[3] << 8);
    last_lidar_stren = txt[4] | (txt[5] << 8);
    if(last_lidar_stren < LIDAR_LOWER_STREN) return; // weak signal
    if(!lidar_triggered_dist){ // first run
        lidar_triggered_dist = last_lidar_dist;
        return;
    }
    if(triggered){ // check if body gone
        if(last_lidar_dist < the_conf.dist_min || last_lidar_dist > the_conf.dist_max || last_lidar_dist > lidar_triggered_dist + LIDAR_DIST_THRES){
            triggered = 0;
#ifdef EBUG
            SEND("Untriggered! distance=");
            printu(1, last_lidar_dist);
            SEND(" signal=");
            printu(1, last_lidar_stren);
            newline();
#endif
        }
    }else{
        if(last_lidar_dist > the_conf.dist_min && last_lidar_dist < the_conf.dist_max){
            triggered = 1;
            lidar_triggered_dist = last_lidar_dist;
#ifdef EBUG
            SEND("Triggered! distance=");
            printu(1, last_lidar_dist);
            SEND(" signal=");
            printu(1, last_lidar_stren);
            newline();
#endif
        }
    }
}
