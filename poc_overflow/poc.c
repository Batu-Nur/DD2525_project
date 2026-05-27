#include <stdio.h>
#include <string.h>
#include <emscripten.h>


// TODO figure out a good POC
char*  write_to_web(char* in){
    char cmd[] = "console.log('hi')"; 

    //char temp[100];  // UNCOMMENT to change offsets cmd
    char arr[20];
    strcpy(arr, in);

    emscripten_run_script(cmd); // MODIFY DOM
    printf("%s",cmd);
    fflush(stdout);
    return "Done";
}