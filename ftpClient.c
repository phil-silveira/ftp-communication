#include "ftp.h"

Server_t Client;
request_t request;
char ack = OK, echo = 0;

extern unsigned int tempo;	// Tempo de espera de delay

void clientStart();
void clientStop();
void ftpGet();
void ftpPut();
void loading();

int main()
{
	char linha[40], *plv; 
	int op;

	clientStart();

	system("clear");
	printf("+----------------------------------------+\n");
	printf("|=======| File.Transfer.Protocol |=======|\n");
	printf("+----------------------------------------+\n\n");

	while(1){
		
		printf("ftp> ");
		gets(linha);
		if(!decodeComand(linha)) {	// linha nao esta vazia
			Client.saddr.sin_addr.s_addr = inet_addr(request.ip);
			switch(request.op) {
			case GET:
				ftpGet();
				break;
			case PUT:
				ftpPut();
				break;
			case QUIT:
				clientStop();
				return 0;
				break;
			default:
				printf("* comando desconhecido\n");
			}
		} else {
			printf("* sintaxe errada\n");
		}
	}

	return 0;
}

/* 	formata linha : COMANDO IP@ARQUIVO 

   	return 0 sucesso -1 erro de sintaxe
*/
int decodeComand(char *linha)
{
	char comandos[3][4] = {{"put"},{"get"},{"quit"}}, *tk;
	int i;

	tk = strtok(linha, " ");
	request.op = NONE;
	if (tk != NULL) {
		for (i = 0; i < 3; ++i) {
			if(!strcmp(tk, comandos[i])) {
				request.op = ++i;
			}
		}
		if (request.op == QUIT)	{
			return 0;
		}
		tk = strtok(NULL, "@");
		if (tk != NULL) {
			strcpy(request.ip, tk);
			tk = strtok(NULL, " ");
			if (tk != NULL) {
				strcpy(request.filename, tk);
				return 0;
			}
		}
	}
	return -1;
}

