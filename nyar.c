#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

static int current_width;

static int
term_width() 
{
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
}

static void
signal_handler(int signum)
{
  switch (signum) {
  case SIGWINCH:
    current_width = term_width();
    break;
  }
}


int
main(int argc, char **argv)
{
  struct sigaction saction;
  int again_80 = -1;

  sigemptyset(&saction.sa_mask);
  saction.sa_flags = 0;
  saction.sa_handler = signal_handler;

  if (sigaction(SIGWINCH, &saction, NULL)) {
    perror("sigaction");
    exit(1);
  }

  current_width = term_width();

  while (again_80 < 1) {
    if (current_width != 80) {
      again_80 = 0;
      printf("Current width: %d\n", current_width);
      sleep(1);
    }
    else if (current_width == 80 && again_80 > 0) {
      break;
    }
    else if (current_width == 80 && !again_80) {
      again_80 = 1;
    }
  }
  return 0;
}
