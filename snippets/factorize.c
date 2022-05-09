#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

uint16_t factorize1(uint32_t in, uint16_t *o1, uint16_t *o2, uint16_t *niter){
    uint16_t min = 0xffff, minI = 2, i;
    *niter = 0;
    for(i = 2; i < 0xffff; ++i){
        ++(*niter);
        uint32_t a = in/i;
        if(a > 0xffff) continue;
        uint16_t d = abs(in - i*a);
        if(d < min){
            min = d; minI = i;
        }
        if(min == 0) break;
    }
    *o1 = in/minI;
    *o2 = minI;
    return min;
}
uint16_t factorize2(uint32_t in, uint16_t *o1, uint16_t *o2, uint16_t *niter){
    if(in < 0xffff){
        *o1 = 1; *o2 = (uint16_t) in; return 0;
    }
    uint16_t x = 0x8000, d = 0;
    while(d < 2){
        d = in/x;
        x >>= 1;
    }
    if(d < x) d = x;
    uint16_t min = 0xffff, minI = 2, i;
    *niter = 0;
    for(i = d; i < 0xffff; ++i){
        ++(*niter);
        uint32_t a = in/i;
        if(a > 0xffff) continue;
        uint16_t d = abs(in - i*a);
        if(d < min){
            min = d; minI = i;
        }
        if(min == 0) break;
    }
    *o1 = in/minI;
    *o2 = minI;
    return min;
}

int main(int argc, char **argv){
    uint64_t n1 = 0, n2 = 0, p1 = 0, p2 = 0;
    double N = 0.;
    time_t t = time(NULL);
    //for(uint32_t i = 1; i < 0xffffffff; ++i){
    while(1){
        ++N;
        uint32_t i = lrand48() & 0xffffffff;
        uint16_t n, o1, o2;
        p1 += factorize1(i, &o1, &o2, &n);
        n1 += n;
        p2 += factorize2(i, &o1, &o2, &n);
        n2 += n;
        if(time(NULL) - t > 2){
            t = time(NULL);
            printf("n1=%lu, n2=%lu\n", n1, n2);
            printf("1: <p> = %g, <n> = %g\n", p1/N, n1/N);
            printf("2: <p> = %g, <n> = %g\n", p2/N, n2/N);
        }
    }
    return 0;
}
