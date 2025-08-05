include "globals.h"

symbol* make_symbol(FILE *fp) {
    symbol* head = NULL;
    char line[MAX_LINE_LEN];
    char data[MAX_MEMORY];
    char symbolName[64];

  
       
        if (strstr(line, ":") != NULL) {
            data[0] = '\0'; 

         
           if (sscanf(line, "%[^:]:", symbolName) == 1) {
   strcat(data, symbolName);


         
           
            add_symbol(&head, symbolName, data);
}
        
    }

    return head;/* check if return numm we contnie in code*/
}
void add_symbol(symbol **head, const char *name,int value) {
    symbol *new_node = (symbol*)malloc(sizeof(symbol));
    new_node->sym_name = strdup(name);
    new_node->sym_value=value;

    new_node->next = *head;
    *head = new_node;
}
char* find_symbol(symbol* head, const char* name) {
    while (head != NULL) {
        if (strcmp(head->sym_name, name) == 0) {
            return head->sym_name;
        }
        head = head->next;
    }
    return NULL;
}


