#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/time.h>

// This requires at least a C99 compiler

// Before running, connect the pulse gpio output pin to the input gpio
// input pin

int main(){

  int const fd = open("/dev/muon_timer", O_RDONLY);
  if( fd<0 ){
    int err = errno;
    printf("Unable to open /dev/muon_timer!\n");
    printf("\t%s\n", strerror(err));
    exit(1);
  }

  int const pulse = open("/sys/class/muon_timer/muon_timer/pulse", O_WRONLY);
  if( pulse<0 ){
    int err = errno;
    printf("Unable to open sysfs pulser!\n");
    printf("\t%s\n", strerror(err));
    exit(2);
  }

  int const sizets = sizeof(struct timespec);
  for(int i=0; i!=10; ++i){
    struct timespec ts;
    char const s[] = "1";
    int const len = strlen(s);
    if( write(pulse, (void*)s, len) != len){
      int err = errno;
      printf("Error in write to sysfs!\n");
      printf("\t%s\n", strerror(err));
      exit(3);
    }
    if( read(fd, (void*)&ts, sizets) != sizets ){
      int err = errno;
      printf("Error in read from device!\n");
      printf("\t%s\n", strerror(err));
      exit(4);
    }
    printf("%ld %ld\n", ts.tv_sec, ts.tv_nsec);
  }  
  close(fd);
  
  return 0;
}
