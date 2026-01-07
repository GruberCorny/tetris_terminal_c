#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>

// Spielfeld Dimensionen
#define WIDTH 10
#define HEIGHT 20

// Tetromino Formen (7 verschiedene)
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

// Farben (ncurses color pairs)
#define COLOR_PAIR_I 1
#define COLOR_PAIR_O 2
#define COLOR_PAIR_T 3
#define COLOR_PAIR_S 4
#define COLOR_PAIR_Z 5
#define COLOR_PAIR_J 6
#define COLOR_PAIR_L 7

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

// Hold und Next Steine
int hold_piece = -1; // -1 = kein Stein gespeichert
int can_hold = 1;    // Kann nur einmal pro Stein gehalten werden
#define NEXT_PIECES 4
int next_pieces[NEXT_PIECES];

// Bag System für faire Verteilung
int bag[7];
int bag_index = 7; // Startet bei 7, damit sofort ein neuer Bag erstellt wird

void shuffle_bag()
{
    // Alle 7 Steine in den Bag
    for (int i = 0; i < 7; i++)
    {
        bag[i] = i;
    }

    // Fisher-Yates Shuffle
    for (int i = 6; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int temp = bag[i];
        bag[i] = bag[j];
        bag[j] = temp;
    }

    bag_index = 0;
}

int get_random_piece()
{
    if (bag_index >= 7)
    {
        shuffle_bag();
    }
    return bag[bag_index++];
}

void init_next_pieces()
{
    for (int i = 0; i < NEXT_PIECES; i++)
    {
        next_pieces[i] = get_random_piece();
    }
}

int get_next_piece()
{
    int piece = next_pieces[0];
    // Alle nach vorne schieben
    for (int i = 0; i < NEXT_PIECES - 1; i++)
    {
        next_pieces[i] = next_pieces[i + 1];
    }
    // Neuen am Ende generieren mit Bag-System
    next_pieces[NEXT_PIECES - 1] = get_random_piece();
    return piece;
}

