#include <sys/ioctl.h>
#include <sys/types.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MIN(x, y) (x > y ? y: x)

static int current_width;
static int needs_relaunch = 0;

#define NEXT_THROBBER (_throbber[(_throbber_i++) % 4])
static char _throbber[] = {
  '/', '-', '\\', '|'
};
static int _throbber_i = 0;


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
  case SIGCHLD:
    printf("sigchld\n");
    needs_relaunch = 1;
    break;
  }
}

static void
fill(char *dest, char *src, size_t maxsize, size_t srcsiz)
{
  int i = 0;
  while (i < maxsize) {
    strncpy(&dest[i], src, srcsiz);
    i += srcsiz;
  }
  // TODO: there's an off by maxsize - at least srcsiz - 
  // 1 in the case that these are not evenly divisible.
}

static void
draw_one(int amt, int max)
{
  int i;
  char fmt[32];
  char buf[1024];
  float percentage = amt / (float)max;
  int width = MIN((int)((current_width - 11) * percentage), 1024);
  

  fill(buf, "=", width, 1);
  buf[width] = 0;

  memset(fmt, 0, 32);

  // create format string which takes into consideration width
  snprintf(fmt, 32, "[%%-%ds] %%c %%-4.1f%%%%\n", (current_width - 11));
  printf(fmt, buf, NEXT_THROBBER, 100 * percentage);
}

static int last_number = 0;

static int
read_last_number(int fd)
{
  char buf[1024];
  int br;
  int i;
  br = read(fd, buf, 1024);
  // TODO: this is broken severely
  if (br > 0) {
    buf[br-1] = 0;
    return atoi(buf);
  }
  return 0;
}

static void
main_loop(int finish, int argc, char *argv[])
{
  int i = 0; // tmp
  char buf[10];
  int pdes[2] = {-1, -1};
  pid_t chld;
  int progress = 0;
  int completed = 0;
  int ps;
  struct pollfd pfd;

 loop:
  needs_relaunch = 0;

  if (pipe(pdes) < 0) {
    perror("pipe");
    _exit(EXIT_FAILURE);
  }

  chld = fork();
  if (chld < 0) {
    // FAIL
    perror("fork");
    _exit(EXIT_FAILURE);
  }
  else if (chld == 0) {
    close(pdes[0]); // close read

    if (pdes[1] > STDOUT_FILENO) {
      if (dup2(pdes[1], STDOUT_FILENO) < 0) {
        perror("dup2-child");
        _exit(EXIT_FAILURE);
      }
      close(pdes[1]);
    }

    // EXEC Should happen here with /bin/sh -c {whatever}
    execl("/bin/sh", "-c", "/bin/echo", "4");
  }
  else {
    close(pdes[1]); // close pipe write end

    if (pdes[0] > 0) {
      if (dup2(pdes[0], STDIN_FILENO) < 0) { // dup2 to stdin
        perror("dup2");
        _exit(EXIT_FAILURE);
      }
      close(pdes[0]);
    }

    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;


    completed = (progress / finish) >= 1;
    // poll here on STDIN_FILENO and update after each timeout
    while (!needs_relaunch && !completed) {
      ps = poll(&pfd, 1, 200);
      if (ps < 0) {
        perror("perror");        
        _exit(EXIT_FAILURE);
      }
      else if (ps > 0) {
        progress = read_last_number(STDIN_FILENO);
      }
      draw_one(progress, finish);
      completed = (progress / finish) >= 1;
    }

    if (!completed && needs_relaunch) {
      sleep(1);
      goto loop;
    }
    kill(chld, SIGTERM);
    waitpid(&chld);
  }
}

int
main(int argc, char **argv)
{
  struct sigaction saction;
  int i = 0; 

  sigemptyset(&saction.sa_mask);
  saction.sa_flags = 0;
  saction.sa_handler = signal_handler;

  if (sigaction(SIGWINCH, &saction, NULL)) {
    perror("sigaction");
    _exit(EXIT_FAILURE);
  }
  if (sigaction(SIGCHLD, &saction, NULL)) {
    perror("sigaction");
    _exit(EXIT_FAILURE);
  }

  current_width = term_width();

  if (argc > 2) {
    main_loop(100, argc - 2, &argv[2]);
  }
  else {
    printf("usage: %s <max val> <cmd> <cmd arg 1> <cmd arg ...> <cmd arg N>\n", 
           argv[0]);
    _exit(EXIT_FAILURE);
  }

  return 0;
}
