#include <stdio.h>
#include <stdlib.h>


/*
* ==========================================================================================================================================
*   ATTENTION A METTRE LA NORME C99 DANS CODEBLOCKS ( Parametres > Compilateur > cocher "Have gcc follow the 1999 ISO C language standard" )
* ==========================================================================================================================================
*/





unsigned int convertTabToHex(char* t, int taille){
// Convertit un tableau t = {h1, h2, h3, ... , h(taille-1)}
// En un nombre hexa res = h1h2h3...h(taille-1)
// ex : t = {E0 , F2} taille = 2 --> convertTabhex(t, taille) = E0F2
    unsigned int res = t[taille-1];
    for (int i=2;i<=taille;i++){
        res = (t[taille-i]<<(8*(i-1))) + res;
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
    unsigned char MThd[4] = {0x4D,0x54,0x68,0x64};
    unsigned char MThdVerif = 1; //booleen valant 0 si le fichier n'est pas un fichier MIDI sinon il vaut 1

    int x = 0;
    for (int i=0;i<4;i++){
        x = fgetc(fichier);
        printf("%4x ", x);
        // On compare les octets lus avec les octets composants MThd
        if ((x)!=MThd[i]){
            MThdVerif = 0;
            printf("Le fichier n\'est pas un fichier MIDI...");
            return 1;
        }
    }

    if (MThdVerif){
        printf("\nLe fichier est un fichier MIDI !\n\n");
    }

    // Lecture du nombre d'octets pris par les données de l'en-tete
    unsigned int tailleDonneesEntete = 0;
    //char* tmp = malloc(4*sizeof(char));
    for (int i=0;i<4;i++){
        x = fgetc(fichier);
        printf("%4x ", x);
        //tmp[i] = x;
        // On recupere la valeur en convertissant les caracteres en 1 seul nombre hexa
        tailleDonneesEntete = tailleDonneesEntete + ((x)<<(8*(4-(i+1))));
    }
    //tailleDonneesEntete = convertTabToHex(tmp, 4);
    printf("\ntaille donnes entete = %d\n", tailleDonneesEntete);
    if (tailleDonneesEntete != 6){
        printf("\nLa taille des donnees de l\'entete est differente de 6. Il y a un probleme avec le fichier, on quitte\n");
        return 2;
    }
    unsigned int format1 = 0;
    unsigned int format2 = 0;
    int nbPistes = 0;
    unsigned char division1 = 0x0;
    unsigned char division2 = 0x0;
    // division = [division1 division2]
    // on lit les données
    for (int i=0;i<tailleDonneesEntete;i++){
        x = fgetc(fichier);
        printf("%x ", x);
        // Recuperation du type de fichier MIDI
        if (i==0){
            format1 =  x;
            if (format1 != 0x0){
                printf("\nFormat MIDI inconnu. On quitte.\n");
                return 3;
            }
        }else if(i==1){
            format2 = (x);
            if (format2 > 2){
                printf("\nFormat MIDI inconnu. On quitte\n");
                return 3;
            }
        }
        // Recuperation du nombre de pistes
        else if(i==2){
            nbPistes = nbPistes + ((x)*16);
        }else if(i==3){
            nbPistes = nbPistes + (x);
        }
        // Recuperation de la division
        else if(i==4){
            division1 = x;
            //printf("\ndivision1 = %x\n", division1);
        }else if(i==5){
            division2 = x;
            //printf("\ndivision2 = %x\n", division2);
        }

        else{
            printf("\n!!! Pb : taille>6 !!!\n");
        }
    }

    unsigned char MSBdivision = division1 & (1u << 7);

    printf("\nformat = %x%x\n", format1, format2);
    printf("\nNombre de pistes = %d\n", nbPistes);
    printf("\ndivision = %x %x\n", division1, division2);
    printf("\n15eme bit de division = %d\n", MSBdivision);

    if (MSBdivision){
        printf("\nDivision dans le cas MSB=1\n");
        // On place le 15eme bit (MSBdivision) à 0
        unsigned int mask = 0x10F447; // 0111 1111 --> le MSB est a 0 dans le mask donc il sera mis à 0 dans le nombre final
        division1 = division1 & mask;
        // division1 est le nombre d'images par secondes
        // division2 est le nombre de delta-time par image
    }else{
        printf("\nDivision dans le cas MSB=0\n");
        unsigned int division = division2 + (division1 << 8);
        printf("\ndivision = %d + %d = %d delta-time\n\n", division2, division1<<8, division);
    }

    /*
    *   LECTURE DU CORPS
    */

    // vaut 0 si on est dans une piste
    // vaut 1 si on n'est pas dans une piste et qu'on attend de voir 'MTrk'
    // vaut l'indice+1 du tableau MTrk sinon. Par exemple, si on attend le début de piste, on attend le caractere 'M'
    // donc debutDePisteAttendu = 1 car MTrk[1 - 1] = MTrk[0] = 'M'
    // si on trouve ce 'M' dans le fichier, alors debutDePisteAttendu augmente de 1 car on attend maintenant un 'T'
    // et MTrk[2 - 1] = MTrk[1] = 'T'
    char debutDePisteAttendu = 1;
    char MTrk[4] = {0x4D, 0x54, 0x72, 0x6B};
    int nbPisteTrouvee = 0;

    // Lecture de la totalité du fichier caractere par caractere
    while ((x = fgetc(fichier)) != EOF) {
        //printf("%x ", x);
        if (debutDePisteAttendu){
            if ((debutDePisteAttendu<4) && (x == MTrk[debutDePisteAttendu-1])){
                debutDePisteAttendu++;
            }else if(debutDePisteAttendu==4){
                nbPisteTrouvee++;
                printf("\nPiste trouvee (%d/%d)!!\n", nbPisteTrouvee, nbPistes);
                debutDePisteAttendu = 0;
            }else{
                debutDePisteAttendu = 1;
            }
        }
    }


    /*
    * FERMETURE DU FICHIER
    */

    fclose(fichier);
    return 0;
}
