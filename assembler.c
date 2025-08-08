/* Full assembler driver: preassembler (.am) + first pass + second pass + outputs */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"

/* Local module headers */
/* These modules are added in this refactor */
/* symtab, pass1, pass2, output, asm_utils are provided below */

/* Symbol table */
typedef struct Sym {
    char name[MAX_SYMBOL_LENGTH];
    int value;            /* absolute address */
    unsigned attrs;       /* bitmask: 1=data, 2=code, 4=extern, 8=entry */
    struct Sym *next;
} Sym;

enum { ATTR_DATA = 1u, ATTR_CODE = 2u, ATTR_EXTERN = 4u, ATTR_ENTRY = 8u };

typedef struct ExtRef {
    char name[MAX_SYMBOL_LENGTH];
    int address;                 /* absolute address of the word using the extern */
    struct ExtRef *next;
} ExtRef;

typedef struct {
    /* images */
    unsigned short code[4096];   /* 10-bit words stored in 16-bit */
    unsigned short data[4096];
    int ic; /* number of code words */
    int dc; /* number of data words */
    /* tables */
    Sym *symbols;
    ExtRef *extrefs;
    /* error state */
    int error_count;
} AsmState;

/* ---- helpers (decls) ---- */
static void state_init(AsmState *st);
static void state_free(AsmState *st);
static void sym_add(AsmState *st, const char *name, int value, unsigned attrs, int line);
static Sym *sym_get(Sym *head, const char *name);
static void sym_mark_entry(AsmState *st, const char *name, int line);
static void sym_add_extern(AsmState *st, const char *name, int line);
static void sym_adjust_data(AsmState *st, int add);
static void ext_add(AsmState *st, const char *name, int address);

/* passes */
static int first_pass(const char *am_path, AsmState *st);
static int second_pass(const char *am_path, AsmState *st);

/* output */
static int write_outputs(const char *base, const AsmState *st);

/* preassembler already provided (globals.h decls) */

/* string helpers */
static void trim(char *s);
static int is_blank_or_comment(const char *s);
static int is_label_token(const char *tok);
static int is_register_name(const char *s);
static int parse_int10(const char *s, int *out);
static int starts_with(const char *s, const char *pfx);
static char* find_first_quote(char *s);
static char* find_last_quote(char *s);
static int quote_len_at(const unsigned char *p);

/* opcode and addressing */
typedef enum { OP_MOV=0, OP_CMP, OP_ADD, OP_SUB, OP_NOT, OP_CLR, OP_LEA, OP_INC, OP_DEC, OP_JMP, OP_BNE, OP_RED, OP_PRN, OP_JSR, OP_RTS, OP_STOP, OP_INVALID=99 } OpCode;
typedef enum { ADDR_IMMEDIATE=0, ADDR_DIRECT=1, ADDR_MATRIX=2, ADDR_REGISTER=3, ADDR_INVALID=99 } AddrMode;
static OpCode opcode_from_str(const char *s);
static AddrMode addrmode_from_operand(const char *op);

/* encoder */
static unsigned short make_word10(unsigned short value);              /* mask to 10 bits */
static unsigned short word_first(OpCode op, AddrMode src, AddrMode dst); /* ARE=00 */
static unsigned short word_immediate(int imm);                        /* ARE=00 + 8-bit value */
static unsigned short word_label(int address, int is_extern);         /* ARE: extern=01, reloc=10 */
static unsigned short word_regs(int src_reg, int dst_reg);            /* shared reg word with ARE=00 */

/* base-4 unique encoding */
static void to_base4a(unsigned short w10, char out[6]); /* 5 chars + NUL using a,b,c,d */
static void to_base4a_addr(int addr, char out[16]);     /* up to 16 for safety */

