/*
 * parceargs.c - parcing command line arguments & print help
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

#include <stdio.h>	// DBG
#include <getopt.h>	// getopt_long
#include <stdlib.h>	// calloc, exit, strtoll
#include <assert.h>	// assert
#include <string.h> // strdup, strchr, strlen
#include <limits.h> // INT_MAX & so on
#include <libintl.h>// gettext
#include <ctype.h>	// isalpha
#include "parceargs.h"

// macro to print help messages
#ifndef PRNT
	#define PRNT(x) gettext(x)
#endif

char *helpstring = "%s\n";

/**
 * Change standard help header
 * MAY consist ONE "%s" for progname
 * @param str (i) - new format
 */
void change_helpstring(char *s){
	int pcount = 0, scount = 0;
	char *str = s;
	// check `helpstring` and set it to default in case of error
	for(; pcount < 2; str += 2){
		if(!(str = strchr(str, '%'))) break;
		if(str[1] != '%') pcount++; // increment '%' counter if it isn't "%%"
		else{
			str += 2; // pass next '%'
			continue;
		}
		if(str[1] == 's') scount++; // increment "%s" counter
	};
	if(pcount > 1 || pcount != scount){ // amount of pcount and/or scount wrong
		fprintf(stderr, "Wrong helpstring!\n");
		exit(-1);
	}
	helpstring = s;
}

/**
 * Carefull atoll/atoi
 * @param num (o) - returning value (or NULL if you wish only check number) - allocated by user
 * @param str (i) - string with number must not be NULL
 * @param t   (i) - T_INT for integer or T_LLONG for long long (if argtype would be wided, may add more)
 * @return TRUE if conversion sone without errors, FALSE otherwise
 */
bool myatoll(void *num, char *str, argtype t){
	long long tmp, *llptr;
	int *iptr;
	char *endptr;
	assert(str);
	assert(num);
	tmp = strtoll(str, &endptr, 0);
	if(endptr == str || *str == '\0' || *endptr != '\0')
		return FALSE;
	switch(t){
		case arg_longlong:
			llptr = (long long*) num;
			*llptr = tmp;
		break;
		case arg_int:
		default:
			if(tmp < INT_MIN || tmp > INT_MAX){
				fprintf(stderr, "Integer out of range\n");
				return FALSE;
			}
			iptr = (int*)num;
			*iptr = (int)tmp;
	}
	return TRUE;
}

// the same as myatoll but for double
// There's no NAN & INF checking here (what if they would be needed?)
bool myatod(void *num, const char *str, argtype t){
	double tmp, *dptr;
	float *fptr;
	char *endptr;
	assert(str);
	tmp = strtod(str, &endptr);
	if(endptr == str || *str == '\0' || *endptr != '\0')
		return FALSE;
	switch(t){
		case arg_double:
			dptr = (double *) num;
			*dptr = tmp;
		break;
		case arg_float:
		default:
			fptr = (float *) num;
			*fptr = (float)tmp;
		break;
	}
	return TRUE;
}

/**
 * Get index of current option in array options
 * @param opt (i) - returning val of getopt_long
 * @param options (i) - array of options
 * @return index in array
 */
int get_optind(int opt, myoption *options){
	int oind;
	myoption *opts = options;
	assert(opts);
	for(oind = 0; opts->name && opts->val != opt; oind++, opts++);
	if(!opts->name || opts->val != opt) // no such parameter
	showhelp(-1, options);
	return oind;
}

/**
 * Parce command line arguments
 * ! If arg is string, then value will be strdup'ed!
 *
 * @param argc (io) - address of argc of main(), return value of argc stay after `getopt`
 * @param argv (io) - address of argv of main(), return pointer to argv stay after `getopt`
 * BE CAREFUL! if you wanna use full argc & argv, save their original values before
 * 		calling this function
 * @param options (i) - array of `myoption` for arguments parcing
 *
 * @exit: in case of error this function show help & make `exit(-1)`
  */
