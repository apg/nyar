#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <poll.h>
#include <fcntl.h>
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
  '-', '\\', '|', '/'
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
    needs_relaunch = 1;
    break;
  }
}

static void
fill(char *dest, char *src, size_t maxsize, size_t srcsiz)
{
  unsigned int i = 0;
  while (i < maxsize) {
    strncpy(&dest[i], src, srcsiz);
    i += srcsiz;
  }
  // TODO: there's an off by maxsize - at least srcsiz - 
  // 1 in the case that these are not evenly divisible.
}


static void 
msleep(long ms)
{
  struct timespec tm;
  tm.tv_sec = ms / 1000;
  tm.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&tm, NULL); // if there's an error, I don't care here.
}

static void
draw_one(int amt, int max)
{
  int width;
  char fmt[64];
  char buf[1024];
  float percentage = amt / (float)max;

  if (percentage > 1.0) {
    percentage = 1.0;
  }
  
  width = MIN((int)((current_width - 11) * percentage), 1024);

  fill(buf, "=", width, 1);
  buf[width] = 0;

  memset(fmt, 0, 64);

  // create format string which takes into consideration width
  snprintf(fmt, 64, "\n\033[1A\033[2K[%%-%ds] %%c %%-4.1f%%%%", (current_width - 11));
  printf(fmt, buf, NEXT_THROBBER, 100 * percentage);
}

static int last_number = 0;

static int
read_last_number(int fd)
{
  char buf[1024];
  int br;
  int tmp;
  br = read(fd, buf, 1024);
  // TODO: this is broken severely
  if (br > 0) {
    buf[br-1] = 0;
    tmp = atoi(buf);
    // only ever make forward progress.
    if (tmp > last_number) {
      last_number = tmp;
      return tmp;
    }
    return last_number;
  }
  return -1;
}

static void
main_loop(int finish, int argc, char *argv[])
{
  char *newargs[4];
  int pdes[2] = {-1, -1};
  pid_t chld;
  int chld_status;
  int progress = 0;
  int tprog = 0;
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
    perror("fork");
    _exit(EXIT_FAILURE);
  }
  else if (chld == 0) {
    close(pdes[0]); // close read

    if (pdes[1] > STDOUT_FILENO) {
      if (dup2(pdes[1], STDOUT_FILENO) < 0) {
        perror("dup2-child");
        _exit(EXIT_FAILURE);
        close(pdes[1]);
      }
    }

    if (argc > 1) {
      fprintf(stderr, "too many arguments");
      _exit(EXIT_FAILURE);
    }

    newargs[0] = "/bin/sh";
    newargs[1] = "-c";
    newargs[2] = strdup(argv[0]);
    newargs[3] = 0;

    if (execv("/bin/sh", newargs) < 0) {
      perror("execv");
      _exit(EXIT_FAILURE);
    }
  }
  else {
    close(pdes[1]); // close pipe write end

    if (pdes[0] > 0) {
      if (dup2(pdes[0], STDIN_FILENO) < 0) { // dup2 to stdin
        perror("dup2");
        _exit(EXIT_FAILURE);
        close(pdes[0]);
      }
    }

    if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) < 0) {
      perror("fcntl");
      _exit(EXIT_FAILURE);
    }

    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;

    completed = (progress / finish) >= 1;
    // poll here on STDIN_FILENO and update after each timeout
    while (!needs_relaunch && !completed) {
      ps = poll(&pfd, 1, 50);
      if (ps < 0) {
        perror("perror");        
        _exit(EXIT_FAILURE);
      }
      else if (ps > 0) {
        tprog = read_last_number(STDIN_FILENO);
        if (tprog >= 0) {
          progress = tprog;
        }
        draw_one(progress, finish);
        completed = (progress / finish) >= 1;
      }
    }

    if (!completed && needs_relaunch) {
      // nanosleep
      msleep(500);
      close(STDIN_FILENO);
      goto loop;
    }
    kill(chld, SIGTERM);
    waitpid(chld, &chld_status, 0);
  }
}

int
main(int argc, char **argv)
{
  struct sigaction saction;

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
    main_loop(atoi(argv[1]), argc - 2, &argv[2]);
  }
  else {
    printf("usage: %s <max val> <cmd>\n", 
           argv[0]);
    _exit(EXIT_FAILURE);
  }
  printf("\n");
  return 0;
}
