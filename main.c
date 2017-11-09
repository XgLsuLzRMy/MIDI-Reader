#include <stdio.h>
#include <stdlib.h>

int main()
{
    printf("%d\n", sizeof(char));
    char* chemin_du_fichier = "D:\\Documents\\Cours INSA 4A\\projet math\\MIDI-Reader\\MIDI-Reader\\bach_846.mid";
    FILE *fichier = fopen(chemin_du_fichier, "r");
    //for (int i=0;i<100;i++){
        //char *x = malloc(sizeof(char));
        char x[1];
        fwrite(x, sizeof(char), sizeof(x)/sizeof(char), fichier);
        printf("%c", x);
    //}
    fclose(fichier);
    return 0;
}
