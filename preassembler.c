#include "globals.h"




macro* make_macro(FILE *fp) {
    macro* head = NULL;
    char line[MAX_LINE_LEN];
    char data[MAX_MEMORY];
    char macroName[64];

    while (fgets(line, MAX_LINE_LEN, fp) != NULL) {
       
        if (strstr(line, "mcro") != NULL) {
            data[0] = '\0'; 

         
            if (sscanf(line, "%*s %63s", macroName) != 1) {
             
                continue;
            }

         
            while (fgets(line, MAX_LINE_LEN, fp) != NULL) {
             
                if (strncmp(line, "mcroend", 7) == 0) {
                    break;
                }
                strcat(data, line);
            }

            add_macro(&head, macroName, data);
        }
    }

    return head;
}
void add_macro(macro **head, const char *name, const char *data) {
    macro *new_node = (macro*)malloc(sizeof(macro));
    new_node->mc_name = strdup(name);
    new_node->mc_data = strdup(data);
    new_node->next = *head;
    *head = new_node;
}
char* find_macro_data(macro* head, const char* name) {
    while (head != NULL) {
        if (strcmp(head->mc_name, name) == 0) {
            return head->mc_data;
        }
        head = head->next;
    }
    return NULL;
}
void replace_macros_in_line(char* line,macro* macros, char* output) {
    char* token = strtok(line, " \t\n");
    int first = 1;
    output[0] = '\0';

    while (token != NULL) {
        char* replacement = find_macro_data(macros, token);
        if (!first) strcat(output, " ");
        first = 0;

        if (replacement) {
            strcat(output, replacement);
        } else {
            strcat(output, token);
        }

        token = strtok(NULL, " \t\n");
    }
    strcat(output, "\n");
}

void process_file(FILE* in, FILE* out, macro* macros) {
    char line[MAX_LINE_LEN];
    char new_line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), in)) {
        replace_macros_in_line(line, macros, new_line);
        fputs(new_line, out);
    }
}



