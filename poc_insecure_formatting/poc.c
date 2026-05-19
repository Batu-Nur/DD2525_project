#include <stdio.h>
#include <string.h>
#include <emscripten.h>


// TODO figure out a good POC
void  write_to_web(char* in){
     //IDK WHAT TO DO HERE

    // TODO GET DATA FROM USER AND DISPLAY IT
     char secret[] = "Admin:password";

     printf(in);
     printf("\n");
     printf("secret %s, addr: %p , in_addr %p\n", secret, (void *)secret, &in);
}