/* ---- main driver ---- */
int main(int argc, char *argv[]) {
    int i;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input1> [input2 ...] (omit .as)\n", argv[0]);
        return ERROR;
    }

    for (i = 1; i < argc; i++) {
        char as_name[512];
        char am_name[512];
        char base_name[512];
        FILE *in = NULL, *am = NULL;
        AsmState st;
        size_t blen;

        /* build names */
        sprintf(base_name, "%s", argv[i]);
        blen = strlen(base_name);
        if (blen + 3 >= sizeof(as_name) || blen + 3 >= sizeof(am_name)) {
            fprintf(stderr, "Error: base name too long: %s\n", base_name);
            continue;
        }
        as_name[0] = '\0'; am_name[0] = '\0';
        strcpy(as_name, base_name); strcat(as_name, ".as");
        strcpy(am_name, base_name); strcat(am_name, ".am");

        in = fopen(as_name, "r");
        if (!in) {
            fprintf(stderr, "Error: cannot open %s\n", as_name);
            continue;
        }
        am = fopen(am_name, "w");
        if (!am) {
            fprintf(stderr, "Error: cannot create %s\n", am_name);
            fclose(in);
            continue;
        }

        /* preassemble: expand macros to .am */
        {
            macro *macros = make_macro(in);
            rewind(in);
            process_file(in, am, macros);
        }
        fclose(in);
        fclose(am);

        /* two passes */
        state_init(&st);
        if (!first_pass(am_name, &st)) {
            fprintf(stderr, "Errors in first pass. Skipping %s\n", base_name);
            state_free(&st);
            continue;
        }
        /* adjust DATA symbols by ICF + 100 */
        sym_adjust_data(&st, st.ic + 100);
        if (!second_pass(am_name, &st)) {
            fprintf(stderr, "Errors in second pass. Skipping %s\n", base_name);
            state_free(&st);
            continue;
        }
        if (!write_outputs(base_name, &st)) {
            fprintf(stderr, "Failed writing outputs for %s\n", base_name);
            state_free(&st);
            continue;
        }
        state_free(&st);
    }
    return OK;
}

/* ================= Implementation (minimal, compliant with homework.txt) ================ */

static void state_init(AsmState *st) {
    memset(st, 0, sizeof(*st));
}
static void state_free(AsmState *st) {
    Sym *s;
    ExtRef *e;
    s = st->symbols;
    while (s) { Sym *n = s->next; free(s); s = n; }
    e = st->extrefs;
    while (e) { ExtRef *n = e->next; free(e); e = n; }
}

static Sym *sym_get(Sym *head, const char *name) {
    for (; head; head = head->next) if (strcmp(head->name, name) == 0) return head;
    return NULL;
}
static void sym_add(AsmState *st, const char *name, int value, unsigned attrs, int line) {
    Sym *s;
    if (sym_get(st->symbols, name)) {
        fprintf(stderr, "[%d] error: duplicate symbol '%s'\n", line, name);
        st->error_count++;
        return;
    }
    s = (Sym*)calloc(1, sizeof(Sym));
    strncpy(s->name, name, MAX_SYMBOL_LENGTH-1);
    s->value = value;
    s->attrs = attrs;
    s->next = st->symbols;
    st->symbols = s;
}
static void sym_mark_entry(AsmState *st, const char *name, int line) {
    Sym *s = sym_get(st->symbols, name);
    if (!s) {
        fprintf(stderr, "[%d] error: .entry refers to undefined symbol '%s'\n", line, name);
        st->error_count++;
        return;
    }
    s->attrs |= ATTR_ENTRY;
}
static void sym_add_extern(AsmState *st, const char *name, int line) {
    if (sym_get(st->symbols, name)) {
        fprintf(stderr, "[%d] error: symbol '%s' already defined; cannot mark extern\n", line, name);
        st->error_count++;
        return;
    }
    sym_add(st, name, 0, ATTR_EXTERN, line);
}
static void sym_adjust_data(AsmState *st, int add) {
    Sym *s;
    for (s = st->symbols; s; s = s->next) {
        if (s->attrs & ATTR_DATA) s->value += add;
    }
}
static void ext_add(AsmState *st, const char *name, int address) {
    ExtRef *e = (ExtRef*)calloc(1, sizeof(ExtRef));
    strncpy(e->name, name, MAX_SYMBOL_LENGTH-1);
    e->address = address;
    e->next = st->extrefs;
    st->extrefs = e;
}

