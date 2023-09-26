#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <threads.h>
#include <unistd.h>

#define RIGHT 4
#define LEFT 1
#define DOWN 2
#define UP 3

typedef struct termios tstate_t;

typedef uint8_t dir_t;

typedef struct entity_t {
  size_t X;
  size_t Y;
} entity_t;

typedef struct maze_t {
  size_t SIZE_X;
  size_t SIZE_Y;
  entity_t *PLAYER;
  entity_t *START;
  entity_t *FINISH;
  uint8_t **PNTR;
} maze_t;

tstate_t TSTATE;
size_t TERM_COL;
size_t TERM_ROW;
maze_t *MAZE;

bool QUIT = false;
bool WINSTATE = false;

void get_term_size() {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  TERM_ROW = w.ws_row;
  TERM_COL = w.ws_col;
}

void term_too_small_handler() {
  if (TERM_COL < 2 * MAZE->SIZE_X || TERM_ROW < MAZE->SIZE_Y) {
    printf("Terminal is too small to display the whole maze.\n\
Either make the terminal window bigger, or use a smaller maze map.\n");
    exit(0);
  }
}

void get_maze_size(const char *path) {
  FILE *file;
  if (!(file = fopen(path, "r"))) {
    printf("Couldn't find map file. Check file path.\n");
    exit(0);
  }

  char ch;
  size_t x_cnt = 0;
  size_t y_cnt = 0;
  bool cnt_x = true;
  while ((ch = getc(file)) != EOF) {
    if (ch == '\n') {
      ++y_cnt;
      cnt_x = false;
    } else if (cnt_x) {
      ++x_cnt;
    }
  }

  MAZE->SIZE_X = x_cnt;
  MAZE->SIZE_Y = y_cnt;
}

#define restore_cursor() printf("\033[u")
#define save_cursor() printf("\033[s")
#define zero_cursor()                                                          \
  printf("\033[0;0H");                                                         \
  fflush(stdout)

void move(dir_t dir) {
  switch (dir) {
  case 1:
    if (MAZE->PLAYER->X != 0 &&
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X - 1] != 1) {
      if (MAZE->PLAYER->X == MAZE->START->X &&
          MAZE->PLAYER->Y == MAZE->START->Y) {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 2;
      } else {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 0;
      }
      --MAZE->PLAYER->X;
      MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 4;
    }
    break;
  case 2:
    if (MAZE->PLAYER->Y != MAZE->SIZE_Y - 1 &&
        MAZE->PNTR[MAZE->PLAYER->Y + 1][MAZE->PLAYER->X] != 1) {
      if (MAZE->PLAYER->X == MAZE->START->X &&
          MAZE->PLAYER->Y == MAZE->START->Y) {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 2;
      } else {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 0;
      }
      ++MAZE->PLAYER->Y;
      MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 4;
    }
    break;
  case 3:
    if (MAZE->PLAYER->Y != 0 &&
        MAZE->PNTR[MAZE->PLAYER->Y - 1][MAZE->PLAYER->X] != 1) {
      if (MAZE->PLAYER->X == MAZE->START->X &&
          MAZE->PLAYER->Y == MAZE->START->Y) {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 2;
      } else {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 0;
      }
      --MAZE->PLAYER->Y;
      MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 4;
    }
    break;
  case 4:
    if (MAZE->PLAYER->X != MAZE->SIZE_X - 1 &&
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X + 1] != 1) {
      if (MAZE->PLAYER->X == MAZE->START->X &&
          MAZE->PLAYER->Y == MAZE->START->Y) {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 2;
      } else {
        MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 0;
      }
      ++MAZE->PLAYER->X;
      MAZE->PNTR[MAZE->PLAYER->Y][MAZE->PLAYER->X] = 4;
    }
    break;
  }
}

void free_map() {
  for (size_t y_cord = 0; y_cord < MAZE->SIZE_Y; ++y_cord) {
    free(MAZE->PNTR[y_cord]);
  }
  free(MAZE->PNTR);
  free(MAZE);
}

void alloc_maze_buffer(const char *path) {
  MAZE = (maze_t *)malloc(sizeof(maze_t));
  MAZE->PLAYER = (entity_t *)malloc(sizeof(entity_t));
  MAZE->START = (entity_t *)malloc(sizeof(entity_t));
  MAZE->FINISH = (entity_t *)malloc(sizeof(entity_t));
  get_maze_size(path);
  MAZE->PNTR = (uint8_t **)malloc(MAZE->SIZE_Y * sizeof(uint8_t *));
  for (size_t y_cord = 0; y_cord < MAZE->SIZE_Y; ++y_cord) {
    MAZE->PNTR[y_cord] = (uint8_t *)malloc(MAZE->SIZE_X * sizeof(uint8_t));
  }
}