void parceargs(int *argc, char ***argv, myoption *options){
	char *short_options, *soptr;
	struct option *long_options, *loptr;
	size_t optsize, i;
	myoption *opts = options;
	// check whether there is at least one options
	assert(opts);
	assert(opts[0].name);
	// first we count how much values are in opts
	for(optsize = 0; opts->name; optsize++, opts++);
	// now we can allocate memory
	short_options = calloc(optsize * 3 + 1, 1); // multiply by three for '::' in case of args in opts
	long_options = calloc(optsize + 1, sizeof(struct option));
	opts = options; loptr = long_options; soptr = short_options;
	// fill short/long parameters and make a simple checking
	for(i = 0; i < optsize; i++, loptr++, opts++){
		// check
		assert(opts->name); // check name
		if(opts->has_arg){
			assert(opts->type != arg_none); // check error with arg type
			assert(opts->argptr);  // check pointer
		}
		if(opts->type != arg_none) // if there is a flag without arg, check its pointer
			assert(opts->argptr);
		// fill long_options
		// don't do memcmp: what if there would be different alignment?
		loptr->name		= opts->name;
		loptr->has_arg	= opts->has_arg;
		loptr->flag		= opts->flag;
		loptr->val		= opts->val;
		// fill short options if they are:
		if(!opts->flag){
			*soptr++ = opts->val;
			if(opts->has_arg) // add ':' if option has required argument
				*soptr++ = ':';
			if(opts->has_arg == 2) // add '::' if option has optional argument
				*soptr++ = ':';
		}
	}
	// now we have both long_options & short_options and can parse `getopt_long`
	while(1){
		int opt;
		int oindex = 0, optind = 0; // oindex - number of option in argv, optind - number in options[]
		if((opt = getopt_long(*argc, *argv, short_options, long_options, &oindex)) == -1) break;
		if(opt == '?'){
			opt = optopt;
			optind = get_optind(opt, options);
			if(options[optind].has_arg == 1) showhelp(optind, options); // need argument
		}
		else{
			if(opt == 0 || oindex > 0) optind = oindex;
			else optind = get_optind(opt, options);
		}
		opts = &options[optind];
		if(opt == 0 && opts->has_arg == 0) continue; // only long option changing integer flag
		// now check option
		if(opts->has_arg == 1) assert(optarg);
		bool result = TRUE;
		// even if there is no argument, but argptr != NULL, think that optarg = "1"
		if(!optarg) optarg = "1";
		switch(opts->type){
			default:
			case arg_none:
				if(opts->argptr) *((int*)opts->argptr) = 1; // set argptr to 1
			break;
			case arg_int:
				result = myatoll(opts->argptr, optarg, arg_int);
			break;
			case arg_longlong:
				result = myatoll(opts->argptr, optarg, arg_longlong);
			break;
			case arg_double:
				result = myatod(opts->argptr, optarg, arg_double);
			break;
			case arg_float:
				result = myatod(opts->argptr, optarg, arg_float);
			break;
			case arg_string:
				result = (*((char **)opts->argptr) = strdup(optarg));
			break;
			case arg_function:
				result = ((argfn)opts->argptr)(optarg, optind);
			break;
		}
		if(!result){
			showhelp(optind, options);
		}
	}
	*argc -= optind;
	*argv += optind;
}

/**
 * Show help information based on myoption->help values
 * @param oindex (i)  - if non-negative, show only help by myoption[oindex].help
 * @param options (i) - array of `myoption`
 *
 * @exit:  run `exit(-1)` !!!
 */
void showhelp(int oindex, myoption *options){
	// ATTENTION: string `help` prints through macro PRNT(), by default it is gettext,
	// but you can redefine it before `#include "parceargs.h"`
	int max_opt_len = 0; // max len of options substring - for right indentation
	const int bufsz = 255;
	char buf[bufsz+1];
	myoption *opts = options;
	assert(opts);
	assert(opts[0].name); // check whether there is at least one options
	if(oindex > -1){ // print only one message
		opts = &options[oindex];
		printf("  ");
		if(!opts->flag && isalpha(opts->val)) printf("-%c, ", opts->val);
		printf("--%s", opts->name);
		if(opts->has_arg == 1) printf("=arg");
		else if(opts->has_arg == 2) printf("[=arg]");
		printf("  %s\n", PRNT(opts->help));
		exit(-1);
	}
	// header, by default is just "progname\n"
	printf("\n");
	if(strstr(helpstring, "%s")) // print progname
		printf(helpstring, __progname);
	else // only text
		printf("%s", helpstring);
	printf("\n");
	// count max_opt_len
	do{
		int L = strlen(opts->name);
		if(max_opt_len < L) max_opt_len = L;
	}while((++opts)->name);
	max_opt_len += 14; // format: '-S , --long[=arg]' - get addition 13 symbols
	opts = options;
	// Now print all help
	do{
		int p = sprintf(buf, "  "); // a little indent
		if(!opts->flag && isalpha(opts->val)) // .val is short argument
			p += snprintf(buf+p, bufsz-p, "-%c, ", opts->val);
		p += snprintf(buf+p, bufsz-p, "--%s", opts->name);
		if(opts->has_arg == 1) // required argument
			p += snprintf(buf+p, bufsz-p, "=arg");
		else if(opts->has_arg == 2) // optional argument
			p += snprintf(buf+p, bufsz-p, "[=arg]");
		assert(p < max_opt_len); // there would be magic if p >= max_opt_len
		printf("%-*s%s\n", max_opt_len+1, buf, PRNT(opts->help)); // write options & at least 2 spaces after
	}while((++opts)->name);
	printf("\n\n");
	exit(-1);
}