void ftpGet()
{
	int i, bytes = 0;
	FILE *fd;
	char buffer[MAXBUFF];
	Buffer_t buff;
	int buff_control_size = sizeof(buff) - sizeof(buff.data);
	int x = 0;

	sendto(Client.s0, &request, sizeof(request), 0, (struct sockaddr *)&Client.saddr, Client.sock_server_size);
 #ifdef __SECURITY__
 	i = recvfrom(Client.s0, &echo, sizeof(echo), 0, (struct sockaddr *)&Client.saddr, &Client.sock_server_size);
 	printf("[%s] request\n", (echo == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mFAIL\e[;m");
 	if (echo == FAIL){
		printf("Erro no DOWNLOAD de %s\n", request.filename);//debug
 		return;
 	}
 #endif
	printf("Fazendo DOWNLOAD de %s\n", request.filename);//debug
	fd = fopen(request.filename, "wb");
	if (!fd){
		fprintf(stderr, "Arquivo '%s' nao foi criando\n", request.filename);
	} else {
		i = MAXBUFF;
		buff.pkg_n = 0;
   		//printf("Escrevendo %s\n", request.filename);//debug
		while(i > 0){
   		 	recvfrom(Client.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Client.saddr, &Client.sock_server_size);
   		 	i = buff.data_n;
   		 	loading();
 #ifdef __SECURITY__
			ack = (verify(buff.flags) && verify(buff.end_block)) ? OK : REPEAT;
	
	// TESTE DO REENVIO		
			++x;
	   		if (x == 3){
	   			x = 0;
	   			ack = REPEAT;
	   		}
	// FIM DO TESTE
	
			//ack = (buff.flags == OK && buff.end_block == OK) ? OK : REPEAT;
   		 	sendto(Client.s0, &ack, sizeof(ack), 0, (struct sockaddr *)&Client.saddr, Client.sock_server_size);
 			//printf("[%s] acknowledge\n", (ack == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mREPEAT\e[;m");
   			while (ack != OK) {
	   		 	recvfrom(Client.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Client.saddr, &Client.sock_server_size);
 							
				ack = (verify(buff.flags) && verify(buff.end_block)) ? OK : REPEAT;
				//ack = (buff.flags == OK && buff.end_block == OK) ? OK : REPEAT;
	#ifdef __DELAY__
	   		 	usleep(tempo);
	#endif
   			 	sendto(Client.s0, &ack, sizeof(ack), 0, (struct sockaddr *)&Client.saddr, Client.sock_server_size);
 				//printf("[%s] acknowledge\n", (ack == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mREPEAT\e[;m");
 			}
   		 	i = buff.data_n;
 #endif
   		 	bytes += i; 
			fwrite(&buff.data, sizeof(char), buff.data_n, fd);

 			buff.pkg_n++;
		}
		printf("\nBytes recebidos: %d\n", bytes);
	}
	fclose(fd);
}

void ftpPut()
{
	int i, bytes = 0;
	FILE *fd;
	char buffer[MAXBUFF];
	Buffer_t buff;
	int buff_control_size = sizeof(buff) - sizeof(buff.data);

	buff.flags  = RR;
	buff.end_block = RR;
		    
	//printf("comando (%d) - ip (%s) - arquivo (%s) - porta (%d)\n", request.op, request.ip, request.filename, htons(Client.saddr.sin_port));
	fd = fopen(request.filename, "rb");
	if (!fd){
		fprintf(stderr, "Arquivo '%s' nao foi encontrado\n", request.filename);
		return;		
	} 
	sendto(Client.s0, &request, sizeof(request), 0, (struct sockaddr *)&Client.saddr, Client.sock_server_size);
 #ifdef __SECURITY__
 	i = recvfrom(Client.s0, &echo, sizeof(echo), 0, (struct sockaddr *)&Client.saddr, &Client.sock_server_size);
 	printf("[%s] request\n", (echo == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mFAIL\e[;m");
 	if (echo != OK){
 		return;
 	}
 #endif
	printf("Fazendo UPLOAD arquivo %s\n", request.filename);//debug
	i = MAXBUFF;
	while(i > 0){
		buff.data_n = fread(&buff.data, sizeof(char), MAXBUFF, fd);
		i = buff.data_n;
   	 	bytes = bytes + i; 
	 	loading();
	#ifdef __DELAY__
   	 	usleep(tempo);
	#endif
	 	sendto(Client.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Client.saddr, Client.sock_server_size);
 #ifdef __SECURITY__
   	 	recvfrom(Client.s0, &echo, sizeof(echo), 0, (struct sockaddr *)&Client.saddr, &Client.sock_server_size);
 		//printf("[%s] echo\n", (echo == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mREPEAT\e[;m");
 		while(echo == REPEAT){
	#ifdef __DELAY__
		 	usleep(tempo);
	#endif
	 		sendto(Client.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Client.saddr, Client.sock_server_size);
		 	recvfrom(Client.s0, &echo, sizeof(echo), 0, (struct sockaddr *)&Client.saddr, &Client.sock_server_size);
 		}
 		echo = NOTHING;
 #endif
	}
	printf("\nBytes enviados: %d\n", bytes);
	
	fclose(fd);
	printf("\n");
}

void clientStart()
{
	FILE *file;

	Client.sock_server_size = sizeof(Client.saddr);
	memset(&Client.saddr, 0, Client.sock_server_size);

	Client.s0 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	

	Client.saddr.sin_family = AF_INET;
	Client.saddr.sin_port = htons(PORT);
	Client.saddr.sin_addr.s_addr = htonl(INADDR_ANY);

 #ifndef __MESMO_PC__
    if(bind(Client.s0, (struct sockaddr *)&Client.saddr, Client.sock_server_size) == -1){
        perror("Bind");
        exit(1);
    }
    else{
        printf("Bind ok!\n");
    }

    if(getsockname(Client.s0, (struct sockaddr *)&Client.saddr, &Client.sock_server_size) == -1){
    	perror("Get sock name");
    	exit(1);
    }
    else{
    	printf("Port: %d\n", htons(Client.saddr.sin_port));	
    }
 #endif
    system("caminho=`pwd`\"/ip.txt\"; hostname -I | cut -d ' ' -f 1 > $caminho");
	
	file = fopen ("ip.txt", "r");
    while(!feof(file)){
		fscanf(file, "%s", Client.ip);
	}
	fclose(file);
    system("caminho=`pwd`\"/ip.txt\"; rm $caminho");
	strcpy(request.clientip, Client.ip);
	printf("IP: %s\n", request.clientip);
}

void clientStop()
{
	shutdown(Client.s0, 2);
	close(Client.s0);
}

