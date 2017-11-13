#include <stdio.h>
#include <stdlib.h>


/*
* ==========================================================================================================================================
*   ATTENTION A METTRE LA NORME C99 DANS CODEBLOCKS ( Parametres > Compilateur > cocher "Have gcc follow the 1999 ISO C language standard" )
* ==========================================================================================================================================
*/





unsigned int convertTabToHex(char* t, int taille){
//Ne marche pas...
// Convertit un tableau t = {h1, h2, h3, ... , h(taille-1)}
// En un nombre hexa res = h1h2h3...h(taille-1)
    unsigned int res = t[taille-1];
    for (int i=2;i<=taille;i++){
        res = (t[taille-i]<<(8*(i-1))) | res;
    }
    return res;
}

int main()
{


    /*
    *   OUVERTURE DU FICHIER
    */
    printf("Ouverture du fichier... ");
    char* chemin_du_fichier = "fichier_MIDI_test.mid";
    //char* chemin_du_fichier = "faux_MIDI.mid";
    FILE *fichier = fopen(chemin_du_fichier, "r");
    if (fichier == NULL){
        printf("echec\n");
        return -1;
    }
    printf("succes\n");

    /*
    *   LECTURE DE L'ENTETE
    */
    printf("\n===Analyse de l\'en-tete===\n");
    // On lit le fichier caractere ASCII par caractere ASCII
    // Un char (charactere) en C fait 8 bits = 1 octet
    // Un char est donc représenté par 2 nombres hexadécimaux


    // On verifie que le fichier commence bien par MThd
    printf("\nVerification du fichier (MThd)...\n");
    char MThd[4] = {0x4D,0x54,0x68,0x64};
    char MThdVerif = 1; //booleen valant 0 si le fichier n'est pas un fichier MIDI sinon il vaut 1

    char *x = malloc(sizeof(char));
    for (int i=0;i<4;i++){
        *x = fgetc(fichier);
        printf("%4x ", *x);
        // On compare les octets lus avec les octets composants MThd
        if ((*x)!=MThd[i]){
            MThdVerif = 0;
            printf("Le fichier n\'est pas un fichier MIDI...");
            return 1;
        }
    }

    if (MThdVerif){
        printf("\nLe fichier est un fichier MIDI !\n\n");
    }

    // Lecture du nombre d'octets pris par les données de l'en-tete
    unsigned int tailleDonneesEntete = 0x0;
    //char* tmp = malloc(4*sizeof(char));
    for (int i=0;i<4;i++){
        *x = fgetc(fichier);
        printf("%4x ", *x);
        //tmp[i] = *x;
        // On recupere la valeur en convertissant les caracteres en 1 seul nombre hexa
        tailleDonneesEntete = tailleDonneesEntete + ((*x)<<(8*(4-(i+1))));
    }
    //tailleDonneesEntete = convertTabToHex(tmp, 4);
    printf("\ntaille donnes entete = %d\n", tailleDonneesEntete);
    if (tailleDonneesEntete != 6){
        printf("\nLa taille des donnees de l\'entete est differente de 6. Il y a un probleme avec le fichier, on quitte\n");
        return 2;
    }
    unsigned int format = 0x0;
    unsigned int nbPistes = 0x0;
    unsigned int division = 0x0;
    for (int i=0;i<tailleDonneesEntete;i++){
        *x = fgetc(fichier);
        // Recuperation du type de fichier MIDI
        if (i==0){
            format = format + ((*x) << 8);
        }else if(i==1){
            format = format + (*x);
        }
        // Recuperation du nombre de pistes
        else if(i==2){
            nbPistes = nbPistes + ((*x) << 8);
        }else if(i==3){
            nbPistes = nbPistes + (*x);
        }
        // Recuperation de la division
        else if(i==4){
            division = division + ((*x) << 8);
        }else if(i==5){
            division = division + (*x);
        }
    }

    char MSBdivision = division & (1u << 2);

    printf("\nformat = %d\n", format);
    printf("\nombre de pistes = %d\n", nbPistes);
    printf("\ndivision = %d\n", division);
    printf("\n15eme bit de division = %d\n", MSBdivision);




    /*
    *   LECTURE DU CORPS
    */



    /*
    * FERMETURE DU FICHIER
    */

    fclose(fichier);
    return 0;
}
