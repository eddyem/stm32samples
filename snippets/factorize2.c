#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

uint16_t factorize(uint32_t in, uint16_t *o1, uint16_t *o2, uint16_t *niter){
    uint16_t min = 0xffff, minI = 2, i;
    *niter = 0;
    uint16_t start = (uint16_t)(in / 0xffff);
    if(start < 2) start = 2;
    for(i = start; i < 0xffff; ++i){
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
    uint64_t n = 0, p = 0;
    double N = 0.;
    time_t t = time(NULL), t0 = t;
    while(1){
        ++N;
        uint32_t i = lrand48() & 0xffffffff;
        uint16_t dn, o1, o2;
        p += factorize(i, &o1, &o2, &dn);
        n += dn;
        if(time(NULL) - t > 2){
            t = time(NULL);
            printf("N=%g, dt=%ld\n", N, time(NULL)-t0);
            printf("<p> = %g, <n> = %g\n", p/N, n/N);
        }
    }
    return 0;
}
