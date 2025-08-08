/* MMN 14 Assembler – Global definitions (cleaned) */
#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* General limits */
#define MAX_LINE_LENGTH 80     /* For error list messages */
#define MAX_SYMBOL_LENGTH 31

/* Preassembler limits */
#define MAX_LINE_LEN 256
#define MAX_MEMORY 4096

/* Return codes */
#define OK 0
#define ERROR 1

/* Symbol attributes */
#define DATA_ATTR 1
#define CODE_ATTR 2
#define EXTERNAL_ATTR 3
#define ENTRY_ATTR 4

/* Symbol used by utils.c (fixed-size table) */
typedef struct {
    char name[MAX_SYMBOL_LENGTH];
    int value;
    int attr;
} Symbol;

extern Symbol symbol_table[];
extern int symbol_count;

/* Macro list used by preassembler */
typedef struct MacroNode {
    char *mc_name;
    char *mc_data;
    struct MacroNode *next;
} macro;

/* Optional linked-list symbol (not used by build, kept for compatibility) */
typedef struct SymNode {
    char *sym_name;
    int sym_value;
    struct SymNode *next;
} symbol;

/* Common helpers implemented in utils.c */
char *strdup(const char *s);
void build_new_file_name(char *str, char *newExt);

/* Preassembler API implemented in preassembler.c */
macro *make_macro(FILE *fp);
void add_macro(macro **head, const char *name, const char *data);
char *find_macro_data(macro *head, const char *name);
void replace_macros_in_line(char *line, macro *macros, char *output);
void process_file(FILE *in, FILE *out, macro *macros);

#endif /* GLOBALS_H */



