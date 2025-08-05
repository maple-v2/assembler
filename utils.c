#include "globals.h"
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
	for (short i = 15; i >= 0; i--) {
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
	char str[5] = "";
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


	str[4] = '\0';


}

