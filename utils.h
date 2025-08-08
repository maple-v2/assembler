/* MMN 14 Assembler utility functions and error handling */
#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

/* Error handling */
void report_error(const char *msg);
int has_errors();
void clear_errors();

/* Symbol table management */
int add_symbol(const char *symbol, int value, int attr);
int find_symbol(const char *symbol);
Symbol* get_symbol(const char *symbol);

#endif /* UTILS_H */
