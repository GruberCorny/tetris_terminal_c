#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

// Spielfeld Dimensionen
#define WIDTH 10
#define HEIGHT 20
#define PREVIEW_SIZE 4

// Farben für Terminal
#define COLOR_RESET "\033[0m"
#define COLOR_CYAN "\033[36m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_PURPLE "\033[35m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_BLUE "\033[34m"
#define COLOR_ORANGE "\033[38;5;208m"
#define COLOR_GRAY "\033[90m"

int shapes[7][4][4] = {
    // I
    {{0, 0, 0, 0},
     {1, 1, 1, 1},
     {0, 0, 0, 0},
     {0, 0, 0, 0}},
    // O
    {{0, 0, 0, 0},
     {0, 1, 1, 0},
     {0, 1, 1, 0},
     {0, 0, 0, 0}},
    // T
    {{0, 0, 0, 0},
     {0, 1, 1, 1},
     {0, 0, 1, 0},
     {0, 0, 0, 0}},
    // S
    {{0, 0, 0, 0},
     {0, 0, 1, 1},
     {0, 1, 1, 0},
     {0, 0, 0, 0}},
    // Z
    {{0, 0, 0, 0},
     {0, 1, 1, 0},
     {0, 0, 1, 1},
     {0, 0, 0, 0}},
    // J
    {{0, 0, 0, 0},
     {0, 1, 1, 1},
     {0, 0, 0, 1},
     {0, 0, 0, 0}},
    // L
    {{0, 0, 0, 0},
     {0, 1, 1, 1},
     {0, 1, 0, 0},
     {0, 0, 0, 0}}};

const char *colors[7] = {
    COLOR_CYAN,   // I
    COLOR_YELLOW, // O
    COLOR_PURPLE, // T
    COLOR_GREEN,  // S
    COLOR_RED,    // Z
    COLOR_BLUE,   // J
    COLOR_ORANGE  // L
};

typedef struct
{
    int x, y;
    int type;
    int rotation;
} Tetromino;

int board[HEIGHT][WIDTH] = {0};
int score = 0;
int level = 1;
int lines_cleared = 0;
int game_over = 0;

struct termios orig_termios;

#define INPUT_BUFFER_SIZE 10
char input_buffer[INPUT_BUFFER_SIZE];
int input_buffer_start = 0;
int input_buffer_end = 0;

void add_to_input_buffer(char c)
{
    int next = (input_buffer_end + 1) % INPUT_BUFFER_SIZE;
    if (next != input_buffer_start)
    {
        input_buffer[input_buffer_end] = c;
        input_buffer_end = next;
    }
}

int get_from_input_buffer(char *c)
{
    if (input_buffer_start == input_buffer_end)
    {
        return 0;
    }
    *c = input_buffer[input_buffer_start];
    input_buffer_start = (input_buffer_start + 1) % INPUT_BUFFER_SIZE;
    return 1;
}

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int kbhit()
{
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    int ch = getchar();

    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void clear_screen()
{
    printf("\033[2J\033[H");
}

void rotate_shape(int shape[4][4], int rotated[4][4])
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            rotated[i][j] = shape[3 - j][i];
        }
    }
}

int check_collision(Tetromino *t)
{
    int current_shape[4][4];
    memcpy(current_shape, shapes[t->type], sizeof(current_shape));

    for (int r = 0; r < t->rotation % 4; r++)
    {
        int temp[4][4];
        rotate_shape(current_shape, temp);
        memcpy(current_shape, temp, sizeof(current_shape));
    }

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (current_shape[i][j])
            {
                int x = t->x + j;
                int y = t->y + i;

                if (x < 0 || x >= WIDTH || y >= HEIGHT)
                    return 1;
                if (y >= 0 && board[y][x])
                    return 1;
            }
        }
    }
    return 0;
}

void merge_tetromino(Tetromino *t)
{
    int current_shape[4][4];
    memcpy(current_shape, shapes[t->type], sizeof(current_shape));

    for (int r = 0; r < t->rotation % 4; r++)
    {
        int temp[4][4];
        rotate_shape(current_shape, temp);
        memcpy(current_shape, temp, sizeof(current_shape));
    }

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (current_shape[i][j])
            {
                int x = t->x + j;
                int y = t->y + i;
                if (y >= 0 && y < HEIGHT && x >= 0 && x < WIDTH)
                {
                    board[y][x] = t->type + 1;
                }
            }
        }
    }
}

int clear_lines()
{
    int cleared = 0;

    for (int i = HEIGHT - 1; i >= 0; i--)
    {
        int full = 1;
        for (int j = 0; j < WIDTH; j++)
        {
            if (!board[i][j])
            {
                full = 0;
                break;
            }
        }

        if (full)
        {
            cleared++;
            for (int k = i; k > 0; k--)
            {
                for (int j = 0; j < WIDTH; j++)
                {
                    board[k][j] = board[k - 1][j];
                }
            }
            for (int j = 0; j < WIDTH; j++)
            {
                board[0][j] = 0;
            }
            i++;
        }
    }

    return cleared;
}

