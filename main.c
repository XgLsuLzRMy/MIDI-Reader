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
    char* chemin_du_fichier = "NotesRepetees.mid";
    // char* chemin_du_fichier = "fichier_MIDI_test.mid";
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
        // Recuperation du format de fichier MIDI
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
            return 4;
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

    char debutDePisteAttendu = 1;
    // vaut 0 si on est dans une piste
    // vaut 1 si on n'est pas dans une piste et qu'on attend de voir 'MTrk'
    // vaut l'indice+1 du tableau MTrk sinon. Par exemple, si on attend le début de piste, on attend le caractere 'M'
    // donc debutDePisteAttendu = 1 car MTrk[1 - 1] = MTrk[0] = 'M'
    // si on trouve ce 'M' dans le fichier, alors debutDePisteAttendu augmente de 1 car on attend maintenant un 'T'
    // et MTrk[2 - 1] = MTrk[1] = 'T'

    char MTrk[4] = {0x4D, 0x54, 0x72, 0x6B}; // Sert à vérifié qu'on a bien MTrk en début de piste
    int nbPisteTrouvee = 0;
    int taillePiste = -1;
    int deltaTime = -1;// le delta time des événements

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
    // On fait un premier if pour savoir si on attend le début d'une piste (MTrk) ou si on l'a déjà détécté et qu'on est donc dans une piste
        // Si on attend le début d'une piste il faut qu'on ait la séquence MTrk entière

        // Sinon on récupère d'abord la taille de la piste dans laquelle on se trouve si on ne l'a pas déjà récupérée (dans ce cas on se trouve + loin dans la piste)
        // Si la taille a déjà été récupérée, on boucle sur la taille de la piste pour la lire en entier
            // Pour chaque événement de la piste on récupère son deltaTime si on ne l'a pas déjà (dans ce cas on est à l'intérieur d'un événément)
            // On analyse ensuite selon le type d'événement et on fait une boucle sur la taille de cet événement pour le lire en entier

    while ((x = fgetc(fichier)) != EOF) {

        /*
        *
        *
        * Code pour détecter le début d'une piste pas très compréhensible donc à remanier + tard
        *
        *
        */
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
        // Fin si on attend le début d'une piste
        }else{ // Dans une piste

            // On récupère la taille totale de la piste
            // Cette taille n'est pas en Variable Length Quantity (VLQ) donc on la récupère telle quelle
            // Par contre elle est stockée sur 4 octets dont le premier est déjà lu par la boucle while
            // On lit les 3 autres dans une boucle for
            if(taillePiste == -1){
                // lecture de la taille de la piste
                taillePiste = x;
                for(int i=0;i<3;i++){
                    x = fgetc(fichier); // on récupère le reste des octets de la taille de la piste
                    taillePiste = (taillePiste << 8) + x;
                }
            }
            printf("\nTaille piste = %d\n", taillePiste);

            // On lit la piste
            for(int i = 0;i<taillePiste;i++){
                x = fgetc(fichier);
                // On recupere le deltaTime si on ne l'a pas déjà (on a déjà le deltaTime si il est différent de -1)
                // La valeur du deltaTime est réinitialisée à -1 à la fin de chaque événement
                if (deltaTime == -1){
                    // Le deltaTime est en Variable Length Quantity (VLQ) donc il faut l'analyser
                    // Début analyse VLQ (https://en.wikipedia.org/wiki/Variable-length_quantity#Examples)
                    // VLQ = AB1B2...B7 --> Si A = 0 le nombre est simplement B1B2...B7
                    //                  --> Si A = 1 Le prochain octet est la suite du VLQ
                    VLQ = x;
                    MSB = (VLQ >> 7); // on récupère le Most Significant Bit (MSB)
                    deltaTime = (VLQ & 0x7F); // on récupère les 7 bits (on enlève le MSB)
                    while (MSB != 0x0){
                        VLQ = fgetc(fichier);i++; // on récupère le prochain VLQ et on incrémente le compteur puisqu'on avance dans le fichier
                        deltaTime = (deltaTime << 7) + (VLQ & 0x7F); // on ajoute les 7 bits de la nouvelle valeur au deltaTime
                        MSB = (VLQ >> 7); // on récupère le MSB pour savoir si le prochain octet est la suite du VLQ
                    } // Fin analyse VLQ
                    printf("\n\nNouvel evenement :\ndelta time = %d\n", deltaTime);
                }else{ // Si on a déjà récupérer le deltaTime lié à l'événement en cours
                    if(x == 0xFF){ // Un meta event https://www.csie.ntu.edu.tw/~r92092/ref/midi/#meta_event
                        printf("Meta Event\n");
                        type = fgetc(fichier);i++; // le premier octet d'un meta event est son type
                        switch(type){
                            case 0x2 : // type Copyright
                                printf("type %x (Copyright)\n", type);
                                // On récupère la longueur de l'événement
                                // Cette longueur est en VLQ
                                // Debut analyse VLQ
                                VLQ = fgetc(fichier);i++;
                                MSB = (VLQ >> 7); // on récupère le Most Significant Bit (MSB)
                                tailleEvent = (VLQ & 0x7F); // on récupère les 7 bits (on enlève le MSB)
                                while (MSB != 0x0){
                                    VLQ = fgetc(fichier);i++; // on récupère le prochain VLQ et on incrémente le compteur puisqu'on avance dans le fichier
                                    tailleEvent = (tailleEvent << 7) + (VLQ & 0x7F); // on ajoute les 7 bits de la nouvelle valeur au deltaTime
                                    MSB = (VLQ >> 7); // on récupère le MSB pour savoir si le prochain octet est la suite du VLQ
                                } // Fin analyse VLQ
                                printf("tailleEvent = %d\n", tailleEvent);

                                copyright = malloc((tailleEvent+1) * sizeof(char)); // On alloue une chaine de characteres pour récupérer le contenu de l'événement
                                copyright[tailleEvent] = '\0'; // On place le symbole de fin de chaine de caracteres
                                // On lit le contenu de l'événement
                                for(int j=0;j<tailleEvent;j++){
                                    x = fgetc(fichier);i++; // puisqu'on lit un caractere, on incrémente le compteur
                                    copyright[j] = x;
                                }
                                printf("\ncopyright : \n%s\n", copyright);
                                break; // Fin type "Copyright"

                            case 0x3 : // type nom de piste
                                printf("type %x (Track Name)\n", type);
                                // On récupère la longueur de l'événement
                                // Cette longueur est en VLQ
                                // Debut analyse VLQ
                                VLQ = fgetc(fichier);i++;
                                MSB = (VLQ >> 7); // on récupère le Most Significant Bit (MSB)
                                tailleEvent = (VLQ & 0x7F); // on récupère les 7 bits (on enlève le MSB)
                                while (MSB != 0x0){
                                    VLQ = fgetc(fichier);i++; // on récupère le prochain VLQ et on incrémente le compteur puisqu'on avance dans le fichier
                                    tailleEvent = (tailleEvent << 7) + (VLQ & 0x7F); // on ajoute les 7 bits de la nouvelle valeur au deltaTime
                                    MSB = (VLQ >> 7); // on récupère le MSB pour savoir si le prochain octet est la suite du VLQ
                                }  // Fin analyse VLQ
                                printf("tailleEvent = %d\n", tailleEvent);

                                nomPiste = malloc((tailleEvent+1) * sizeof(char)); // On alloue une chaine de characteres pour récupérer le contenu de l'événement
                                nomPiste[tailleEvent] = '\0'; // On place le symbole de fin de chaine de caracteres
                                for(int j=0;j<tailleEvent;j++){
                                    x = fgetc(fichier);i++; // puisqu'on lit un caractere, on incrémente le compteur
                                    nomPiste[j] = x;
                                }
                                printf("\nNom de la piste %d : \n%s\n", nbPisteTrouvee, nomPiste);
                                break; // Fin type "Nom de piste"

                            case 0x2F: // type "fin de piste"
                                printf("Fin de track (%x)\n\n",type);
                                tailleEvent = fgetc(fichier);i++; // On ne fait pas le VLQ car il vaut toujours 0 dans le cas d'une fin de psite
                                // Normalement puisque c'est la fin de la piste on doit retrouver le compteur i égale à la taille théorique de la piste (tailePiste)
                                if(i!=(taillePiste-1)){
                                    printf("BUG : Fin de piste mais on n\'a pas atteint taillePiste (i=%d et taillePiste = %d)", i, taillePiste);
                                }
                                i = taillePiste; // pour être sûr de sortir de la boucle
                                break; // Fin type "Fin de Piste"

                            default: // pour tout autre type qu'on n'a pas analyser
                                printf("type inconnu (%x)\n",type);
                                // On récupère la longueur de l'événement
                                // Cette longueur est en VLQ
                                // Debut analyse VLQ
                                VLQ = fgetc(fichier);i++; // on récupère le Most Significant Bit (MSB)
                                MSB = (VLQ >> 7); // on récupère les 7 bits (on enlève le MSB)
                                tailleEvent = (VLQ & 0x7F);
                                while (MSB != 0x0){
                                    VLQ = fgetc(fichier);i++; // on récupère le prochain VLQ et on incrémente le compteur puisqu'on avance dans le fichier
                                    tailleEvent = (tailleEvent << 7) + (VLQ & 0x7F); // on ajoute les 7 bits de la nouvelle valeur au deltaTime
                                    MSB = (VLQ >> 7); // on récupère le MSB pour savoir si le prochain octet est la suite du VLQ
                                } // Fin analyse VLQ

                                printf("tailleEvent = %d\n", tailleEvent);

                                // On fait une boucle qui lit les caracteres sans rien en faire juste pour ignorer l'événement
                                for(int j=0;j<tailleEvent;j++){
                                    x = fgetc(fichier);i++;
                                }
                                printf("Event ignore\n");
                                break; // Fin autre type
                        }
                    }else{ // Si ce n'est pas un meta event (donc soit un SysEvent soit un MIDI event)
                        if((x==0xF0) || x==0xF7){ // Si c'est un SysEvent
                            printf("\nSysEvent (%x)\n", x);
                            // Attention la taille d'un SysEvent est en Variable Lenght Quantity (VLQ)
                            // Debut analyse VLQ
                            VLQ = fgetc(fichier);i++;
                            unsigned char MSB = (VLQ >> 7);
                            unsigned int res = (VLQ & 0x7F);
                            while (MSB != 0x0){
                                VLQ = fgetc(fichier);i++;
                                res = (res << 7) + (VLQ & 0x7F);
                                MSB = (VLQ >> 7);
                            } // Fin analyse VLQ

                            // On fait une boucle qui lit les caracteres sans rien en faire juste pour ignorer l'événement
                            for(int j=0;j<res;j++){
                                x = fgetc(fichier);i++;
                            }
                            printf("Event ignore\n");

                        }else{ // C'est forcément un MIDI Event
                            printf("MIDI Event");
                            // on decale de 4 pour ne recuperer que les 4 premiers bits (0x42>>4 = 0x4) car il s'agit du type de midi évément
                            // Par exemple si on a x = 0x81 alors (x>>4)=0x8 --> événement "Note OFF" sur la channel (x & 0x0F)=0x1
                            switch(x>>4){
                                case 0x8: // type Note OFF
                                    printf("\nNote Off (%x)\n",x);

                                    channel = (x & 0x0F); // La channel est donnée par les 4 bits de droite (0x81 --> channel 1)
                                    printf("Channel %d\n", channel);

                                    key = fgetc(fichier);i++; // On lit le caractere suivant qui correspond à la touche du piano appuyée
                                    printf("Key = %d", key);
                                    // Normalement key<=88=0x58 mais key peut dans la norme MIDI aller jusqu'à 127=0x7F
                                    // On vérifie donc que key ne dépasse pas cette valeur théorique sinon il y a un problème
                                    if (!(key>=0x0 && key<=0x7F)){
                                        printf("\n\n!!!!!!!!!!!!!!!!!!!!!\n     BUG KEY = %x\n!!!!!!!!!!!!!!!!!!!!!\n\n", velocity);
                                    }

                                    velocity = fgetc(fichier);i++; // On lit le caractere suivant qui correspond à la vélocité c'est à dire le volume de la note
                                    printf(" | Velocity = %x", velocity);
                                    // De même que pour la key, la velocité doit être entre 0 et 127
                                    if (!(velocity>=0x0 && velocity<=0x7F)){
                                        printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!\n   BUG VELOCITY = %x\n!!!!!!!!!!!!!!!!!!!!!!!\n\n", velocity);
                                    }

                                    /*
                                    *
                                    *
                                    * ICI IL FAUT METTRE LA TOUCHE ET SON VOLUME DANS UN TABLEAU
                                    *
                                    *
                                    */

                                    // On réinitialise les variables pour la prochaine lecture
                                    channel = 0;
                                    key = 0;
                                    break; // Fin type "Note OFF"

                                case 0x9: // type Note ON
                                    printf("\nNote On (%x)\n",x);

                                    channel = (x & 0x0F); // La channel est donnée par les 4 bits de droite (0x81 --> channel 1)
                                    printf("Channel %d\n", channel);

                                    key = fgetc(fichier);i++; // On lit le caractere suivant qui correspond à la touche du piano appuyée
                                    printf("Key = %d", key);
                                    // Normalement key<=88=0x58 mais key peut dans la norme MIDI aller jusqu'à 127=0x7F
                                    // On vérifie donc que key ne dépasse pas cette valeur théorique sinon il y a un problème
                                    if (!(key>=0x0 && key<=0x7F)){
                                        printf("\n\n!!!!!!!!!!!!!!!!!!!!!\n     BUG KEY = %x\n!!!!!!!!!!!!!!!!!!!!!\n\n", velocity);
                                    }

                                    velocity = fgetc(fichier);i++; // On lit le caractere suivant qui correspond à la vélocité c'est à dire le volume de la note
                                    printf(" | Velocity = %x", velocity);
                                    // De même que pour la key, la velocité doit être entre 0 et 127
                                    if (!(velocity>=0x0 && velocity<=0x7F)){
                                        printf("\n\n!!!!!!!!!!!!!!!!!!!!!!!\n   BUG VELOCITY = %x\n!!!!!!!!!!!!!!!!!!!!!!!\n\n", velocity);
                                    }

                                    /*
                                    *
                                    *
                                    * ICI IL FAUT METTRE LA TOUCHE ET SON VOLUME DANS UN TABLEAU
                                    *
                                    *
                                    */

                                    // On réinitialise les variables pour la prochaine lecture
                                    channel = 0;
                                    key = 0;
                                    break; // Fin type "Note ON"

                                default: // Si on ne connait pas ce type d'événement MIDI
                                    printf("\nMIDI event inconnu (%x)\n",x);
                                    for(int j=0;j<3;j++){
                                        x = fgetc(fichier);i++;
                                    }
                                    printf("Event ignore\n");
                                    break; // Fin type "autre evenement MIDI"

                            } // Fin du switch sur le type de MIDI event
                        } // Fin MIDI event
                    }// Fin de l'evenement

                    //on reinitialise les variables
                    deltaTime = -1;
                    tailleEvent = 0;
                    type = -1;
                } // Fin du else si on a déjà récupérer le delta time
            } // Fin de la boucle for qui lit la piste

            // Fin de la piste on réinitialise les variables
            taillePiste = -1;
            debutDePisteAttendu = 1;
        } // Fin du else si on est dans une piste
    } // Fin de la boucle while globale sur le fichier


    /*
    * FERMETURE DU FICHIER
    */

    fclose(fichier);
    return 0;
}
