/*
 * main.c - main file
 *
 * Copyright 2015 Edward V. Emelianoff <eddy@sao.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#include <termios.h>		// tcsetattr
#include <unistd.h>			// tcsetattr, close, read, write
#include <sys/ioctl.h>		// ioctl
#include <stdio.h>			// printf, getchar, fopen, perror
#include <stdlib.h>			// exit
#include <sys/stat.h>		// read
#include <fcntl.h>			// read
#include <signal.h>			// signal
#include <time.h>			// time
#include <string.h>			// memcpy
#include <stdint.h>			// int types
#include <sys/time.h>		// gettimeofday
#include <assert.h>			// assert
#include <pthread.h>		// threads

#include "bta_shdata.h"
#include "cmdlnopts.h"

#ifndef _U_
#define _U_    __attribute__((__unused__))
#endif

#ifndef fabs
#define fabs(x)  ((x > 0.) ? x : -x)
#endif

// scale: how much steps there is in one angular second
const double steps_per_second = 2.054;

// end-switch at 8degr
#define MIN_RESTRICT_ANGLE (29000.)
// end-switch at 100degr
#define MAX_RESTRICT_ANGLE (360000.)

#define BUFLEN 1024

glob_pars *Global_parameters = NULL;

int is_running = 1; // ==0 to exit
int BAUD_RATE = B115200;
struct termio oldtty, tty; // TTY flags
struct termios oldt, newt; // terminal flags
int comfd; // TTY fd

#define DBG(...) do{fprintf(stderr, __VA_ARGS__);}while(0)

/**
 * function for different purposes that need to know time intervals
 * @return double value: time in seconds
 */
double dtime(){
	double t;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t = tv.tv_sec + ((double)tv.tv_usec)/1e6;
	return t;
}

/**
 * Exit & return terminal to old state
 * @param ex_stat - status (return code)
 */
void quit(int ex_stat){
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // return terminal to previous state
	ioctl(comfd, TCSANOW, &oldtty ); // return TTY to previous state
	close(comfd);
	printf("Exit! (%d)\n", ex_stat);
	exit(ex_stat);
}

/**
 * Open & setup TTY, terminal
 */
void tty_init(){
	// terminal without echo
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	if(tcsetattr(STDIN_FILENO, TCSANOW, &newt) < 0) quit(-2);
	printf("\nOpen port...\n");
	if ((comfd = open(Global_parameters->serialdev,O_RDWR|O_NOCTTY|O_NONBLOCK)) < 0){
		fprintf(stderr,"Can't use port %s\n",Global_parameters->serialdev);
		quit(1);
	}
	printf(" OK\nGet current settings...\n");
	if(ioctl(comfd,TCGETA,&oldtty) < 0) exit(-1); // Get settings
	tty = oldtty;
	tty.c_lflag     = 0; // ~(ICANON | ECHO | ECHOE | ISIG)
	tty.c_oflag     = 0;
	tty.c_cflag     = BAUD_RATE|CS8|CREAD|CLOCAL; // 9.6k, 8N1, RW, ignore line ctrl
	tty.c_cc[VMIN]  = 0;  // non-canonical mode
	tty.c_cc[VTIME] = 5;
	if(ioctl(comfd,TCSETA,&tty) < 0) exit(-1); // set new mode
	printf(" OK\n");
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}

/**
 * read tty
 * @param buff (o)    - buffer for messages readed from tty
 * @param length (io) - buff's length (return readed len or 0)
 * @return 1 if something was readed here or there
 */
int read_tty(char *buff, size_t *length){
	ssize_t L;
	size_t buffsz = *length;
	struct timeval tv;
	int sel, retval = 0;
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);
	FD_SET(comfd, &rfds);
	tv.tv_sec = 0; tv.tv_usec = 100000;
	sel = select(comfd + 1, &rfds, NULL, NULL, &tv);
	if(sel > 0){
		if(FD_ISSET(comfd, &rfds)){
			if((L = read(comfd, buff, buffsz)) < 1){ // disconnect or other troubles
				fprintf(stderr, "USB error or disconnected!\n");
				quit(1);
			}else{
				if(L == 0){ // USB disconnected
					fprintf(stderr, "USB disconnected!\n");
					quit(1);
				}
				// all OK continue reading
				*length = (size_t) L;
				retval = 1;
			}
		}else{
			*length = 0;
		}
	}
	return retval;
}

