#include <stdio.h>
#include <string.h>
#include <emscripten.h>
#include <limits.h>

// TODO figure out a good POC
char*  write_to_web(int in){
    char *a[4];
    a[0] = "test:1";
    a[1] = "secret";
    a[2] = "Illegal";
    a[3] = "test3";

    printf("value of in: %d\n", in);
    if (in == 1 || in == 2 || in > INT_MAX || in < INT_MIN)
    {
        return "NOT ALLOWED";
    }
    else{
        printf("returned: %s\n", a[in]);
        return "Allowed";
    }
}