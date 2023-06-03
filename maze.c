#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <threads.h>
#include <unistd.h>

#define RIGHT 4
#define LEFT 1
#define DOWN 2
#define UP 3

typedef struct termios TERMSTATE;
typedef uint8_t DIRECTION;
typedef struct MAPSTRUCT {
    size_t x_size;
    size_t y_size;
    size_t p_x_cord;
    size_t p_y_cord;
    size_t s_x_cord;
    size_t s_y_cord;
    size_t f_x_cord;
    size_t f_y_cord;
    uint8_t **pntr;
} MAPSTRUCT;

TERMSTATE TSTATE;
MAPSTRUCT *MAP;

bool QUIT = false;
bool WIN = false;

size_t X_SIZE;
size_t Y_SIZE;

void get_size(const char *path) {
    FILE *file = fopen(path, "r");

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

    X_SIZE = x_cnt;
    Y_SIZE = y_cnt;
}

void move_cursor() {
    printf("\033[%zdD", Y_SIZE);
    printf("\033[%zdA", X_SIZE);
}

void move(DIRECTION dir) {
    switch (dir) {
    case 1:
        if (MAP->p_x_cord != 0 && MAP->pntr[MAP->p_y_cord][MAP->p_x_cord - 1] != 1) {
            if (MAP->p_x_cord == MAP->s_x_cord && MAP->p_y_cord == MAP->s_y_cord) {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 2;
            } else {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 0;
            }
            --MAP->p_x_cord;
            MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 4;
        }
        break;
    case 2:
        if (MAP->p_y_cord != MAP->y_size - 1 && MAP->pntr[MAP->p_y_cord + 1][MAP->p_x_cord] != 1) {
            if (MAP->p_x_cord == MAP->s_x_cord && MAP->p_y_cord == MAP->s_y_cord) {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 2;
            } else {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 0;
            }
            ++MAP->p_y_cord;
            MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 4;
        }
        break;
    case 3:
        if (MAP->p_y_cord != 0 && MAP->pntr[MAP->p_y_cord - 1][MAP->p_x_cord] != 1) {
            if (MAP->p_x_cord == MAP->s_x_cord && MAP->p_y_cord == MAP->s_y_cord) {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 2;
            } else {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 0;
            }
            --MAP->p_y_cord;
            MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 4;
        }
        break;
    case 4:
        if (MAP->p_x_cord != MAP->x_size - 1 && MAP->pntr[MAP->p_y_cord][MAP->p_x_cord + 1] != 1) {
            if (MAP->p_x_cord == MAP->s_x_cord && MAP->p_y_cord == MAP->s_y_cord) {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 2;
            } else {
                MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 0;
            }
            ++MAP->p_x_cord;
            MAP->pntr[MAP->p_y_cord][MAP->p_x_cord] = 4;
        }
        break;
    }
}

void free_map() {
    for (size_t y_cord = 0; y_cord < MAP->y_size; ++y_cord) {
        free(MAP->pntr[y_cord]);
    }
    free(MAP->pntr);
    free(MAP);
}

void alloc_map() {
    MAP = (MAPSTRUCT *)malloc(sizeof(MAPSTRUCT));
    MAP->x_size = X_SIZE;
    MAP->y_size = Y_SIZE;
    MAP->pntr = (uint8_t **)malloc(MAP->y_size * sizeof(uint8_t *));
    for (size_t y_cord = 0; y_cord < MAP->y_size; ++y_cord) {
        MAP->pntr[y_cord] = (uint8_t *)malloc(MAP->x_size * sizeof(uint8_t));
    }
}

void load_map(const char *path) {
    FILE *file = fopen(path, "r");

    size_t y_cord = 0;
    size_t x_cord = 0;

    char cur;
    while (EOF != (cur = getc(file))) {
        if ('\n' == cur) {
            x_cord = 0;
            ++y_cord;
            continue;
        } else if ('4' == cur) {
            MAP->p_x_cord = x_cord;
            MAP->p_y_cord = y_cord;
            MAP->s_x_cord = x_cord;
            MAP->s_y_cord = y_cord;
        } else if ('3' == cur) {
            MAP->f_x_cord = x_cord;
            MAP->f_y_cord = y_cord;
        }
        MAP->pntr[y_cord][x_cord] = (uint8_t)(cur - (char)'0');
        ++x_cord;
    }
}

void print_map() {
    const char sprites[] = " H*XO";
    for (size_t y_cord = 0; y_cord < MAP->y_size; ++y_cord) {
        for (size_t x_cord = 0; x_cord < MAP->x_size; ++x_cord) {
            putchar(sprites[MAP->pntr[y_cord][x_cord]]);
            putchar(' ');
        }
        putchar('\n');
    }
}

void set_term_def();
void set_term_raw();

void clear() {
    for (size_t y_cord = 0; y_cord < MAP->y_size; ++y_cord) {
        for (size_t x_cord = 0; x_cord < MAP->x_size; ++x_cord) {
            printf("  ");
        }
        printf("\n");
    }
}

void check_win() {
    if (MAP->p_y_cord == MAP->f_y_cord && MAP->p_x_cord == MAP->f_x_cord) {
        WIN = true;
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
        if (WIN) {
            break;
        }
    }

    QUIT = true;

    return EXIT_SUCCESS;
}

void *print() {
    while (!QUIT) { // When ESC is not pressed
        print_map();
        usleep(41667);
        move_cursor();
    }

    if (WIN) {
        clear();
        move_cursor();
        printf("Congratulations! You have won the game.\n");
    }

    return EXIT_SUCCESS;
}

void set_term_raw() {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);              // Save the state of the terminal to struct raw
                                                // STDIN_FILENO is from <stdlib.h>
                                                // tcgetattr() from <termios.h>
    tcgetattr(STDIN_FILENO, &TSTATE);
    atexit(&set_term_def);                      // Revert to canonical mode when exiting the program
                                                // atext() from <stdlib.h>
    raw.c_lflag &= ~(ECHO | ICANON);            // Turn off canonical mode
                                                // Turn off ECHO mode so that keyboard is not
                                                // printing to terminal
                                                // ICANON and ECHO is bitflag. ~ is binary NOT operator

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);   // Set the terminal to be in raw mode
                                                // tcsetattr() from <termios.h>
}

void set_term_def() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &TSTATE); // Set terminal to TSTATE state
}

int main(int argc, char *argv[]) {
    if (argc > 1 && argc < 3) {
        get_size(argv[1]);
        alloc_map();
        load_map(argv[1]);

        pthread_t id_print, id_read;

        pthread_create(&id_print, NULL, print, NULL);
        pthread_create(&id_read, NULL, capture, NULL);

        pthread_join(id_print, NULL);
        pthread_join(id_read, NULL);
    }

    return 0;
}
