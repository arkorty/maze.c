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
typedef struct maze_t {
    size_t X_SIZE;
    size_t Y_SIZE;
    size_t P_X_CORD;
    size_t P_Y_CORD;
    size_t S_X_CORD;
    size_t S_Y_CORD;
    size_t F_X_CORD;
    size_t F_Y_CORD;
    uint8_t **PNTR;
} maze_t;

tstate_t TSTATE;
maze_t *MAZE;

bool QUIT = false;
bool WINSTATE = false;

size_t TERM_COL;
size_t TERM_ROW;

void get_term_size() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    TERM_ROW = w.ws_row;
    TERM_COL = w.ws_col;
}

void exit_on_inadequate_space() {
    if (TERM_COL < 2 * MAZE->X_SIZE || TERM_ROW < MAZE->Y_SIZE) {
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

    MAZE->X_SIZE = x_cnt;
    MAZE->Y_SIZE = y_cnt;
}

#define restore_cursor() printf("\033[u")
#define save_cursor() printf("\033[s")
#define zero_cursor() printf("\033[0;0H"); fflush(stdout)

void move(dir_t dir) {
    switch (dir) {
    case 1:
        if (MAZE->P_X_CORD != 0 && MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD - 1] != 1) {
            if (MAZE->P_X_CORD == MAZE->S_X_CORD && MAZE->P_Y_CORD == MAZE->S_Y_CORD) {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 2;
            } else {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 0;
            }
            --MAZE->P_X_CORD;
            MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 4;
        }
        break;
    case 2:
        if (MAZE->P_Y_CORD != MAZE->Y_SIZE - 1 && MAZE->PNTR[MAZE->P_Y_CORD + 1][MAZE->P_X_CORD] != 1) {
            if (MAZE->P_X_CORD == MAZE->S_X_CORD && MAZE->P_Y_CORD == MAZE->S_Y_CORD) {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 2;
            } else {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 0;
            }
            ++MAZE->P_Y_CORD;
            MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 4;
        }
        break;
    case 3:
        if (MAZE->P_Y_CORD != 0 && MAZE->PNTR[MAZE->P_Y_CORD - 1][MAZE->P_X_CORD] != 1) {
            if (MAZE->P_X_CORD == MAZE->S_X_CORD && MAZE->P_Y_CORD == MAZE->S_Y_CORD) {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 2;
            } else {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 0;
            }
            --MAZE->P_Y_CORD;
            MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 4;
        }
        break;
    case 4:
        if (MAZE->P_X_CORD != MAZE->X_SIZE - 1 && MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD + 1] != 1) {
            if (MAZE->P_X_CORD == MAZE->S_X_CORD && MAZE->P_Y_CORD == MAZE->S_Y_CORD) {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 2;
            } else {
                MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 0;
            }
            ++MAZE->P_X_CORD;
            MAZE->PNTR[MAZE->P_Y_CORD][MAZE->P_X_CORD] = 4;
        }
        break;
    }
}

void free_map() {
    for (size_t y_cord = 0; y_cord < MAZE->Y_SIZE; ++y_cord) {
        free(MAZE->PNTR[y_cord]);
    }
    free(MAZE->PNTR);
    free(MAZE);
}

void alloc_maze_buffer(const char *path) {
    MAZE = (maze_t *)malloc(sizeof(maze_t));
    get_maze_size(path);
    MAZE->PNTR = (uint8_t **)malloc(MAZE->Y_SIZE * sizeof(uint8_t *));
    for (size_t y_cord = 0; y_cord < MAZE->Y_SIZE; ++y_cord) {
        MAZE->PNTR[y_cord] = (uint8_t *)malloc(MAZE->X_SIZE * sizeof(uint8_t));
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
            MAZE->P_X_CORD = x_cord;
            MAZE->P_Y_CORD = y_cord;
            MAZE->S_X_CORD = x_cord;
            MAZE->S_Y_CORD = y_cord;
        } else if ('3' == cur) {
            MAZE->F_X_CORD = x_cord;
            MAZE->F_Y_CORD = y_cord;
        }
        MAZE->PNTR[y_cord][x_cord] = (uint8_t)(cur - (char)'0');
        ++x_cord;
    }
}

void print() {
    const char sprites[] = " H*XO";
    save_cursor();
    for (size_t y_cord = 0; y_cord < MAZE->Y_SIZE; ++y_cord) {
        for (size_t x_cord = 0; x_cord < MAZE->X_SIZE; ++x_cord) {
            putchar(sprites[MAZE->PNTR[y_cord][x_cord]]);
            if (x_cord < MAZE->X_SIZE - 1) {
                putchar(' ');
            }
        }
        if (y_cord < MAZE->Y_SIZE - 1) {
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
        if (y_cord < MAZE->Y_SIZE - 1) {
            putchar('\n');
        }
    }
    restore_cursor();
}

void check_win() {
    if (MAZE->P_Y_CORD == MAZE->F_Y_CORD && MAZE->P_X_CORD == MAZE->F_X_CORD) {
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

    atexit(&set_term_def);           // Revert to canonical mode when exiting the program
                                     // atext() from <stdlib.h>
    raw.c_lflag &= ~(ECHO | ICANON); // Turn off canonical mode
                                     // Turn off ECHO mode so that keyboard is not
                                     // printing to terminal
                                     // ICANON and ECHO is bitflag. ~ is binary NOT operator

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // Set the terminal to be in raw mode
                                              // tcsetattr() from <termios.h>
}

void set_term_def() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &TSTATE); // Set terminal to TSTATE state
}

int main(int argc, char *argv[]) {
    if (argc > 1 && argc < 3) {
        get_term_size();
        alloc_maze_buffer(argv[1]);
        exit_on_inadequate_space();
        load_maze_to_buffer(argv[1]);

        pthread_t update_pid, capture_pid;

        pthread_create(&update_pid, NULL, update, NULL);
        pthread_create(&capture_pid, NULL, capture, NULL);

        pthread_join(update_pid, NULL);
        pthread_join(capture_pid, NULL);
    }

    return 0;
}
