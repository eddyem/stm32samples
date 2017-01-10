/*
 * client.c - simple terminal client
 *
 * Copyright 2013 Edward V. Emelianoff <eddy@sao.ru>
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

#define BUFLEN 1024

double t0; // start time

FILE *fout = NULL; // file for messages duplicating
char *comdev = "/dev/ttyACM0";
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
	if(fout) fclose(fout);
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
	if ((comfd = open(comdev,O_RDWR|O_NOCTTY|O_NONBLOCK)) < 0){
		fprintf(stderr,"Can't use port %s\n",comdev);
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
 * getchar() without echo
 * wait until at least one character pressed
 * @return character readed
 *
int mygetchar(){
	int ret;
	do ret = read_console();
	while(ret == 0);
	return ret;
}*/

/**
 * read both tty & console
 * @param buff (o)    - buffer for messages readed from tty
 * @param length (io) - buff's length (return readed len or 0)
 * @param rb (o)      - byte readed from console or -1
 * @return 1 if something was readed here or there
 */
int read_tty_and_console(char *buff, size_t *length, int *rb){
	ssize_t L;
	// ssize_t l;
	size_t buffsz = *length;
	struct timeval tv;
	int sel, retval = 0;
	fd_set rfds;
	FD_ZERO(&rfds);
	FD_SET(STDIN_FILENO, &rfds);
	FD_SET(comfd, &rfds);
	tv.tv_sec = 0; tv.tv_usec = 10000;
	sel = select(comfd + 1, &rfds, NULL, NULL, &tv);
	if(sel > 0){
		if(FD_ISSET(STDIN_FILENO, &rfds)){
			*rb = getchar();
			retval = 1;
		}else{
			*rb = -1;
		}
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
		/*		DBG("readed %zd bytes, try more.. ", L);
				buffsz -= L;
				while(buffsz > 0 && (l = read(comfd, buff+L, buffsz)) > 0){
					L += l;
					buffsz -= l;
				}
				DBG("full len: %zd\n", L); */
				*length = (size_t) L;
				retval = 1;
			}
		}else{
			*length = 0;
		}
	}
	return retval;
}

void help(){
	printf("Use this commands:\n"
		"h\tShow this help\n"
		"q\tQuit\n"
	);
}

void con_sig(int rb){
	char cmd;
	if(rb < 1) return;
	if(rb == 'q') quit(0); // q == exit
	cmd = (char) rb;
	write(comfd, &cmd, 1);
	/*switch(rb){
		case 'h':
			help();
		break;
		default:
			cmd = (uint8_t) rb;
			write(comfd, &cmd, 1);
	}*/
}

/**
 * Get integer value from buffer
 * @param buff (i) - buffer with int
 * @param len      - length of data in buffer (could be 2 or 4)
 * @return
 */
uint32_t get_int(char *buff, size_t len){
	if(len != 2 && len != 4){
		fprintf(stdout, "Bad data length!\n");
		return 0xffffffff;
	}
	uint32_t data = 0;
	uint8_t *i8 = (uint8_t*) &data;
	if(len == 2) memcpy(i8, buff, 2);
	else memcpy(i8, buff, 4);
	return data;
}

/**
 * Copy line by line buffer buff to file removing cmd starting from newline
 * @param buffer - data to put into file
 * @param cmd - symbol to remove from line startint (if found, change *cmd to (-1)
 * 			or NULL, (-1) if no command to remove
 */
void copy_buf_to_file(char *buffer, int *cmd){
	char *buff, *line, *ptr;
	if(!cmd || *cmd < 0){
		fprintf(fout, "%s", buffer);
		return;
	}
	buff = strdup(buffer), ptr = buff;
	do{
		if(!*ptr) break;
		if(ptr[0] == (char)*cmd){
			*cmd = -1;
			ptr++;
			if(ptr[0] == '\n') ptr++;
			if(!*ptr) break;
		}
		line = ptr;
		ptr = strchr(buff, '\n');
		if(ptr){
			*ptr++ = 0;
			//fprintf(fout, "%s\n", line);
		}//else
			//fprintf(fout, "%s", line); // no newline found in buffer
		fprintf(fout, "%s\n", line);
	}while(ptr);
	free(buff);
}

int main(int argc, char *argv[]){
	int rb, oldcmd = -1;
	char buff[BUFLEN+1];
	size_t L;
	if(argc == 2){
		fout = fopen(argv[1], "a");
		if(!fout){
			perror("Can't open output file");
			exit(-1);
		}
		setbuf(fout, NULL);
	}
	tty_init();
	signal(SIGTERM, quit);		// kill (-15)
	signal(SIGINT, quit);		// ctrl+C
	signal(SIGQUIT, SIG_IGN);	// ctrl+\   .
	signal(SIGTSTP, SIG_IGN);	// ctrl+Z
	setbuf(stdout, NULL);
	t0 = dtime();
	while(1){
		L = BUFLEN;
		if(read_tty_and_console(buff, &L, &rb)){
			if(rb > 0){
				con_sig(rb);
				oldcmd = rb;
			}
			if(L){
				buff[L] = 0;
				printf("%s", buff);
				if(fout){
					copy_buf_to_file(buff, &oldcmd);
				}
			}
		}
	}
}