void draw_board(Tetromino *current)
{
    clear_screen();

    printf("\n");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║              TETRIS GAME             ║\n");
    printf("  ╚══════════════════════════════════════╝\n\n");

    printf("  Score: %d    Level: %d    Lines: %d\n\n", score, level, lines_cleared);

    int display[HEIGHT][WIDTH];
    memcpy(display, board, sizeof(board));

    if (current)
    {
        int current_shape[4][4];
        memcpy(current_shape, shapes[current->type], sizeof(current_shape));

        for (int r = 0; r < current->rotation % 4; r++)
        {
            int temp[4][4];
            rotate_shape(current_shape, temp);
            memcpy(current_shape, temp, sizeof(current_shape));
        }

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (current_shape[i][j])
                {
                    int x = current->x + j;
                    int y = current->y + i;
                    if (y >= 0 && y < HEIGHT && x >= 0 && x < WIDTH)
                    {
                        display[y][x] = current->type + 1;
                    }
                }
            }
        }
    }

    printf("  ╔");
    for (int i = 0; i < WIDTH * 2; i++)
        printf("═");
    printf("╗\n");

    for (int i = 0; i < HEIGHT; i++)
    {
        printf("  ║");
        for (int j = 0; j < WIDTH; j++)
        {
            if (display[i][j])
            {
                printf("%s██%s", colors[display[i][j] - 1], COLOR_RESET);
            }
            else
            {
                if ((i + j) % 2 == 0)
                {
                    printf("%s░░%s", COLOR_GRAY, COLOR_RESET);
                }
                else
                {
                    printf("  ");
                }
            }
        }
        printf("║\n");
    }

    printf("  ╚");
    for (int i = 0; i < WIDTH * 2; i++)
        printf("═");
    printf("╝\n\n");

    printf("  Steuerung:\n");
    printf("  ← → : Bewegen    ↓ : Schneller    ↑ : Rotieren    Q : Beenden\n");

    fflush(stdout);
}

Tetromino create_tetromino()
{
    Tetromino t;
    t.type = rand() % 7;
    t.x = WIDTH / 2 - 2;
    t.y = -1;
    t.rotation = 0;
    return t;
}

int main()
{
    srand(time(NULL));
    enable_raw_mode();

    Tetromino current = create_tetromino();

    clock_t last_fall = clock();
    clock_t last_draw = clock();
    int fall_speed = 30000;
    int draw_interval = 1600;
    int needs_redraw = 1;

    printf("\033[?25l"); // Cursor verstecken

    while (!game_over)
    {
        while (kbhit())
        {
            char c = getchar();
            add_to_input_buffer(c);
        }

        int input_handled = 0;

        char c;
        while (get_from_input_buffer(&c))
        {
            if (c == 'q' || c == 'Q')
            {
                game_over = 1;
                break;
            }

            Tetromino temp = current;

            if (c == 'a' || c == 'A')
            {
                temp.x--;
                if (!check_collision(&temp))
                {
                    current = temp;
                    input_handled = 1;
                }
            }
            else if (c == 'd' || c == 'D')
            {
                temp.x++;
                if (!check_collision(&temp))
                {
                    current = temp;
                    input_handled = 1;
                }
            }
            else if (c == 's' || c == 'S')
            {
                temp.y++;
                if (!check_collision(&temp))
                {
                    current = temp;
                    input_handled = 1;
                }
            }
            else if (c == 'w' || c == 'W')
            {
                temp.rotation++;
                if (!check_collision(&temp))
                {
                    current = temp;
                    input_handled = 1;
                }
            }

            else if (c == 27)
            {
                char next1 = getchar();
                if (next1 == '[')
                {
                    c = getchar();
                    temp = current;

                    if (c == 'A')
                    {
                        temp.rotation++;
                        if (!check_collision(&temp))
                        {
                            current = temp;
                            input_handled = 1;
                        }
                    }
                    else if (c == 'B')
                    {
                        temp.y++;
                        if (!check_collision(&temp))
                        {
                            current = temp;
                            input_handled = 1;
                        }
                    }
                    else if (c == 'C')
                    {
                        temp.x++;
                        if (!check_collision(&temp))
                        {
                            current = temp;
                            input_handled = 1;
                        }
                    }
                    else if (c == 'D')
                    {
                        temp.x--;
                        if (!check_collision(&temp))
                        {
                            current = temp;
                            input_handled = 1;
                        }
                    }
                }
            }
        }

        if (input_handled)
        {
            needs_redraw = 1;
        }

        clock_t now = clock();
        if ((now - last_fall) * 1000000 / CLOCKS_PER_SEC >= fall_speed)
        {
            Tetromino temp = current;
            temp.y++;

            if (check_collision(&temp))
            {
                merge_tetromino(&current);

                int cleared = clear_lines();
                if (cleared > 0)
                {
                    lines_cleared += cleared;
                    score += cleared * cleared * 100;
                    level = 1 + lines_cleared / 10;
                    fall_speed = 300000 - (level - 1) * 25000;
                    if (fall_speed < 80000)
                        fall_speed = 80000;
                }
                current = create_tetromino();

                if (check_collision(&current))
                {
                    game_over = 1;
                }
            }
            else
            {
                current = temp;
            }

            last_fall = now;
            needs_redraw = 1;
        }

        if (needs_redraw && (now - last_draw) * 1000000 / CLOCKS_PER_SEC >= draw_interval)
        {
            draw_board(&current);
            last_draw = now;
            needs_redraw = 0;
        }

        usleep(500);
    }

    printf("\033[?25h");

    clear_screen();
    printf("\n\n");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║           GAME OVER!                 ║\n");
    printf("  ╠══════════════════════════════════════╣\n");
    printf("  ║  Final Score: %-21d ║\n", score);
    printf("  ║  Level: %-28d ║\n", level);
    printf("  ║  Lines: %-28d ║\n", lines_cleared);
    printf("  ╚══════════════════════════════════════╝\n\n");

    disable_raw_mode();

    return 0;
}
