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
    int taillePiste = -1;
    int deltaTime = -1;// le delta time de la piste

    // des variables pour les meta events
    unsigned char* nomPiste;
    unsigned char* copyright;

    char type = -1; // le type de meta event --> https://www.csie.ntu.edu.tw/~r92092/ref/midi/#meta_event
    unsigned int tailleEvent = 0; // la taille des donnees dans le meta event (en Variable Length Quantity)
    unsigned char MSB = 0;

    // des variables pour les MIDI event
    unsigned char channel = 0;
    unsigned char key = 0;
    unsigned char velocity = 0;

    // des variables pour les SysEvent
    // On ignore ces événements donc la seule chose qui nous interresse est la taille des données pour les passer
    // La taille des données des SysEvent est stockée en Variable Length Quantity (VLQ)
    unsigned char VLQ = 0;


    // Lecture de la totalité du fichier caractere par caractere
    while ((x = fgetc(fichier)) != EOF) {
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
        }else{
            // Dans une piste

            if(taillePiste == -1){
                taillePiste = x;
                // lecture de la taille de la piste
                for(int i=0;i<3;i++){
                    x = fgetc(fichier);
                    taillePiste = (taillePiste << 8) + x;
                }
            }
            printf("\nTaille piste = %d\n", taillePiste);

            // On lit la piste
            for(int i = 0;i<taillePiste;i++){
                x = fgetc(fichier);
                // On recupere le deltaTime si on ne l'a pas déjà
                if (deltaTime == -1){
                    VLQ = x;
                    MSB = (VLQ >> 7);
                    deltaTime = (VLQ & 0x7F);
                    while (MSB != 0x0){
                        VLQ = fgetc(fichier);i++;
                        deltaTime = (deltaTime << 7) + (VLQ & 0x7F);
                        MSB = (VLQ >> 7);
                    }
                    printf("\n\nEvenement :\ndelta time = %d\n", deltaTime);
                }else{
                    if(x == 0xFF){ // Un meta event
                        printf("Meta Event\n");
                        type = fgetc(fichier);i++;
                        switch(type){
                            case 0x2 :
                                printf("type %x (Copyright)\n", type);
                                VLQ = fgetc(fichier);i++;
                                MSB = (VLQ >> 7);
                                tailleEvent = (VLQ & 0x7F);
                                while (MSB != 0x0){
                                    VLQ = fgetc(fichier);i++;
                                    tailleEvent = (tailleEvent << 7) + (VLQ & 0x7F);
                                    MSB = (VLQ >> 7);
                                }
                                printf("tailleEvent = %d\n", tailleEvent);
                                copyright = malloc((tailleEvent+1) * sizeof(char));
                                copyright[tailleEvent] = '\0';
                                for(int j=0;j<tailleEvent;j++){
                                    x = fgetc(fichier);i++;
                                    copyright[j] = x;
                                }
                                printf("\ncopyright : \n%s\n", copyright);
                                break;
                            case 0x3 :
                                printf("type %x (Track Name)\n", type);
                                VLQ = fgetc(fichier);i++;
                                MSB = (VLQ >> 7);
                                tailleEvent = (VLQ & 0x7F);
                                while (MSB != 0x0){
                                    VLQ = fgetc(fichier);i++;
                                    tailleEvent = (tailleEvent << 7) + (VLQ & 0x7F);
                                    MSB = (VLQ >> 7);
                                }
                                printf("tailleEvent = %d\n", tailleEvent);
                                nomPiste = malloc((tailleEvent+1) * sizeof(char));
                                nomPiste[tailleEvent] = '\0';
                                for(int j=0;j<tailleEvent;j++){
                                    x = fgetc(fichier);i++;
                                    nomPiste[j] = x;
                                }
                                printf("\nNom de la piste %d : \n%s\n", nbPisteTrouvee, nomPiste);
                                break;
                            case 0x2F:
                                printf("Fin de track (%x)\n\n",type);
                                tailleEvent = fgetc(fichier);i++;
                                if(i!=(taillePiste-1)){
                                    printf("BUG : Fin de piste mais on n\'a pas atteint taillePiste (i=%d et taillePiste = %d)", i, taillePiste);
                                }
                                i = taillePiste; // pour sortir de la boucle
                                break;
                            default:
                                printf("type inconnu (%x)\n",type);
                                VLQ = fgetc(fichier);i++;
                                MSB = (VLQ >> 7);
                                tailleEvent = (VLQ & 0x7F);
                                while (MSB != 0x0){
                                    VLQ = fgetc(fichier);i++;
                                    tailleEvent = (tailleEvent << 7) + (VLQ & 0x7F);
                                    MSB = (VLQ >> 7);
                                }
                                printf("tailleEvent = %d\n", tailleEvent);
                                for(int j=0;j<tailleEvent;j++){
                                    x = fgetc(fichier);i++;
                                }
                                printf("Event ignore\n");
                                break;
                        }
                    }else{ // Pas un meta event
                        if((x==0xF0) || x==0xF7){ // SysEvent
                            printf("\nSysEvent (%x)\n", x);
                            // Attention la taille d'un SysEvent est en Variable Lenght Quantity (VLQ)
                            // VLQ = AB1B2...B7 --> Si A = 0 le nombre est simplement B1B2...B7
                            //                  --> Si A = 1 Le prochain octet est la suite du VLQ
                            VLQ = fgetc(fichier);i++;
                            unsigned char MSB = (VLQ >> 7);
                            unsigned int res = (VLQ & 0x7F);
                            while (MSB != 0x0){
                                VLQ = fgetc(fichier);i++;
                                res = (res << 7) + (VLQ & 0x7F);
                                MSB = (VLQ >> 7);
                            }
                            for(int j=0;j<res;j++){
                                x = fgetc(fichier);i++;
                            }
                            printf("Event ignore\n");
                        }else{ // MIDI Event
                            printf("MIDI Event\n");
                            // on decale de 4 pour ne recuperer que les 4 premiers bits (0x42>>4 = 0x4)
                            switch(x>>4){
                            case 0x8:
                                printf("\nNote Off (%x)\n",x);
                                channel = (x & 0x0F);
                                printf("Channel %d\n", channel);
                                key = fgetc(fichier);i++;
                                printf("Key = %d", key);
                                if (!(key>=0x0 && key<=0x7F)){
                                    printf("\n\n!!!!!!!!!!!!!!!!!!!!!\n     BUG KEY = %x\n!!!!!!!!!!!!!!!!!!!!!\n\n", velocity);
                                }
                                velocity = fgetc(fichier);i++;
                                printf(" | Velocity = %x", velocity);
                                if (!(velocity>=0x0 && velocity<=0x7F)){
                                    printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!\n   BUG VELOCITY = %x\n!!!!!!!!!!!!!!!!!!!!!!!\n\n", velocity);
                                }
                                channel = 0;
                                key = 0;
                                break;
                            default:
                                printf("\nMIDI event inconnu (%x)\n",x);
                                for(int j=0;j<3;j++){
                                    x = fgetc(fichier);i++;
                                }
                                printf("Event ignore\n");
                                break;
                            }
                        }
                    }
                    // Fin de l'evenement on reinitialise les variables
                    deltaTime = -1;
                    tailleEvent = 0;
                    type = -1;
                }
            }
            // Fin de la piste on réinitialise les variables
            taillePiste = -1;
            debutDePisteAttendu = 1;
        }
    }


    /*
    * FERMETURE DU FICHIER
    */

    fclose(fichier);
    return 0;
}
