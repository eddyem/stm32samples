#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "hdr.h"
#include "strfunc.h"

static int noargs(uint32_t hash){
    switch(hash){
        case CMD_REBOOT: printf("REBOOT\n"); break;
        case CMD_TIME: printf("TIME!\n"); break;
        case CMD_TEMP: printf("Temp\n"); break;
        default: printf("Unknown hash 0x%x\n", hash); return 0;
    }
    return 1;
}
static int withparno(uint32_t hash, char *args){
    uint32_t N;
    //printf("args=%s\n", args);
    const char *nxt = getnum((const char*)args, &N);
    if(nxt == args){ printf("need arg\n");  return RET_WRONGCMD; }
    args = (char*)omit_spaces(nxt);
    if(*args){
        if(*args != '='){ printf("need nothing or '=' after par\n"); return RET_WRONGCMD; }
        args = (char*)omit_spaces(args+1);
        // we can check in next functions what if `args` will be an empty string
    }
    else args = NULL;
    const char *fname = NULL;
    switch(hash){
        case CMD_ESW: fname = "ESW"; break;
        case CMD_GOTO: fname = "GOTO"; break;
        case CMD_POS: fname = "POS"; break;
        case CMD_STOP: fname = "STOP"; break;
        default: fname = "unknown";
    }
    if(!args) printf("We want a getter '%s' with par %u\n", fname, N);
    else printf("We want a setter '%s' with par %u and arg '%s'\n", fname, N, args);
    if(N > 255){ printf("N should be 0..255\n"); return RET_BAD;}
    return RET_GOOD;
}
// these functions should be global to substitute weak aliases
int fn_esw(uint32_t hash,  char *args){return withparno(hash, args);}
int fn_goto(uint32_t hash,  char *args){return withparno(hash, args);}
int fn_pos(uint32_t hash,  char *args){return withparno(hash, args);}
int fn_stop(uint32_t hash,  char *args){return withparno(hash, args);}
int fn_voltage(uint32_t hash,  char *args){return withparno(hash, args);}
int fn_reboot(uint32_t hash,  _U_ char *args){return noargs(hash);}
int fn_time(uint32_t hash,  _U_ char *args){return noargs(hash);}
int fn_temp(uint32_t hash,  _U_ char *args){return noargs(hash);}

int main(int argc, char **argv){
    if(argc != 2) return 1;
    int ret = parsecmd(argv[1]);
    switch(ret){
        case RET_CMDNOTFOUND: printf("'%s' not found\n", argv[1]); break;
        case RET_WRONGCMD: printf("Wrong sintax of '%s'\n", argv[1]); break;
        case RET_GOOD: printf("'%s' - OK\n", argv[1]); break;
        default: printf("'%s' - bad\n", argv[1]);
    }
    return 0;
}
