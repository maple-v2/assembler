#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_MEMORY 255 
#define MAX_LINE_LEN 81
#define ERROR 1
typedef enum {
r0=0,r1,r2,r3,r4,r5,r6,r7
}reg;
typedef enum {A,E,R,INVALID_AER }AER;
typedef enum {mov,cmp,add,sub,not,clr,lea,inc,dec,jump,bne,red,prn,jsr,rts,stop,INVALID_OPCODE}opcode;
typedef enum {
immediateAddress,directAddress,matAccessAddress,directRegAddress,INVALID_ADDRESS

}address;
typedef struct {
    const char *name;
    opcode op;
} opcodeEntry;
typedef struct {
    const char *name;
    address addy;
} addyEntry;
typedef struct {
    const char *name;
    AER aer;
} aerEntry;

/*for macro*/
typedef struct Node{
char* mc_name;
char *mc_data;
struct Node* next;
}macro;
 
typedef struct symNode{
char* sym_name ;
int sym_value;
struct symNode* next;
}symbol;

char *strdup(const char *s);
void add_macro(macro **head, const char *name, const char *data);
char* find_macro_data(macro* head, const char* name);
macro *make_macro(FILE *fp);
void replace_macros_in_line(char* line, macro* macros, char* output);
void process_file(FILE* in, FILE* out, macro* macros);
 void build_new_file_name(char *str,. char *newExt);



