#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

struct termios orig_termios;

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
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
    enable_raw_mode();

    printf("Tasten-Test Programm\n");
    printf("====================\n");
    printf("Drücke Tasten um zu sehen was erkannt wird.\n");
    printf("Drücke 'q' zum Beenden.\n\n");

    while (1)
    {
        char c = getchar();

        if (c == EOF)
        {
            continue;
        }

        if (c == 'q' || c == 'Q')
        {
            printf("\nBeende...\n");
            break;
        }

        // ASCII Code anzeigen
        printf("Taste gedrückt: ");

        if (c == 27)
        {
            printf("ESC (27) - ");

            // Nächste Zeichen lesen für Pfeiltasten
            char c2 = getchar();
            if (c2 == '[')
            {
                printf("[ ");
                char c3 = getchar();
                printf("%c ", c3);

                if (c3 == 'A')
                    printf("-> PFEIL HOCH");
                else if (c3 == 'B')
                    printf("-> PFEIL RUNTER");
                else if (c3 == 'C')
                    printf("-> PFEIL RECHTS");
                else if (c3 == 'D')
                    printf("-> PFEIL LINKS");
            }
        }
        else if (c >= 32 && c <= 126)
        {
            printf("'%c' (ASCII %d)", c, c);

            if (c == 'w' || c == 'W')
                printf(" -> ROTIEREN");
            if (c == 'a' || c == 'A')
                printf(" -> LINKS");
            if (c == 's' || c == 'S')
                printf(" -> RUNTER");
            if (c == 'd' || c == 'D')
                printf(" -> RECHTS");
        }
        else
        {
            printf("Steuerzeichen (ASCII %d)", c);
        }

        printf("\n");
        fflush(stdout);
    }

    disable_raw_mode();
    return 0;
}