/* Token helpers */
static void trim(char *s) {
    size_t n = strlen(s);
    size_t i = 0;
    while (n && (s[n-1]=='\r' || s[n-1]=='\n' || s[n-1]==' ' || s[n-1]=='\t')) s[--n]=0;
    while (s[i]==' '||s[i]=='\t') i++;
    if (i) memmove(s, s+i, strlen(s+i)+1);
}
static int is_blank_or_comment(const char *s) { while (*s==' '||*s=='\t') s++; return (*s==0)||(*s==';'); }
static int is_label_token(const char *tok) { size_t n=strlen(tok); return n>1 && tok[n-1]==':'; }
static int is_register_name(const char *s) { return strlen(s)==2 && s[0]=='r' && s[1]>='0' && s[1]<='7'; }
static int parse_int10(const char *s, int *out) {
    char *end=NULL; long v = strtol(s, &end, 10); if (*s==0 || *end!=0) return 0; *out=(int)v; return 1;
}
static int starts_with(const char *s, const char *pfx) {
    size_t n = strlen(pfx);
    return strncmp(s, pfx, n) == 0;
}
static char* find_first_quote(char *s) {
    for (; *s; s++) {
        if (quote_len_at((const unsigned char*)s)) return s;
    }
    return NULL;
}
static char* find_last_quote(char *s) {
    char *last = NULL;
    for (; *s; s++) {
        if (quote_len_at((const unsigned char*)s)) last = s;
    }
    return last;
}
static int quote_len_at(const unsigned char *p) {
    /* ASCII '"' */
    if (p[0] == '"') return 1;
    /* Windows-1252 smart quotes */
    if (p[0] == 0x93 || p[0] == 0x94) return 1;
    /* UTF-8 smart quotes U+201C/U+201D: E2 80 9C / E2 80 9D */
    if (p[0] == 0xE2 && p[1] == 0x80 && (p[2] == 0x9C || p[2] == 0x9D)) return 3;
    return 0;
}

/* opcodes */
static OpCode opcode_from_str(const char *s) {
    int i;
    static const char *names[] = {"mov","cmp","add","sub","not","clr","lea","inc","dec","jmp","bne","red","prn","jsr","rts","stop"};
    for (i=0;i<16;i++) {
        if (strcmp(s,names[i])==0) return (OpCode)i;
    }
    return OP_INVALID;
}

static AddrMode addrmode_from_operand(const char *op) {
    if (op[0]=='#') {
        int v; return parse_int10(op+1,&v)?ADDR_IMMEDIATE:ADDR_INVALID;
    }
    if (is_register_name(op)) return ADDR_REGISTER;
    if (strchr(op,'[')) return ADDR_MATRIX; /* simplistic detection */
    /* label */
    return ADDR_DIRECT;
}

/* encoding helpers (10-bit) */
static unsigned short make_word10(unsigned short value) { return (unsigned short)(value & 0x03FFu); }
static unsigned short word_first(OpCode op, AddrMode src, AddrMode dst) {
    unsigned short v = 0;
    v |= ((unsigned short)op & 0x0Fu) << 6;     /* bits 6-9 */
    v |= ((unsigned short)src & 0x03u) << 4;    /* bits 4-5 */
    v |= ((unsigned short)dst & 0x03u) << 2;    /* bits 2-3 */
    v |= 0u;                                    /* ARE = 00 (Absolute) */
    return make_word10(v);
}
static unsigned short word_immediate(int imm) {
    unsigned short v = ((unsigned short)(imm) & 0x00FFu) << 2; /* 8-bit value, ARE=00 */
    return make_word10(v);
}
static unsigned short word_label(int address, int is_extern) {
    unsigned short are = is_extern ? 0x0001u : 0x0002u; /* 01=E, 10=R */
    unsigned short v = ((unsigned short)(address) & 0x00FFu) << 2; /* low 8 bits of address */
    v |= are;
    return make_word10(v);
}
static unsigned short word_regs(int src_reg, int dst_reg) {
    unsigned short v = 0;
    if (dst_reg>=0) v |= ((unsigned short)dst_reg & 0x0Fu) << 2;  /* bits 2-5 */
    if (src_reg>=0) v |= ((unsigned short)src_reg & 0x0Fu) << 6;  /* bits 6-9 */
    return make_word10(v);
}

static void to_base4a(unsigned short w10, char out[6]) {
    /* 10 bits -> 5 base-4 digits (0..3). map 0,1,2,3 -> a,b,c,d */
    int i;
    const char map[4] = {'a','b','c','d'};
    for (i=4;i>=0;i--) { out[i] = map[w10 & 0x3u]; w10 >>= 2; }
    out[5] = '\0';
}
static void to_base4a_addr(int addr, char out[16]) {
    const char map[4] = {'a','b','c','d'};
    char tmp[16];
    int i=0;
    int j=0;
    if (addr==0) { out[0]='a'; out[1]='\0'; return; }
    while (addr>0 && i<((int)sizeof(tmp))-1) { tmp[i++] = map[addr & 0x3]; addr >>= 2; }
    while (i>0) { out[j++]=tmp[--i]; }
    out[j]='\0';
}

