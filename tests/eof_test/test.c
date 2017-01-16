#include <string.h>
#include <stdio.h>

#define BUFFERS 8
#define BUFSIZE 256

char buffers[BUFFERS][BUFSIZE];

int main(){

  int i;
  for(i=0; i<BUFFERS; ++i){
    strncpy(buffers[i], "a\n", 3);
  }

  for(i=0; i<BUFFERS; ++i){
    printf("%s", buffers[i]);
  }
  return 0;
}