void print_P2(){
	int d, m;
	double s = val_P;
	d = ((int)s / 3600) % 360;
	s -= 3600. * d;
	m = s / 60;
	s -= 60. * m;
	printf("P2 value: %02d:%02d:%03.1f\n", d, m, s);
}

/*
void help(){
	P("G\tgo to N steps\n");
	P("H\tshow this help\n");
	P("P\tset stepper period (us)\n");
	P("S\tstop motor\n");
	P("T\tshow current approx. time\n");
	P("W\tshow current steps value\n");
	P("X\tshow current stepper period\n");
}
*/

void write_tty(char *str){
	while(*str){
		write(comfd, str++, 1);
	}
}

/**
 * give command to move P2 on some angle
 * @param rel  == 1 for relative moving
 * @param angle - angle in seconds
 */
void move_P2(int rel, double angle){
	double curP = val_P, targP;
	char buf[256];
	if(fabs(angle) < 1.) return; // don't move to such little degrees
	if(curP > MIN_RESTRICT_ANGLE && curP < MAX_RESTRICT_ANGLE){
		fprintf(stderr, "Error: motor is in restricted area!\n");
		quit(-1);
	}
	if(rel) targP = curP + angle;
	else targP = angle;
	// check for restricted zone
	if(targP > MIN_RESTRICT_ANGLE && targP < MAX_RESTRICT_ANGLE){
		fprintf(stderr, "Error: you want to move into restricted area!\n");
		quit(-10);
	}
	// now check preferred rotation direction
	angle = targP - curP;
	//printf("targP: %g, curP: %g, angle: %g\n", targP, curP, angle);
	// 360degr = 1296000''; 180degr = 648000''
	if(angle < 0.) angle += 1296000.;
	if(angle > 648000.){ // it's better to move in negative direction
		angle -= 1296000.;
	}
	// convert angle into steps
	angle *= steps_per_second;
	_U_ size_t L = snprintf(buf, 256, "G%d\n", ((int)angle));
	//printf("BUF: %s\n", buf);
	write_tty(buf);
	//is_running = 0;
	while(is_running);
}

void *moving_thread(_U_ void *buf){
	if(Global_parameters->gotopos > 0.){
//printf("gotoval: %g\n", Global_parameters->gotopos);
		move_P2(0, Global_parameters->gotopos);
	}else if(fabs(Global_parameters->relmove) > 1.){ // move relative current position
//printf("relval: %g\n", Global_parameters->relmove);
		move_P2(1, Global_parameters->relmove);
	}
//	sleep(1); // give a little time for receiving messages
//	is_running = 0;
	return NULL;
}

int main(int argc, char *argv[]){
	char buff[BUFLEN+1];
	pthread_t motor_thread;
	size_t L;
	Global_parameters = parce_args(argc, argv);
	assert(Global_parameters != NULL);
	if(!get_shm_block(&sdat, ClientSide) || !check_shm_block(&sdat)){
		fprintf(stderr, "Can't get SHM block!");
		return -1;
	}
	tty_init();
	signal(SIGTERM, quit);		// kill (-15)
	signal(SIGINT, quit);		// ctrl+C
	signal(SIGQUIT, SIG_IGN);	// ctrl+\   .
	signal(SIGTSTP, SIG_IGN);	// ctrl+Z
	setbuf(stdout, NULL);
	double t, t0 = 0;
	double p_old = -400.; // impossible value for printing P2 val on first run

	if(pthread_create(&motor_thread, NULL, moving_thread, NULL)){
		/// "Не могу создать поток для захвата видео"
		fprintf(stderr, "Can't create readout thread\n");
		quit(-1);
	}
	while(is_running){
		L = BUFLEN;
		if((t = dtime()) - t0 > 1.){ // once per second (and on first run) check P2 position
			if(fabs(val_P - p_old) > 0.1){
				p_old = val_P;
				print_P2();
			}
			t0 = t;
		}
		if(read_tty(buff, &L)){
			if(L){
				buff[L] = 0;
				printf("GOT: %s", buff);
				if(strncmp(buff, "Stop", 4) == 0){
					is_running = 0;
					sleep(1);
				}
			}
		}
		if(!check_shm_block(&sdat)){
			fprintf(stderr, "Corruption in SHM block!");
			quit(-2);
		}
	}
	quit(0);
	return 0;
}