/* First pass: build symbol table, encode data/.string/.mat and count code length */
static int first_pass(const char *am_path, AsmState *st) {
    FILE *fp = fopen(am_path, "r");
    if (!fp) { fprintf(stderr, "Error: cannot open %s\n", am_path); return 0; }
    {
        char linebuf[1024];
        int line=0;
        for (; fgets(linebuf,sizeof(linebuf),fp); ) {
            char work[1024];
            char *tok;
            char label_name[64]="";
            int has_label=0;
            line++;
            trim(linebuf);
            if (is_blank_or_comment(linebuf)) continue;
            strcpy(work, linebuf);
            tok = strtok(work, " \t"); if (!tok) continue;
            if (is_label_token(tok)) { has_label=1; tok[strlen(tok)-1]=0; strncpy(label_name,tok,63); tok = strtok(NULL, " \t"); if (!tok) { fprintf(stderr,"[%d] error: label without statement\n", line); st->error_count++; continue; } }
        if (tok[0]=='.') {
            if (strcmp(tok, ".extern")==0 || starts_with(tok, ".extern")) {
                char *name;
                if (strlen(tok) > 7) name = (char*)tok + 7; else name = strtok(NULL, " \t");
                while (name && (*name==' '||*name=='\t')) name++;
                if (!name || *name=='\0'){fprintf(stderr,"[%d] error: .extern missing name\n",line); st->error_count++; continue;}
                sym_add_extern(st, name, line);
            } else if (strcmp(tok, ".entry")==0 || starts_with(tok, ".entry")) {
                /* Defer marking to pass2; accept attached form .entryLABEL too */
                /* No-op here */
            } else if (strcmp(tok, ".data")==0 || starts_with(tok, ".data")) {
                char *p;
                char *q;
                char *endptr;
                char save;
                int val;
                if (has_label) sym_add(st, label_name, st->dc, ATTR_DATA, line);
                if (strlen(tok) > 5) p = (char*)tok + 5; else p = strtok(NULL, "");
                if (!p){fprintf(stderr,"[%d] error: .data needs numbers\n",line); st->error_count++; continue;}
                /* parse comma separated numbers */
                q=p;
                while (q && *q) {
                    while (*q==' '||*q=='\t'||*q==',') q++;
                    if (!*q) break;
                    endptr=q;
                    while (*endptr && *endptr!=',') endptr++;
                    save=*endptr; *endptr='\0';
                    if (!parse_int10(q,&val)) { fprintf(stderr,"[%d] error: invalid number in .data\n", line); st->error_count++; }
                    else { st->data[st->dc++] = make_word10(((unsigned short)val)&0x03FFu); }
                    *endptr=save; q = endptr;
                }
            } else if (strcmp(tok, ".string")==0 || starts_with(tok, ".string")) {
                char *rest;
                char *start;
                char *endq;
                unsigned char *pp;
                int open_len;
                int close_len;
                /* include any text after .string including spaces */
                if (strlen(tok) > 7) rest = (char*)tok + 7; else rest = strtok(NULL, "");
                if (!rest){fprintf(stderr,"[%d] error: .string needs string\n",line); st->error_count++; continue;}
                /* find quotes in the original line (accept ASCII and Windows smart quotes) */
                start = find_first_quote(linebuf);
                endq = NULL;
                if (start) endq = find_last_quote(start+1);
                if (!start||!endq||endq<=start+1) { fprintf(stderr,"[%d] error: invalid .string\n", line); st->error_count++; continue; }
                open_len = quote_len_at((const unsigned char*)start);
                close_len = quote_len_at((const unsigned char*)endq);
                if (open_len == 0 || close_len == 0) { fprintf(stderr,"[%d] error: invalid .string\n", line); st->error_count++; continue; }
                if (has_label) sym_add(st, label_name, st->dc, ATTR_DATA, line);
                for (pp=(unsigned char*)start+open_len; (char*)pp<endq; ++pp) st->data[st->dc++] = make_word10((*pp) & 0x03FFu);
                st->data[st->dc++] = 0; /* NUL */
            } else if (strcmp(tok, ".mat")==0 || starts_with(tok, ".mat")) {
                /* Minimal: allocate rows*cols cells (zero-init), optionally parse init list */
                char *rest;
                int rows=0, cols=0;
                int total;
                int filled=0;
                if (has_label) sym_add(st, label_name, st->dc, ATTR_DATA, line);
                if (strlen(tok) > 4) rest = (char*)tok + 4; else rest = strtok(NULL, "");
                while (rest && (*rest==' '||*rest=='\t')) rest++;
                if (!rest){fprintf(stderr,"[%d] error: .mat requires dims\n",line); st->error_count++; continue;}
                if (sscanf(rest, "[%d][%d]", &rows, &cols)!=2 || rows<=0 || cols<=0){ fprintf(stderr,"[%d] error: .mat dims\n", line); st->error_count++; continue; }
                total = rows*cols;
                if (strchr(rest, ',')) {
                    char *list = strchr(rest, ',');
                    char *q2;
                    list++;
                    q2=list; while (q2 && *q2 && filled<total) { while (*q2==' '||*q2=='\t'||*q2==',') q2++; if (!*q2) break; { char *e=q2; char sv; int v2; while (*e && *e!=',') e++; sv=*e; *e='\0'; if (parse_int10(q2,&v2)){ st->data[st->dc++] = make_word10(((unsigned short)v2)&0x03FFu); filled++; } else { fprintf(stderr,"[%d] error: invalid .mat init\n", line); st->error_count++; } *e=sv; q2=e; }
                    }
                }
                while (filled++ < total) st->data[st->dc++] = 0;
            } else {
                fprintf(stderr, "[%d] error: unknown directive '%s'\n", line, tok);
                st->error_count++;
            }
        } else {
            /* instruction */
            OpCode op = opcode_from_str(tok);
            if (op==OP_INVALID) { fprintf(stderr, "[%d] error: unknown opcode '%s'\n", line, tok); st->error_count++; continue; }
            if (has_label) sym_add(st, label_name, 100 + st->ic, ATTR_CODE, line);
            /* parse operands */
            {
                char *rest;
                char *comma;
                char op1[64]="";
                char op2[64]="";
                int operands;
                AddrMode src, dst;
                int L;
                rest = strtok(NULL, ""); if (!rest) rest = "";
                comma = strchr(rest, ',');
            if (comma) {
                /* two operands */
                *comma='\0'; trim(rest); trim(comma+1); strncpy(op1, rest, 63); strncpy(op2, comma+1, 63);
            } else { trim(rest); if (*rest) strncpy(op1, rest, 63); }
                operands = 0; if (*op1) operands++; if (*op2) operands++;
            /* determine addressing */
                src = ADDR_INVALID; dst = ADDR_INVALID;
                if (operands==2) { src = addrmode_from_operand(op1); dst = addrmode_from_operand(op2); }
                else if (operands==1) { src = 0; dst = addrmode_from_operand(op1); }
                else { src=0; dst=0; }
            /* instruction length counting (approx, without matrix full detail):
               base word=1; each immediate/direct adds +1; registers may share +1 if both regs. */
                L = 1;
                if (operands==2) {
                    /* compute instruction length */
                    if (src==ADDR_IMMEDIATE || src==ADDR_DIRECT) L++;
                    if (dst==ADDR_IMMEDIATE || dst==ADDR_DIRECT) L++;
                    if (src==ADDR_REGISTER && dst==ADDR_REGISTER) L+=1;
                    else if (src==ADDR_REGISTER) L+=1;
                    else if (dst==ADDR_REGISTER) L+=1;
                    if (src==ADDR_MATRIX) L+=2;
                    if (dst==ADDR_MATRIX) L+=2; /* minimal allocation */
                } else if (operands==1) {
                    if (dst==ADDR_IMMEDIATE || dst==ADDR_DIRECT) L++;
                    else if (dst==ADDR_REGISTER) L++;
                    else if (dst==ADDR_MATRIX) L+=2;
                }
                st->ic += L;
            }
        }
    }
    }
    fclose(fp);
    return st->error_count==0;
}

