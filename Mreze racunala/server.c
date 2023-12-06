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
#include <pthread.h>

#define MAXDRETVI 		10
#define MAXBLOKOVA 		10
#define MAXKONEKCIJA    10

typedef struct{
	int commSocket;
	int indexDretve;
} obradiKlijenta__parametar;

int aktivneDretve[MAXDRETVI] = {0};
obradiKlijenta__parametar parametarDretve[MAXDRETVI];
pthread_mutex_t lokot_aktivneDretve = PTHREAD_MUTEX_INITIALIZER;

//int brojBlokova, brojPrijedloga;
char popisBlokova[MAXBLOKOVA][10];
char popisPrijedloga[MAXDRETVI][10];

void *obradiKlijenta (void *parametar);
void posaljiListu (int sock, char *poruka);
void pohraniPrijedlog(int sock, char* poruka, int indexDretve);
void posaljiPrijedloge(int sock, char* poruka);
void zavrsiKomunikaciju(int sock, int indexDretve);

void inicijalizacija() {
	int i;
	for (i = 0; i < MAXDRETVI; i++)
		strcpy(popisPrijedloga[i], "");
}

void provjeriFrekvenciju(void) {
	int i, ukupno = 0;
	char kandidat[10];
	strcpy(kandidat, "");
	
	for (i = 0; i < MAXDRETVI; i++) {
		if (strcmp(popisPrijedloga[i], "") != 0) {
			if (ukupno == 0) {
				strcpy(kandidat, popisPrijedloga[i]);
				ukupno = 1;
			} else {
				if (strcmp(popisPrijedloga[i], kandidat) == 0)
					ukupno++;
				else 
					ukupno--;
			}
		}
	}

	int brojac = 0, ukupno_prijedloga = 0;
	
	for (i = 0; i < MAXDRETVI; i++) {
		if (strcmp(popisPrijedloga[i], "") != 0) {
			ukupno_prijedloga++;
		}
		if (strcmp(popisPrijedloga[i], kandidat) == 0)
			brojac++;
	}
	
	if (ukupno_prijedloga <= 1) return;
	
	if (brojac > ukupno_prijedloga / 2) {
		for (i = 0; i < MAXBLOKOVA; i++)
			if (strcmp(popisBlokova[i], "") == 0) {
				strcpy(popisBlokova[i], kandidat);
				break;
			}
		if (i == MAXBLOKOVA) error1("Nema vise mjesta za novi blok!");
		for (i = 0; i < MAXDRETVI; i++)
			strcpy(popisPrijedloga[i], "");
	}	
	
}

int main (int argc, char **argv) {
    if( argc != 2 )
		error2( "Upotreba: %s port\n", argv[0]);
    int port;
    sscanf(argv[1], "%d", &port);

	inicijalizacija(); //svi prijedlozi prazan string

    int listenerSocket = socket(PF_INET, SOCK_STREAM, 0);
    if( listenerSocket == -1 )
		myperror( "socket" );

    struct sockaddr_in mojaAdresa;
    mojaAdresa.sin_family = AF_INET;
    mojaAdresa.sin_port = htons(port);
    mojaAdresa.sin_addr.s_addr = INADDR_ANY;
	memset(mojaAdresa.sin_zero, '\0', 8 );

    if (bind(listenerSocket, (struct sockaddr*) &mojaAdresa, sizeof(mojaAdresa)) == -1)
        myperror( "bind" );
    if (listen(listenerSocket, MAXKONEKCIJA) == -1)
        myperror("listen");

    pthread_t dretve[10];

    while(1) {
        struct sockaddr_in klijentAdresa;
        int size = sizeof(klijentAdresa);
        int commSocket = accept(listenerSocket, (struct sockaddr*) &klijentAdresa, &size);
        if( commSocket == -1 )
			myperror( "accept" );
        char *dekadskiIP;
        dekadskiIP = inet_ntoa(klijentAdresa.sin_addr);
        printf( "Prihvacena konekcija od %s [socket=%d]\n", dekadskiIP, commSocket);
        
        pthread_mutex_lock(&lokot_aktivneDretve);
		int i, indexNeaktivne = -1;
		for( i = 0; i < MAXDRETVI; ++i )
			if(aktivneDretve[i] == 0 )
				indexNeaktivne = i;
			else if(aktivneDretve[i] == 2) {
				pthread_join(dretve[i], NULL);
				aktivneDretve[i] = 0;
				indexNeaktivne = i;
			}

		if(indexNeaktivne == -1){
			close(commSocket);
			printf("Konekcija odbijena. Nema vise slobodnih dretvi.\n");
		}
		else {
		    aktivneDretve[indexNeaktivne] = 1;
			parametarDretve[indexNeaktivne].commSocket = commSocket;
			parametarDretve[indexNeaktivne].indexDretve = indexNeaktivne;
			printf("Koristim dretvu broj %d.\n", indexNeaktivne);

			pthread_create(
				&dretve[indexNeaktivne], NULL,
				obradiKlijenta, &parametarDretve[indexNeaktivne]);
		}
		pthread_mutex_unlock(&lokot_aktivneDretve);
    }

    return 0;
}

