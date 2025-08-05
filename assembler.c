#include "globals.h"
#define OK 0
#define ERROR 1

int main(int argc, char *argv[])
{
    FILE *ifp, *ofp;
macro *macros;
    if (argc == 1)
      {     fprintf(stderr, "Usage: %s <input-files>\n", argv[0]);}

    while (--argc > 0) {
        if ((ifp = fopen(*++argv, "r")) == NULL) {
            fprintf(stderr, "Error: couldn't open input file %s\n", *argv);
            continue;
        }

        build_new_file_name(*argv, ".s") ;

        if ((ofp = fopen(*argv, "w")) == NULL) {
            fprintf(stderr, "Error: couldn't open output file %s\n", *argv);
            fclose(ifp);
            continue;
        }

    
   
 macros = make_macro(ifp);
        rewind(ifp);  

       
        process_file(ifp, ofp, macros);

        fclose(ifp);
        fclose(ofp);
      
    }
    return OK;
}


