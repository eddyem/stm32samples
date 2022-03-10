#include <stdio.h>
#include <string.h>
#include "fonts.h"
#include "screen.h"

#define WHITE   "\033[1;38;40m"
#define RED     "\033[1;31;40m"
#define GREEN   "\033[1;32;40m"
#define BLUE    "\033[1;34;40m"
#define YELLOW  "\033[1;33;40m"
#define DEFCOL  "\033[0m"

void dumpC(uint8_t C){
    for(int i = 7; i > -1; --i)
        if(C & 1<<i) printf("*");
        else printf(" ");
}

void chcolr(int n){
    switch(n){
        case 0:
            printf(RED);
        break;
        case 1:
            printf(GREEN);
        break;
        case 2:
            printf(BLUE);
        break;
        default:
            printf(YELLOW);
    }
}

void dumpbuf(){
    uint8_t *buf = getScreenBuf();
    printf(WHITE "   ");
    for(int x = 0; x < 4; ++x){
        printf("%d... ...", x);
    }
    printf("|");
    for(int x = 4; x < 8; ++x){
        printf("%d... ...", x);
    }
    printf(DEFCOL "\n");
    for(int y = 0; y < 16; ++y){
        chcolr(y%4);
        printf("%02d|", y);
        for(int x = 0; x < 4; ++x){
            dumpC(*buf++);
        }
        printf("|");
        for(int x = 4; x < 8; ++x){
            dumpC(*buf++);
        }
        printf("|" DEFCOL "\n");
    }
}

static uint8_t dmabuf[4][SCREENBUF_SZ/4];
void CSB(){
    uint8_t *screenbuf = getScreenBuf();
    for(uint8_t partNo = 0; partNo < 4; ++ partNo){ // cycle by strings
        uint8_t *dmaptr = dmabuf[partNo];
        for(int X = 0; X < SCREEN_WIDTH/8; ++X){
            for(int Y = SCREEN_HEIGHT-4+partNo; Y >= 0; Y -= 4){ // and cycle by Y
                *dmaptr++ = screenbuf[X + Y*(SCREEN_WIDTH/8)];
            }
        }
    }

}

void dumpdmastr(uint8_t n){
    if(n > 3) return;
    printf("\n");
    chcolr(n);
    printf("BUF[%d]:\n", n);
    uint8_t *ptr = dmabuf[n];
    for(int y = 0; y < 4; ++y){
        printf("%02d|", y);
        for(int x = 0; x < 4; ++x){
            dumpC(*ptr++);
        }
        printf("|");
        for(int x = 4; x < 8; ++x){
            dumpC(*ptr++);
        }
        printf("|\n");
    }
    printf(DEFCOL);
}

int main(int argc, char **argv){
    if(argc != 2){
        fprintf(stderr, "USAGE: %s string\n", argv[0]);
        return 1;
    }
    printf("\n\nFONT14:\n\n");
    PutStringAt(0, 15-curfont->baseline, argv[1]);
    dumpbuf();
    CSB(); //ConvertScreenBuf();
    for(uint8_t i = 0; i < 4; ++i) dumpdmastr(i);
/*
    printf("\n\nFONT16:\n\n");
    choose_font(FONT16);
    PutStringAt(0, 15-curfont->baseline, argv[1]);
    dumpbuf();
//    CSB(); //ConvertScreenBuf();
//    for(uint8_t i = 0; i < 4; ++i) dumpdmastr(i);
*/
    return 0;
}
