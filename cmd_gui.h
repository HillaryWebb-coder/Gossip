#ifndef __CMD_GUI_
#define __CMD_GUI_

#include <pthread.h>

extern pthread_mutex_t ui_mutex;

void init_ui();
void print_message(const char *prefix, const char *msg);
void read_input(char *buf, int maxlen);
void close_ui(void);

#endif