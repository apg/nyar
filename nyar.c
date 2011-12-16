#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define BLACK 30
#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37


// current width of the terminal
static int current_width;

// possible foreground colors
static int foreground_colors[] = {
  RED, YELLOW, GREEN, BLUE, MAGENTA, CYAN, GREEN
};
static int _foreground_color_i = 0;

#define NEXT_FOREGROUND (_foreground_color_i++ % 7)

// possible background colors
static int background_colors[] = {
  RED+10, YELLOW+10, GREEN+10, BLUE+10, MAGENTA+10, CYAN+10, GREEN+10
};
static int _background_color_i = 0;

#define NEXT_BACKGROUND (_background_color_i++ % 7)

// tail make up
static char *tail_bits[2] = {
  "v", "~", "^", "~"
};
static int _tail_bit_i = 0;

#define NEXT_TAILBIT (_tail_bit_i++ % 4)

// leg shift
static int leg_shift[] = {
  -1, 0, 1
};

static int _leg_shift_i = 0;

#define NEXT_LEGSHIFT (_leg_shift_i++ % 3)

// toast position
static int toast[] = {
  1, 2
};

static int _toast_i = 0;

#define NEXT_TOAST (_toast_i++ % 2)

static char *stream[3] = {
  "~_", "_-"
};

#define NEXT_STREAM (_stream_i++ % 2)

#define CURSOR_UP_TMPL "\x1b[%dA"
#define FOREGROUND_TMPL "\x1b[1;%dm"
#define BACKGROUNDED_TMPL "\x1b[1;%d;%dm"
#define RESET "\x1b[0;m"

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

// wrap str in color, then reset the colors
static void
colored(int fg, int bg, char *str, char *buf, size_t bufsiz)
{
  char tmp[128];
  if (bg > 0) {
    snprintf(tmp, 128, BACKGROUNDED_TMPL, bg, fg);
  }
  else {
    snprintf(tmp, 128, FOREGROUND_TMPL, fg);
  }
  
  sprintf(buf, bufsiz, "%s%s%s", tmp, str, RESET);
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
draw_one(int amt)
{
  char buf[1024];
  char tmp[1024];
  int background = NEXT_BACKGROUND;
  int foreground = NEXT_FOREGROUND;
  char *st = NEXT_STREAM;
  char tailbit = NEXT_TAILBIT;
  int legshift = NEXT_LEGSHIFT;
  int toastpos = NEXT_TOAST;

  // STUB

  //#define CAT_1_TMPL "\x1b[2K%s,------,"
  //#define CAT_2_TMPL "\x1b[2K%s%s| %s /\\_/\\"
  //#define CAT_3_TMPL "\x1b[2K%s|_%s( ^ w^)"
  //#define CAT_4_TMPL "\x1b[2K%s    \"  \""
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