void init_colors()
{
    start_color();
    // Farbige Blöcke mit schwarzem Hintergrund
    init_pair(COLOR_PAIR_I, COLOR_BLACK, COLOR_CYAN);    // I - Cyan
    init_pair(COLOR_PAIR_O, COLOR_BLACK, COLOR_YELLOW);  // O - Gelb
    init_pair(COLOR_PAIR_T, COLOR_BLACK, COLOR_MAGENTA); // T - Magenta
    init_pair(COLOR_PAIR_S, COLOR_BLACK, COLOR_GREEN);   // S - Grün
    init_pair(COLOR_PAIR_Z, COLOR_BLACK, COLOR_RED);     // Z - Rot
    init_pair(COLOR_PAIR_J, COLOR_BLACK, COLOR_BLUE);    // J - Blau
    init_pair(COLOR_PAIR_L, COLOR_BLACK, COLOR_WHITE);   // L - Weiß
    init_pair(8, COLOR_WHITE, COLOR_BLACK);              // Für Rahmen
    init_pair(9, COLOR_BLACK, COLOR_BLACK);              // Dunkles Schachbrett
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
    clear();

    // Titel
    mvprintw(0, 2, "=== TETRIS ===");
    mvprintw(1, 2, "Score: %d  Level: %d  Lines: %d", score, level, lines_cleared);
    mvprintw(2, 2, "<- -> : Bewegen  |  v : Runter  |  ^/W : Hard Drop  |  R : Rotieren  |  E : Hold  |  Q : Beenden");

    // Temporäres Board für Anzeige
    int display[HEIGHT][WIDTH];
    memcpy(display, board, sizeof(board));

    // Aktuellen Tetromino hinzufügen
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

    // Spielfeld zeichnen
    int start_y = 4;
    int start_x = 2;

    // Oberer Rand
    attron(COLOR_PAIR(8) | A_BOLD);
    mvaddch(start_y, start_x, '+');
    for (int i = 0; i < WIDTH * 2; i++)
        addch('=');
    addch('+');
    attroff(COLOR_PAIR(8) | A_BOLD);

    // Spielfeld
    for (int i = 0; i < HEIGHT; i++)
    {
        attron(COLOR_PAIR(8) | A_BOLD);
        mvaddch(start_y + i + 1, start_x, '|');
        attroff(COLOR_PAIR(8) | A_BOLD);

        for (int j = 0; j < WIDTH; j++)
        {
            if (display[i][j])
            {
                // Farbige Blöcke mit fettem Text
                attron(COLOR_PAIR(display[i][j]) | A_BOLD);
                addstr("  "); // Volle Blöcke
                attroff(COLOR_PAIR(display[i][j]) | A_BOLD);
            }
            else
            {
                // Raster mit einfachen Punkten (jede 2. Zeile und Spalte)
                if (i % 2 == 0 && j % 2 == 0)
                {
                    attron(A_DIM);
                    addstr(". ");
                    attroff(A_DIM);
                }
                else
                {
                    addstr("  ");
                }
            }
        }

        attron(COLOR_PAIR(8) | A_BOLD);
        addch('|');
        attroff(COLOR_PAIR(8) | A_BOLD);
    }

    // Unterer Rand
    attron(COLOR_PAIR(8) | A_BOLD);
    mvaddch(start_y + HEIGHT + 1, start_x, '+');
    for (int i = 0; i < WIDTH * 2; i++)
        addch('=');
    addch('+');
    attroff(COLOR_PAIR(8) | A_BOLD);

    // HOLD Box zeichnen (links vom Spielfeld)
    int hold_x = start_x + WIDTH * 2 + 5;
    mvprintw(start_y, hold_x, "HOLD (E):");
    mvaddch(start_y + 1, hold_x, '+');
    for (int i = 0; i < 10; i++)
        addch('-');
    addch('+');

    for (int i = 0; i < 4; i++)
    {
        mvaddch(start_y + 2 + i, hold_x, '|');
        for (int j = 0; j < 5; j++)
            addstr("  ");
        addch('|');
    }

    mvaddch(start_y + 6, hold_x, '+');
    for (int i = 0; i < 10; i++)
        addch('-');
    addch('+');

    // Hold Piece zeichnen
    if (hold_piece >= 0)
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (shapes[hold_piece][i][j])
                {
                    attron(COLOR_PAIR(hold_piece + 1) | A_BOLD);
                    mvaddstr(start_y + 2 + i, hold_x + 2 + j * 2, "  ");
                    attroff(COLOR_PAIR(hold_piece + 1) | A_BOLD);
                }
            }
        }
    }

    // NEXT Boxes zeichnen (rechts vom Spielfeld)
    int next_x = hold_x + 15;
    mvprintw(start_y, next_x, "NEXT:");

    for (int n = 0; n < NEXT_PIECES; n++)
    {
        int box_y = start_y + 1 + n * 5;

        mvaddch(box_y, next_x, '+');
        for (int i = 0; i < 10; i++)
            addch('-');
        addch('+');

        for (int i = 0; i < 4; i++)
        {
            mvaddch(box_y + 1 + i, next_x, '|');
            for (int j = 0; j < 5; j++)
                addstr("  ");
            addch('|');
        }

        // Next Piece zeichnen
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (shapes[next_pieces[n]][i][j])
                {
                    attron(COLOR_PAIR(next_pieces[n] + 1) | A_BOLD);
                    mvaddstr(box_y + 1 + i, next_x + 2 + j * 2, "  ");
                    attroff(COLOR_PAIR(next_pieces[n] + 1) | A_BOLD);
                }
            }
        }
    }

    refresh();
}

Tetromino create_tetromino()
{
    Tetromino t;
    t.type = get_next_piece(); // Benutze Next-System
    t.x = WIDTH / 2 - 2;
    t.y = -1;
    t.rotation = 0;
    return t;
}

