#include <ncurses.h>
#include <time.h>

#include "cmd_gui.h"

pthread_mutex_t ui_mutex = PTHREAD_MUTEX_INITIALIZER;

WINDOW *msg_win;
WINDOW *input_win;

void init_ui(){

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    msg_win = newwin(rows - 3, cols, 0, 0);
    input_win = newwin(3, cols, rows - 3, 0);

    scrollok(msg_win, TRUE);

    char space = ' ';
    box(input_win, (int)space, 0);
    wrefresh(msg_win);
    wrefresh(input_win);
}

void print_message(const char *prefix, const char *msg){
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);

    wprintw(msg_win, "%02d: %02d %s %s\n", local_time->tm_hour, local_time->tm_min, prefix, msg);
    wrefresh(msg_win);
}

void read_input(char *buf, int maxlen){
    wmove(input_win, 1, 2);
    wclrtoeol(input_win);
    mvwprintw(input_win, 1, 1, "> ");
    wrefresh(input_win);
    echo();
    wgetnstr(input_win, buf, maxlen);
    noecho();
}

void close_ui(void){
    endwin();
}