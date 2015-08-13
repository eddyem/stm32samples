/*
 * cmdlnopts.c - the only function that parce cmdln args and returns glob parameters
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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "cmdlnopts.h"

/*
 * here are global parameters initialisation
 */
glob_pars G;  // internal global parameters structure
int help = 0; // whether to show help string

glob_pars Gdefault = {
	.serialdev        = "/dev/ttyACM0",
	.gotopos          = -1000.,
	.relmove          = 0.,
};

#ifndef N_
#define N_(x)   (x)
#endif

#ifndef _
#define _(x)    (x)
#endif

bool get_angle(double *dest, char *src){
	double A = 0., sign = 1.;
	int d,m;
//	printf("arg: %s\n", src);
	if(myatod(&A, src, arg_double)){ // angle in seconds
		*dest = A;
		return true;
	}else if(3 == sscanf(src, "%d:%d:%lf", &d, &m, &A)){
		if(d > -361 && d < 361 && m > -1 && m < 60 && A > -0.1 && A < 60.){
			if(d < 0) sign = -1.;
			*dest = sign*A + sign*60.*m + 3600.*d;
			return true;
		}
	}else if(2 == sscanf(src, "%d:%lf", &m, &A)){
		if(m > -60 && m < 60 && A > -0.1 && A < 60.){
			if(m < 0) sign = -1.;
			*dest = sign*A + 60.*m;
			return true;
		}
	}
	fprintf(stderr, "Wrong angle format [%s], need: ss.ss, mm:ss.ss or dd:mm:ss.ss\n", src);
	return false;
}
#ifndef _U_
#define _U_    __attribute__((__unused__))
#endif
bool ang_goto(void *arg, _U_ int N){
	return get_angle(&G.gotopos, arg);
}

bool ang_gorel(void *arg, _U_ int N){
	return get_angle(&G.relmove, arg);
}

/*
 * Define command line options by filling structure:
 *	name	has_arg	flag	val		type		argptr			help
*/
myoption cmdlnopts[] = {
	/// "отобразить это сообщение"
	{"help",	0,	NULL,	'h',	arg_int,	APTR(&help),		N_("show this help")},
	/// "путь к устройству микроконтроллера"
	{"serialdev",1,	NULL,	'd',	arg_string,	APTR(&G.serialdev),	N_("path to MCU device")},
	{"goto",	1,	NULL,	'g',	arg_function,APTR(ang_goto),	N_("go to given position")},
	{"relative",1,	NULL,	'r',	arg_function,APTR(ang_gorel),	N_("go relative current position")},
	end_option
};


/**
 * Parce command line options and return dynamically allocated structure
 * 		to global parameters
 * @param argc - copy of argc from main
 * @param argv - copy of argv from main
 * @return allocated structure with global parameters
 */
glob_pars *parce_args(int argc, char **argv){
	int i;
	void *ptr;
	ptr = memcpy(&G, &Gdefault, sizeof(G)); assert(ptr);
	// format of help: "Usage: progname [args]\n"
	/// "Использование: %s [аргументы]\n\n\tГде аргументы:\n"
	change_helpstring(_("Usage: %s [args]\n\n\tWhere args are:\n"));
	// parse arguments
	parceargs(&argc, &argv, cmdlnopts);
	if(help) showhelp(-1, cmdlnopts);
	if(argc > 0){
		/// "Игнорирую аргумент[ы]:"
		printf("\n%s\n", _("Ignore argument[s]:"));
		for (i = 0; i < argc; i++)
			printf("\t%s\n", argv[i]);
	}
	return &G;
}

