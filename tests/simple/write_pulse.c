#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(){

  int muons = open("/dev/muon_timer", O_RDONLY);
  int pulse = open("/sys/class/muon_timer/muon_timer/pulse", O_WRONLY | O_SYNC);

  sleep(1);
  char one = '1';
  write(pulse, &one, sizeof(one));

  close(pulse);
  close(muons);

  return 0;
}
