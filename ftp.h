// Bibliotecas padroes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
// Bibliotecas socket
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>

#define PORT 61021
#define MAXBUFF 1024

#define __MESMO_PC__

#define __SECURITY__

#define __DELAY__

#define RR 0x0080
#define RNR 0x0040

unsigned int tempo = 1;	// Tempo de espera de delay

enum command_t {NONE, PUT, GET, QUIT};
enum commandServer_t {ANYTHING, UPLOAD, DOWNLOAD};
enum acknowledgement_t{NOTHING, OK, REPEAT, FAIL};

typedef struct {
	int s0;
	unsigned sock_server_size;
	struct sockaddr_in saddr;
	char ip[16];
} Server_t;

typedef struct {
	int op;
	char ip[16];
	char clientip[16];
	char filename[32];
} request_t;

typedef struct {
	int flags;
	int pkg_n;
	int data_n;
	char filename[32];
	char data[MAXBUFF];
	int end_block;
} Buffer_t;

/* Descobre a instrucao requisitada */ 
int decodeComand(char *entrada);
int verify(int entrada);
void loading();

void loading()
{
	static char load[] = {'-','\\','|','/'};
	static int i;
	system("clear");
	printf("(%c)\n", load[i/10]);
	i = ((++i) %(4*10));
}

/* Verifica os bits [7, 6] - retorna 1 se for RR e 0 em outros casos */
int verify(int entrada)
{
	entrada &= 0x00C0;
	//printf("%s\n", (entrada == RR)? "RR" : (entrada == RNR)? "RNR" : "?");//debug
	if (entrada == RR){
		return 1;
	}
	return 0;
}