void load_maze_to_buffer(const char *path) {
  FILE *file;
  if (!(file = fopen(path, "r"))) {
    printf("Couldn't find map file. Check file path.\n");
    exit(0);
  }

  size_t y_cord = 0;
  size_t x_cord = 0;

  char cur;
  while (EOF != (cur = getc(file))) {
    if ('\n' == cur) {
      x_cord = 0;
      ++y_cord;
      continue;
    } else if ('4' == cur) {
      MAZE->PLAYER->X = x_cord;
      MAZE->PLAYER->Y = y_cord;
      MAZE->START->X = x_cord;
      MAZE->START->Y = y_cord;
    } else if ('3' == cur) {
      MAZE->FINISH->X = x_cord;
      MAZE->FINISH->Y = y_cord;
    }
    MAZE->PNTR[y_cord][x_cord] = (uint8_t)(cur - (char)'0');
    ++x_cord;
  }
}

void print() {
  const char sprites[] = " H*XO";
  save_cursor();
  for (size_t y_cord = 0; y_cord < MAZE->SIZE_Y; ++y_cord) {
    for (size_t x_cord = 0; x_cord < MAZE->SIZE_X; ++x_cord) {
      putchar(sprites[MAZE->PNTR[y_cord][x_cord]]);
      if (x_cord < MAZE->SIZE_X - 1) {
        putchar(' ');
      }
    }
    if (y_cord < MAZE->SIZE_Y - 1) {
      putchar('\n');
    }
  }
  restore_cursor();
}

void set_term_def();
void set_term_raw();

void clear() {
  zero_cursor();
  save_cursor();
  for (size_t y_cord = 0; y_cord < TERM_ROW; ++y_cord) {
    for (size_t x_cord = 0; x_cord < TERM_COL; ++x_cord) {
      putchar(' ');
    }
    if (y_cord < MAZE->SIZE_Y - 1) {
      putchar('\n');
    }
  }
  restore_cursor();
}

void check_win() {
  if (MAZE->PLAYER->Y == MAZE->FINISH->Y &&
      MAZE->PLAYER->X == MAZE->FINISH->X) {
    WINSTATE = true;
  }
}

// Reads keyboard input
void *capture() {
  set_term_raw(); // local function: Enable Raw Mode

  char ch;
  while ((ch = getchar()) != 'q') {
    switch (ch) {
    case 'w':
      move(UP);
      break;
    case 'a':
      move(LEFT);
      break;
    case 's':
      move(DOWN);
      break;
    case 'd':
      move(RIGHT);
      break;
    }

    check_win();
    if (WINSTATE) {
      break;
    }
  }

  QUIT = true;

  return EXIT_SUCCESS;
}

void *update() {
  zero_cursor();
  clear();
  while (!QUIT) { // When ESC is not pressed
    print();
    usleep(41667);
  }

  if (WINSTATE) {
    clear();
    printf("Congratulations! You have won the game.\n");
  } else {
    clear();
    printf("Keyboard interrupt! Quitting now...\n");
  }

  return EXIT_SUCCESS;
}

void set_term_raw() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw); // Save the state of the terminal to struct raw
                                 // STDIN_FILENO is from <stdlib.h>
                                 // tcgetattr() from <termios.h>

  tcgetattr(STDIN_FILENO, &TSTATE);

  atexit(&set_term_def); // Revert to canonical mode when exiting the program
                         // atext() from <stdlib.h>
  raw.c_lflag &=
      ~(ECHO | ICANON); // Turn off canonical mode
                        // Turn off ECHO mode so that keyboard is not
                        // printing to terminal
                        // ICANON and ECHO is bitflag. ~ is binary NOT operator

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // Set the terminal to be in raw
                                            // mode tcsetattr() from <termios.h>
}

void set_term_def() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &TSTATE); // Set terminal to TSTATE state
}

int main(int argc, char *argv[]) {
  if (argc > 1 && argc < 3) {
    get_term_size();
    alloc_maze_buffer(argv[1]);
    term_too_small_handler();
    load_maze_to_buffer(argv[1]);

    pthread_t update_pid, capture_pid;

    pthread_create(&update_pid, NULL, update, NULL);
    pthread_create(&capture_pid, NULL, capture, NULL);

    pthread_join(update_pid, NULL);
    pthread_join(capture_pid, NULL);
  }

  return 0;
}
