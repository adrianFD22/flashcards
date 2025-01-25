
// A simple utility for displaying flashcards. A file is passed as an argument.
// Then randomly reads the line of the file and displays the correspondent
// flashcard. The user answers back with the guess and the program gives
// feedback on correct and incorrect answers. When it is over, the program
// displays the percentage of correct and wrong answers.

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>

//#include "config.h"

#define DEFAULT_SESSION_LEN 30      // Default number of words in session
#define SEP_TOKEN ","               // Separation token for reading csv
#define WAIT_TIME 700               // Time in miliseconds to show the correct message

#define STR_LEN 100
#define MIN(x,y) (x) < (y) ? (x) : (y)

int* gen_session_indices(int len, int num_lines);
char *trim_str(char *str);
void print_centered_x(int pos_y, int pos_x, char* str);
void getstr_centered(int center_y, int center_x, char *str);

// struct flashcard {
//     char front[STR_LEN];
//     char back[STR_LEN];
// };
typedef char flashcard[2][STR_LEN];

int main(int argc, char **argv)
{
    int front, back;
    char *file_name;
    FILE *file_ptr;
    int *session_indices;
    int num_lines;
    char c;
    int curr_index;

    char raw_str[2*STR_LEN];
    char curr_token[STR_LEN];
    char *token_trim;
    flashcard *flashcard_list;
    flashcard curr_flashcard;
    char user_input[STR_LEN];

    int score;

    int center_x;
    int center_y;

    int session_len = DEFAULT_SESSION_LEN;

    // -------------------------
    //      Parse arguments
    // -------------------------
    if (argc == 1 || argc > 4)
    {
        fprintf(stderr, "usage: flash [-r] csv_file [n_flashcards]\n");
        return 1;
    }

    // Check revert option
    if (strcmp(argv[1], "-r") == 0)
    {
        front = 1;
        back = 0;
        argv++;
        argc--;
    } else
    {
        front = 0;
        back = 1;
    }

    if (argc == 3)
    {
        session_len = atoi(argv[2]);
    }

    file_name = argv[1];
    //file_name = strcat(file_name, ".csv");
    //file_name = strcat(flashcards_path, file_name);

    // -------------------------
    //      Read flashcards
    // -------------------------
    // Check if file exists
    file_ptr = fopen(file_name, "r");

    if (file_ptr == NULL) {
        fprintf(stderr, "Invalid file\n");
        return 1;
    };

    // Get number of lines in file
    num_lines = 0;
    while (!feof(file_ptr)) {
        c = fgetc(file_ptr);
        if (c == '\n' ) num_lines++;
    }
    fseek(file_ptr, 0, SEEK_SET);

    session_len = MIN(num_lines, session_len);

    // Generate random order to show flashcards
    srand(time(NULL));
    session_indices = gen_session_indices(session_len, num_lines);
    flashcard_list = malloc(session_len * sizeof(flashcard));

    // Read flashcards from file
    for (int i = 0; i < session_len; i++)
    {
        curr_index = session_indices[i];

        // Read line in file
        fseek(file_ptr, 0, SEEK_SET);
        for (int line = 0; line < curr_index;)
        {
            c = fgetc(file_ptr);
            if (c == '\n') line++;
        }
        fgets(raw_str, STR_LEN, file_ptr);

        strcpy(curr_token, strtok(raw_str, SEP_TOKEN));
        token_trim = trim_str(curr_token);
        strcpy(flashcard_list[i][0], token_trim);

        strcpy(curr_token, strtok(NULL, SEP_TOKEN));
        token_trim = trim_str(curr_token);
        strcpy(flashcard_list[i][1], token_trim);
    }

    // --------------------
    //     Display game
    // --------------------
    score = 0;
    setlocale(LC_ALL, "");      // TODO: why this works?

    initscr();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);

    for (int i = 0; i < session_len; i++)
    {
        // Get terminal size
        center_y = getmaxy(stdscr) / 2;
        center_x = getmaxx(stdscr) / 2;

        // Print front
        clear();
        print_centered_x(center_y-1, center_x, flashcard_list[i][front]);

        // Get input
        getstr_centered(center_y+1, center_x, user_input);
        trim_str(user_input);

        clear();
        // Show correct or incorrect
        if (strcasecmp(user_input, flashcard_list[i][back]) == 0)
        {
            // Print correct
            print_centered_x(center_y, center_x, "Correct");
            score++;
            refresh();
            timeout(WAIT_TIME);
            getch();
            timeout(-1);

        } else
        {
            // Print incorrect
            print_centered_x(center_y-1, center_x, flashcard_list[i][front]);
            print_centered_x(center_y+1, center_x, flashcard_list[i][back]);
            getch();
        }
    }

    // Print score
    center_x = getmaxx(stdscr) / 2;
    center_y = getmaxy(stdscr) / 2;
    clear();
    mvprintw(center_y, center_x-6, "Score: %d/%d", score, session_len);
    getch();
    endwin();
}

// Generates a list of random distinct integers using
// reservoir shuffle
int *gen_session_indices(int len, int num_lines)
{
    int *session_indices, *available_indices;
    int rand_index;

    session_indices = (int*) malloc(len * sizeof(int));

    available_indices = (int*) malloc(num_lines * sizeof(int));

    for (int i=0; i < num_lines; i++) {
        available_indices[i] = i;
    }

    for (int i=0; i < len; i++)
    {
        rand_index = rand() % (num_lines - i);

        session_indices[i] = available_indices[rand_index];
        available_indices[rand_index] = available_indices[num_lines - i - 1];
    }

    return session_indices;
}

// Trim whitespaces and tabulations and remove
// trailing linebreak in str.
char *trim_str(char *str)
{
    char *end_str;

    // Trim spaces at beggining
    while (*str == ' ' || *str == '\t') str++;

    // Trim trailing linebreak
    str[strcspn(str, "\n")] = 0;

    // Trim spaces at end
    end_str = str + strlen(str) - 1;
    while (*end_str== ' ' || *end_str == '\t')
    {
        *end_str = '\0';
        end_str--;
    }

    return str;
}

// Print given string str in coordinates center_y
// and center_x - strlen(str), i.e., centered horizontally
// at height center_y
void print_centered_x(int center_y, int center_x, char *str)
{
    int str_len;

    str_len = strlen(str);
    mvprintw(center_y, center_x - str_len/2, "%s", str);
}

// Get user input, store it in str and display it centered
void getstr_centered(int center_y, int center_x, char *str)
{
    int len;
    wchar_t c;

    move(center_y,0);

    len = 0;
    while ( (c = wgetch(stdscr)) != '\n')
    {
        if (c == KEY_BACKSPACE)
        {
            if (len > 0)
            {
                if (len > 1 && str[len-1] < 0) {    // If char to delete is extended ascii (and therefore 2 bytes), remove two
                    len--;
                }

                len--;
                *(str+len) = '\0';
            }
        }
        else
        {
            *(str+len) = c;
            *(str+len+1) = '\0';
            len++;
        }

        // Print word centered
        move(center_y,0);
        clrtoeol();
        mvprintw(center_y, center_x - len/2, "%s", str);
        refresh();
    }
};