int main()
{
    srand(time(NULL));

    // ncurses initialisieren
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    init_colors();

    init_next_pieces(); // Next Pieces initialisieren

    Tetromino current = create_tetromino();

    clock_t last_fall = clock();
    int fall_speed = 500000; // 500ms

    while (!game_over)
    {
        // Input verarbeiten
        int ch = getch();

        if (ch != ERR)
        {
            Tetromino temp = current;

            if (ch == 'q' || ch == 'Q')
            {
                game_over = 1;
                continue;
            }
            else if (ch == KEY_LEFT || ch == 'a' || ch == 'A')
            {
                temp.x--;
                if (!check_collision(&temp))
                    current = temp;
            }
            else if (ch == KEY_RIGHT || ch == 'd' || ch == 'D')
            {
                temp.x++;
                if (!check_collision(&temp))
                    current = temp;
            }
            else if (ch == KEY_DOWN || ch == 's' || ch == 'S')
            {
                temp.y++;
                if (!check_collision(&temp))
                    current = temp;
            }
            else if (ch == KEY_UP || ch == 'w' || ch == 'W')
            {
                // Hard Drop - Stein fällt sofort runter
                while (!check_collision(&temp))
                {
                    current = temp;
                    temp.y++;
                }
                // Sofort mergen und neuen Stein
                merge_tetromino(&current);

                int cleared = clear_lines();
                if (cleared > 0)
                {
                    lines_cleared += cleared;
                    score += cleared * cleared * 100;
                    level = 1 + lines_cleared / 5;             // Level up alle 5 Linien (statt 10)
                    fall_speed = 500000 - (level - 1) * 50000; // Schnellere Steigerung
                    if (fall_speed < 50000)
                        fall_speed = 50000; // Schnelleres Minimum
                }

                current = create_tetromino();

                if (check_collision(&current))
                {
                    game_over = 1;
                }

                last_fall = clock(); // Timer zurücksetzen
                can_hold = 1;        // Nach Hard Drop kann wieder gehalten werden
            }
            else if (ch == 'e' || ch == 'E')
            {
                // Hold Funktion
                if (can_hold)
                {
                    if (hold_piece == -1)
                    {
                        // Erstes Mal halten - speichere aktuellen Stein
                        hold_piece = current.type;
                        current = create_tetromino();
                    }
                    else
                    {
                        // Tausche mit gehaltenem Stein
                        int temp_type = current.type;
                        current.type = hold_piece;
                        current.x = WIDTH / 2 - 2;
                        current.y = -1;
                        current.rotation = 0;
                        hold_piece = temp_type;

                        // Prüfe ob der getauschte Stein passt
                        if (check_collision(&current))
                        {
                            // Wenn nicht, Game Over
                            game_over = 1;
                        }
                    }
                    can_hold = 0; // Kann nur einmal pro Stein benutzt werden
                }
            }
            else if (ch == 'r' || ch == 'R')
            {
                temp.rotation++;
                if (!check_collision(&temp))
                    current = temp;
            }
        }

        // Automatisches Fallen
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
                    level = 1 + lines_cleared / 5;             // Level up alle 5 Linien (statt 10)
                    fall_speed = 500000 - (level - 1) * 50000; // Schnellere Steigerung
                    if (fall_speed < 50000)
                        fall_speed = 50000; // Schnelleres Minimum
                }

                current = create_tetromino();
                can_hold = 1; // Neuer Stein, kann wieder gehalten werden

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
        }

        draw_board(&current);
        usleep(10000); // 10ms
    }

    // Game Over Bildschirm
    clear();
    mvprintw(10, 10, "=== GAME OVER ===");
    mvprintw(12, 10, "Final Score: %d", score);
    mvprintw(13, 10, "Level: %d", level);
    mvprintw(14, 10, "Lines: %d", lines_cleared);
    mvprintw(16, 10, "Druecke eine Taste zum Beenden...");
    refresh();

    nodelay(stdscr, FALSE);
    getch();

    endwin();

    return 0;
}