/* Second pass: encode instructions fully and emit ext ref log */
static int second_pass(const char *am_path, AsmState *st) {
    FILE *fp = fopen(am_path, "r");
    if (!fp) { fprintf(stderr, "Error: cannot open %s\n", am_path); return 0; }
    {
        char linebuf[1024]; int line=0; int ic=0; /* ic counts words */
        for (; fgets(linebuf,sizeof(linebuf),fp); ) {
            char work[1024];
            char *tok;
            char *rest;
            char *comma;
            char op1[64]="";
            char op2[64]="";
            int operands;
            AddrMode src, dst;
            OpCode op;
            line++; trim(linebuf); if (is_blank_or_comment(linebuf)) continue;
            strcpy(work, linebuf);
            tok = strtok(work, " \t"); if (!tok) continue;
            if (is_label_token(tok)) { tok = strtok(NULL, " \t"); if (!tok) continue; }
            if (tok[0]=='.') {
                if (strcmp(tok, ".entry")==0) { char *name=strtok(NULL, " \t"); if (name) sym_mark_entry(st, name, line); }
                continue; /* others already handled in pass1 */
            }
        /* instruction */
            op = opcode_from_str(tok);
            rest = strtok(NULL, ""); if (!rest) rest = "";
            comma = strchr(rest, ',');
            if (comma) { *comma='\0'; trim(rest); trim(comma+1); strncpy(op1, rest, 63); strncpy(op2, comma+1, 63); }
            else { trim(rest); if (*rest) strncpy(op1, rest, 63); }
            operands = 0; if (*op1) operands++; if (*op2) operands++;
            src = ADDR_INVALID; dst = ADDR_INVALID;
            if (operands==2) { src = addrmode_from_operand(op1); dst = addrmode_from_operand(op2); }
            else if (operands==1) { src = 0; dst = addrmode_from_operand(op1); }
            else { src=0; dst=0; }
        st->code[ic++] = word_first(op, src, dst);
        /* encode operands */
        if (operands==2) {
            /* source */
            if (src==ADDR_IMMEDIATE) {
                int v; parse_int10(op1+1,&v); st->code[ic++] = word_immediate(v);
            } else if (src==ADDR_DIRECT) {
                Sym *s = sym_get(st->symbols, op1);
                if (!s) { fprintf(stderr,"[%d] error: undefined symbol '%s'\n", line, op1); st->error_count++; s = NULL; }
                {
                    int ext = (s && (s->attrs & ATTR_EXTERN)) ? 1 : 0;
                st->code[ic++] = word_label(s? s->value : 0, ext);
                    if (ext) ext_add(st, op1, 100 + ic - 1);
                }
            } else if (src==ADDR_REGISTER) {
                int sr = op1[1]-'0'; st->code[ic++] = word_regs(sr, -1);
            } else if (src==ADDR_MATRIX) {
                /* Minimal: treat like direct label + regs word (two regs rX][rY]) */
                char label[64]; int rA=-1,rB=-1; Sym *s; int ext;
                sscanf(op1, "%63[^[][%*1sr%d][%*1sr%d]", label, &rA, &rB);
                s = sym_get(st->symbols, label);
                ext = (s && (s->attrs & ATTR_EXTERN)) ? 1 : 0;
                st->code[ic++] = word_label(s? s->value : 0, ext);
                if (ext) ext_add(st, label, 100 + ic - 1);
                st->code[ic++] = word_regs(rA, rB);
            }
            /* destination */
            if (dst==ADDR_IMMEDIATE) {
                int v; parse_int10(op2+1,&v); st->code[ic++] = word_immediate(v);
            } else if (dst==ADDR_DIRECT) {
                Sym *s = sym_get(st->symbols, op2);
                if (!s) { fprintf(stderr,"[%d] error: undefined symbol '%s'\n", line, op2); st->error_count++; s = NULL; }
                {
                    int ext = (s && (s->attrs & ATTR_EXTERN)) ? 1 : 0;
                    st->code[ic++] = word_label(s? s->value : 0, ext);
                    if (ext) ext_add(st, op2, 100 + ic - 1);
                }
            } else if (dst==ADDR_REGISTER) {
                int dr = op2[1]-'0';
                /* if previous word was regs-only for src, merge: overwrite with combined */
                if (src==ADDR_REGISTER) { ic--; st->code[ic++] = word_regs(op1[1]-'0', dr); }
                else { st->code[ic++] = word_regs(-1, dr); }
            } else if (dst==ADDR_MATRIX) {
                char label[64]; int rA=-1,rB=-1; Sym *s; int ext;
                sscanf(op2, "%63[^[][%*1sr%d][%*1sr%d]", label, &rA, &rB);
                s = sym_get(st->symbols, label);
                ext = (s && (s->attrs & ATTR_EXTERN)) ? 1 : 0;
                st->code[ic++] = word_label(s? s->value : 0, ext);
                if (ext) ext_add(st, label, 100 + ic - 1);
                st->code[ic++] = word_regs(rA, rB);
            }
        } else if (operands==1) {
            if (dst==ADDR_IMMEDIATE) { int v; parse_int10(op1+1,&v); st->code[ic++] = word_immediate(v); }
            else if (dst==ADDR_DIRECT) {
                Sym *s = sym_get(st->symbols, op1);
                if (!s) { fprintf(stderr,"[%d] error: undefined symbol '%s'\n", line, op1); st->error_count++; s=NULL; }
                {
                    int ext = (s && (s->attrs & ATTR_EXTERN)) ? 1 : 0;
                    st->code[ic++] = word_label(s? s->value : 0, ext);
                    if (ext) ext_add(st, op1, 100 + ic - 1);
                }
            } else if (dst==ADDR_REGISTER) { int dr=op1[1]-'0'; st->code[ic++] = word_regs(-1, dr); }
            else if (dst==ADDR_MATRIX) {
                char label[64]; int rA=-1,rB=-1; Sym *s; int ext;
                sscanf(op1, "%63[^[][%*1sr%d][%*1sr%d]", label, &rA, &rB);
                s = sym_get(st->symbols, label);
                ext = (s && (s->attrs & ATTR_EXTERN)) ? 1 : 0;
                st->code[ic++] = word_label(s? s->value : 0, ext);
                if (ext) ext_add(st, label, 100 + ic - 1);
                st->code[ic++] = word_regs(rA, rB);
            }
        }
    }
    }
    fclose(fp);
    /* append data after code into final code image for output stage */
    /* Here we keep separate arrays and let writer print code then data */
    return st->error_count==0;
}

