#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "protokol.h"

void pogledajListu(int sock);
void pogledajPrijedloge(int sock);
void predloziBlok(int sock);
void kraj(int sock);

int main(int argc, char **argv){
    if(argc != 3){
        printf("Upotreba: %s IP-adresa port", argv[0]);
        exit(1);
    }

    char dekadskiIP[20];
    strcpy(dekadskiIP, argv[1]);
    int port;
    sscanf(argv[2], "%d", &port);

    //socket
    int mySocket = socket(PF_INET, SOCK_STREAM, 0);
    if(mySocket == -1){
        perror("socket");
    }
    
    //connecting
    struct sockaddr_in serverAdresa;

    serverAdresa.sin_family = AF_INET;
    serverAdresa.sin_port = htons(port);
    if(inet_aton(dekadskiIP, &serverAdresa.sin_addr) == 0){
        printf("Kriva IP adresa!");
        exit(2);
    }
    memset( serverAdresa.sin_zero, '\0', 8 );

    if( connect( mySocket,(struct sockaddr *) &serverAdresa, sizeof(serverAdresa)) == -1) perror( "connect");

    int gotovo = 0;
    while(!gotovo){
        printf( "\n\nOdaberite jednu od opcija:\n"
				"   1. Pogledaj trenutnu listu\n"
				"   2. Pogledaj prijedloge za novi blok\n"
				"   3. Predlozi novi blok\n"
				"   4. Kraj\n"
				"   \n: " );

        int opcija;
        scanf("%d", &opcija);

        switch(opcija)
		{
			case 1: pogledajListu(mySocket); break;
			case 2: pogledajPrijedloge(mySocket); break;
			case 3: predloziBlok(mySocket); break;
			case 4: kraj(mySocket); gotovo = 1; break;
			default: printf( "Pogresan odabir!\n" ); break;
		}
    }

    return 0;
}

void pogledajListu(int sock) {
    posaljiPoruku(sock, LISTA, "");
    int vrstaPoruke, i;
    char *odgovor;
    int ukupnoBlokova_n, ukupnoBlokova;

    if (recv(sock, &ukupnoBlokova_n, sizeof(ukupnoBlokova_n), 0) != sizeof(ukupnoBlokova_n)) 
        error1("Neuspjelo primanje velicine liste blokova.");
    ukupnoBlokova = ntohl(ukupnoBlokova_n);
    
    if (primiPoruku(sock, &vrstaPoruke, &odgovor) != OK) 
        error1("Pogreska u komunikaciji sa serverom.");
    if (vrstaPoruke != ODGOVOR)
        error1("Pogreska u komunikaciji sa serverom.");
    if (strcmp(odgovor, "OK") != 0) {
        printf("Greska: %s.\n", odgovor);
        exit(0);
    } 
    
    printf("Lista blokova: \n");
    /*
    int ukupnoBlokova_n, ukupnoBlokova;
    sscanf(odgovor, "%d", &ukupnoBlokova_n);
    int ukupnoBlokova = ntohl(ukupnoBlokova_n), i;
    */

    for (i = 0; i < ukupnoBlokova; i++) {
        if (primiPoruku(sock, &vrstaPoruke, &odgovor) != OK) 
            error1("Pogreska u komunikaciji sa serverom.");
        if (vrstaPoruke != LISTA_R)
            error1("Pogreska u komunikaciji sa serverom.");
        printf("%d. %s\n", i + 1, odgovor);
        free(odgovor);
    }
    return;
}

void pogledajPrijedloge(int sock) {
    posaljiPoruku(sock, PRIJEDLOZI, "");
    int vrstaPoruke;
    char *odgovor;
    
    int ukupnoPrijedloga_n;
    if (recv(sock, &ukupnoPrijedloga_n, sizeof(ukupnoPrijedloga_n), 0) == -1) 
        error1("Neuspjelo primanje velicine liste prijedloga.");
    
    if (primiPoruku(sock, &vrstaPoruke, &odgovor) != OK) 
        error1("Pogreska u komunikaciji sa serveromA.");
    if (vrstaPoruke != ODGOVOR)
        error1("Pogreska u komunikaciji sa serveromB.");
    if (strcmp(odgovor, "OK") != 0) {
        printf("Pogreska: %s\n", odgovor);
        return;
    }
    int ukupnoPrijedloga = ntohl(ukupnoPrijedloga_n), i;
    
    printf("\nLista prijedloga za novi blok: \n");
    for (i = 0; i < ukupnoPrijedloga; i++) {
        if (primiPoruku(sock, &vrstaPoruke, &odgovor) != OK) 
            error1("Pogreska u komunikaciji sa serverom.");
        if (vrstaPoruke != PRIJEDLOZI_R)
            error1("Pogreska u komunikaciji sa serverom.");
        printf("%d. %s\n", i + 1, odgovor);
        free(odgovor);
    }
    
    return;
}

void predloziBlok(int sock) {
    char blok[10];
    printf("Unesite prijedlog: ");
    scanf("%s", blok);
    posaljiPoruku(sock, PRIJEDLOG, blok);
    int vrstaPoruke;
    char *odgovor;
    if (primiPoruku(sock, &vrstaPoruke, &odgovor) != OK) 
        error1("Pogreska u komunikaciji sa serverom.");
    if (vrstaPoruke != ODGOVOR)
        error1("Pogreska u komunikaciji sa serverom.");
    if (strcmp(odgovor, "OK"))
        printf("Pogreska: %s\n", odgovor);
    else
        printf("Zahtjev za prijedlog bloka je uspjesno obradjen!");
}

void kraj(int sock) {
    posaljiPoruku(sock, BOK, "");
    int vrstaPoruke;
    char *odgovor;
    if (primiPoruku(sock, &vrstaPoruke, &odgovor) != OK) 
        error1("Pogreska u komunikaciji sa serverom.");
    if (vrstaPoruke != ODGOVOR)
        error1("Pogreska u komunikaciji sa serverom.");
    if (strcmp(odgovor, "OK"))
        printf("Pogreska: %s\n", odgovor);
    else
        printf("Zahtjev za odjavu sa servera je uspjesno obradjen!");
}