void *obradiKlijenta (void *parametar) {
    obradiKlijenta__parametar *param = (obradiKlijenta__parametar*) parametar;
    int sock = param->commSocket;
    int tipPoruke, gotovo = 0;
    char *poruka;

    while(!gotovo) {
		if(primiPoruku( sock, &tipPoruke, &poruka) != OK ) {
			printf("Pogreska pri komunikaciji na [socket=%d]...\n", sock);

			pthread_mutex_lock( &lokot_aktivneDretve);
			aktivneDretve[param->indexDretve] = 2;
			strcpy(popisPrijedloga[param->indexDretve], "");
			provjeriFrekvenciju();
			pthread_mutex_unlock(&lokot_aktivneDretve);

			gotovo = 1;
			continue;
		}
		
		switch(tipPoruke)
		{
			case LISTA: posaljiListu (sock, poruka); break; 
			case PRIJEDLOZI: posaljiPrijedloge(sock, poruka); break;
			case PRIJEDLOG: pohraniPrijedlog(sock, poruka, param->indexDretve); break;
			case BOK: zavrsiKomunikaciju(sock, param->indexDretve); gotovo = 1; break;
			//default: SendMessage( sock, ANSWER, "Message code does not exist!\n" );
		}
		
		free(poruka);
	}
}

void posaljiListu(int sock, char* poruka) {
    int i, ukupno = 0;
    for (i = 0; i < MAXBLOKOVA; i++)
        if (strcmp("", popisBlokova[i])) ukupno++;
    
    char broj[10];
    int ukupno_n = htonl(ukupno);
    /*sprintf(broj, "%d", ukupno_n);
    posaljiPoruku(sock, ODGOVOR, broj);
	*/
	if (send(sock, &ukupno_n, sizeof(ukupno_n), 0) != sizeof(ukupno_n)) {
		printf("Pogreska pri komunikaciji na [socket=%d]...\n", sock);
		posaljiPoruku(sock, ODGOVOR, "slanje velicine liste nije uspjelo.");
		return;
	}
	posaljiPoruku(sock, ODGOVOR, "OK");

    for (i = 0; i < ukupno; i++) //ukupno > MAXBLOKOVA?
		if (strcmp("", popisBlokova[i]))  
            posaljiPoruku(sock, LISTA_R, popisBlokova[i]);

    return;
}

void pohraniPrijedlog(int sock, char* poruka, int indexDretve) {
	/*
	int i;
	for (i = 0; i < MAXDRETVI; i++)
		if (strcmp(popisPrijedloga[i], "") == 0) break;
	if (i == MAXDRETVI) {
		printf("Greska. Nema mjesta za pohranu bloka.");
		return;
	}
	*/

	if (strcmp(popisPrijedloga[indexDretve], "")) {
		posaljiPoruku(sock, ODGOVOR, "Vas prijedlog je vec zabiljezen!");
		return;
	}

	pthread_mutex_lock( &lokot_aktivneDretve); //ovdje mozda ne treba lock?
	posaljiPoruku(sock, ODGOVOR, "OK");
	strcpy(popisPrijedloga[indexDretve], poruka);
	provjeriFrekvenciju();
	pthread_mutex_unlock(&lokot_aktivneDretve);
	
	return;
}

void posaljiPrijedloge(int sock, char* poruka) {
	int i, ukupno = 0;
    for (i = 0; i < MAXDRETVI; i++)
        if (strcmp("", popisPrijedloga[i])) ukupno++;
    int ukupno_n = htonl(ukupno);
   	if (send(sock, &ukupno_n, sizeof(ukupno_n), 0) == -1) {
		posaljiPoruku(sock, ODGOVOR, "slanje velicine liste prijedloga nije uspjelo.");
		return;
	}
	posaljiPoruku(sock, ODGOVOR, "OK");
	
    for (i = 0; i < MAXDRETVI; i++) 
		if (strcmp("", popisPrijedloga[i]))  
            posaljiPoruku(sock, PRIJEDLOZI_R, popisPrijedloga[i]);
	
    return;
}

void zavrsiKomunikaciju(int sock, int indexDretve) {
	posaljiPoruku(sock, ODGOVOR, "OK");

	pthread_mutex_lock( &lokot_aktivneDretve);

	aktivneDretve[indexDretve] = 2;
	strcpy(popisPrijedloga[indexDretve], "");
	provjeriFrekvenciju();

	pthread_mutex_unlock(&lokot_aktivneDretve);
	
	close(sock);
}