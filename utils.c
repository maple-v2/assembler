#include "globals.h"
/* MMN 14 Assembler utility functions and error handling implementation */
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define MAX_ERRORS 100

static char error_list[MAX_ERRORS][MAX_LINE_LENGTH];
static int error_count = 0;

Symbol symbol_table[100];
int symbol_count = 0;

void report_error(const char *msg) {
	if (error_count < MAX_ERRORS) {
		strncpy(error_list[error_count], msg, MAX_LINE_LENGTH - 1);
		error_list[error_count][MAX_LINE_LENGTH - 1] = '\0';
		error_count++;
	}
	fprintf(stderr, "Error: %s\n", msg);
}

int has_errors() {
	return error_count > 0;
}

void clear_errors() {
	error_count = 0;
}

int add_symbol(const char *symbol, int value, int attr) {
	if (find_symbol(symbol) != -1) {
		report_error("Symbol already exists");
		return 0;
	}
	if (symbol_count < 100) {
		strncpy(symbol_table[symbol_count].name, symbol, MAX_SYMBOL_LENGTH - 1);
		symbol_table[symbol_count].name[MAX_SYMBOL_LENGTH - 1] = '\0';
		symbol_table[symbol_count].value = value;
		symbol_table[symbol_count].attr = attr;
		symbol_count++;
		return 1;
	}
	report_error("Symbol table full");
	return 0;
}

int find_symbol(const char *symbol) {
    int i;
    for (i = 0; i < symbol_count; i++) {
		if (strcmp(symbol_table[i].name, symbol) == 0) {
			return i;
		}
	}
	return -1;
}

Symbol* get_symbol(const char *symbol) {
	int idx = find_symbol(symbol);
	if (idx != -1) {
		return &symbol_table[idx];
	}
	return NULL;
}
char *strdup(const char *s) {
	char *copy;
	size_t len = strlen(s) + 1;  

	copy = (char *)malloc(len);
	if (copy != NULL) {
		strcpy(copy, s);
	}
	return copy;
}


void build_new_file_name(char *str,  char *newExt)
{
	char *dot;
dot  = strchr(str, '.');  
	if (dot)
	{
		*dot = '\0';  
	}

	strcat(str, newExt);
}


short encode_ARE(short num) {
	num = num << 0;
	return num;
}
short encode_des_operand(short num) {
	num = num << 2;
	return num;
}

short encode_source_operand(short num) {
	num = num << 4;
	return num;
}
short encode_opcode(short num) {
	num = num << 6;
	return num;
}
short encode(short op, short source, short des, short ARE) {
	short num;
	ARE = encode_ARE(ARE);
	des = encode_des_operand(des);
	source = encode_source_operand(source);
	op = encode_opcode(op);
	num = ARE | des | source | op;
	return num;
}
void print_binary(unsigned short num) {
    short i;
    for (i = 15; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
	printf("\n");
}
void print_abcd_mem(unsigned short num)
{
	char str[6] = "";
	char symbol[] = { 'a','b','c','d' };
	int temp;
	temp = num;
	temp = temp & 3;
	str[0] = symbol[temp];
	temp = num;
	temp = temp & 12;
	temp = temp >> 2;
	str[1] = symbol[temp];
	temp = num;
	temp = temp & 48;
	temp = temp >> 4;
	str[2] = symbol[temp];
	temp = num;
	temp = temp & 192;
	temp = temp >> 6;
	str[3] = symbol[temp];
	temp = num;
	temp = temp & 768;
	temp = temp >> 8;
	str[4] = symbol[temp];

	str[5] = '\0';
	printf("Memory Address: %s\n", str);

}
void print_abcd_addy(unsigned short num)
{
    char symbol[] = { 'a','b','c','d' };
	int temp;
	temp = num;
	temp = temp & 3;
    /* unused local string removed */
	temp = num;
	temp = temp & 12;
	temp = temp >> 2;
    (void)symbol[temp];
	temp = num;
	temp = temp & 48;
	temp = temp >> 4;
    (void)symbol[temp];
	temp = num;
	temp = temp & 192;
	temp = temp >> 6;
    (void)symbol[temp];


}

