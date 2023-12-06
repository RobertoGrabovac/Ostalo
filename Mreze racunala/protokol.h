#ifndef PROTOKOL_H
#define PROTOKOL_H

#define LISTA           1
#define LISTA_R         2
#define PRIJEDLOZI      3
#define PRIJEDLOZI_R    4
#define PRIJEDLOG       5
#define BOK             6
#define ODGOVOR         7

#define OK              1
#define NIJE_OK         2

#define error1( s ) { printf( s ); exit( 0 ); }
#define error2( s1, s2 ) { printf( s1, s2 ); exit( 0 ); }
#define myperror( s ) { perror( s ); exit( 0 ); }

int primiPoruku( int sock, int *vrstaPoruke, char **poruka);
int posaljiPoruku( int sock, int vrstaPoruke, const char *poruka);

#endif