static int write_outputs(const char *base, const AsmState *st) {
    /* .ob */
    FILE *fob;
    char b_ic[16], b_dc[16];
    char ob[512], ent[512], ext[512];
    sprintf(ob, "%s.ob", base);
    sprintf(ent, "%s.ent", base);
    sprintf(ext, "%s.ext", base);
  fob = fopen(ob, "w");
  
    if (!fob) { fprintf(stderr,"Error: cannot create %s\n", ob); return 0; }
    /* header: lengths in base-4 unique */
    to_base4a_addr(st->ic, b_ic); to_base4a_addr(st->dc, b_dc);
    fprintf(fob, "%s %s\n", b_ic, b_dc);
    /* code */
    {
        int i;
        for (i=0;i<st->ic;i++) {
        char addr[16], word[6]; to_base4a_addr(100+i, addr); to_base4a(st->code[i], word);
        fprintf(fob, "%s\t%s\n", addr, word);
    }
    /* data after code */
    }
    {
        int i;
        for (i=0;i<st->dc;i++) {
        char addr[16], word[6]; to_base4a_addr(100+st->ic+i, addr); to_base4a(st->data[i], word);
        fprintf(fob, "%s\t%s\n", addr, word);
    }
    }
    fclose(fob);

    /* .ent (only if at least one) */
    {
        int wrote_ent=0; FILE *fent=NULL; Sym *siter;
        for (siter=st->symbols; siter; siter=siter->next) {
            if (siter->attrs & ATTR_ENTRY) {
                if (!fent){ fent=fopen(ent,"w"); if(!fent){fprintf(stderr,"Error: cannot create %s\n",ent); break;} }
                {
                    char addr[16];
                    to_base4a_addr(siter->value, addr);
                    fprintf(fent, "%s\t%s\n", siter->name, addr);
                    wrote_ent=1;
                }
            }
        }
        if (fent) fclose(fent);
        if (!wrote_ent) remove(ent);
    }

    /* .ext (only if at least one) */
    {
        int wrote_ext=0; FILE *fext=NULL; const ExtRef *e;
        for (e=st->extrefs; e; e=e->next){
            if(!fext){ fext=fopen(ext,"w"); if(!fext){fprintf(stderr,"Error: cannot create %s\n",ext); break;} }
            {
                char addr[16];
                to_base4a_addr(e->address, addr);
                fprintf(fext, "%s\t%s\n", e->name, addr);
                wrote_ext=1;
            }
        }
        if (fext) fclose(fext);
        if (!wrote_ext) remove(ext);
    }

    return 1;
}



