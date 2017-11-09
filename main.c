#include <stdio.h>
#include <stdlib.h>

int main()
{
    /*
    *   ATTENTION A METTRE LA NORME C99 DANS CODEBLOCKS ( Parametres > Compilateur > cocher "Have gcc follow the 1999 ISO C language standard" )
    */

    // Le programme ne fait que lire les 100 premiers carcteres

    printf("%d\n", sizeof(char));
    char* chemin_du_fichier = "bach_846.mid";
    FILE *fichier = fopen(chemin_du_fichier, "r");
    for (int i=0;i<100;i++){
        char *x = malloc(sizeof(char));
        x = fgetc(fichier);
        printf("%c", x);
    }
    fclose(fichier);
    return 0;
}
