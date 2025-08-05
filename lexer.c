#include "globals.h"




opcode get_opcode_from_string(const char *s) {
    
    static const opcodeEntry table[] = {
        {"mov", mov}, {"cmp", cmp}, {"add", add}, {"sub", sub},
        {"not", not}, {"clr", clr}, {"lea", lea}, {"inc", inc},
        {"dec", dec}, {"jump", jump}, {"bne", bne}, {"red", red},
        {"prn", prn}, {"jsr", jsr}, {"rts", rts}, {"stop", stop}
    };

    int num_opcodes = sizeof(table) / sizeof(table[0]);

    for (int i = 0; i < num_opcodes; i++) {
        if (strcmp(s, table[i].name) == 0) {
            return table[i].op;  
        }
    }

    return INVALID_OPCODE;
}


address get_address_from_string(const char *s) {
    static const AddyEntry table[] = {
        {"immediate", immediateAddress},
        {"direct", directAddress},
        {"mat_access", matAccessAddress},
        {"direct_reg", directRegAddress}
    };

    int num_entries = sizeof(table) / sizeof(table[0]);

    for (int i = 0; i < num_entries; i++) {
        if (strcmp(s, table[i].name) == 0)
            return table[i].addy;
    }

    return INVALID_ADDRESS;/*use when checking for errors*/
}


AER get_aer_from_string(const char *s) {
    static const AEREntry table[] = {
        {"A", A},
        {"E", E},
        {"R", R}
    };

    int count = sizeof(table) / sizeof(table[0]);

    for (int i = 0; i < count; i++) {
        if (strcmp(s, table[i].name) == 0) {
            return table[i].value;
        }
    }

    return INVALID_AER;
}
char* addy_type(char* operand, symbol* head) {
    if (strncmp(operand, "#", 1) == 0) {
        return "immediate";
    }

    if (strcmp(operand, find_symbol(head, operand)) == 0) {
        return "direct";
    }
    char label[31], idx1[10], idx2[10];/*change to this numbers to meaningufll ones*/
    if (sscanf(operand, "%30[^[][%9[^]]][%9[^]]]", label, idx1, idx2) == 3) {
        return "mat_access";
    }


    if (strcmp(operand, "r0") == 0 || strcmp(operand, "r1") == 0 ||
        strcmp(operand, "r2") == 0 || strcmp(operand, "r3") == 0 ||
        strcmp(operand, "r4") == 0 || strcmp(operand, "r5") == 0 ||
        strcmp(operand, "r6") == 0 || strcmp(operand, "r7") == 0) {
        return "direct_reg";
    }

   

    return "invalid";
}

