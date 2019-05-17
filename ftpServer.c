#include "ftp.h"

Server_t Server;

extern unsigned int tempo;	// Tempo de espera de delay

void serverStart();
void serverBody();
void serverStop();

int main(int argc, char *argv[]){
	serverStart();
	serverBody();
	serverStop();
	return 0;
}

void serverStart()
{
	FILE *file;

	Server.sock_server_size = sizeof(Server.saddr);
	memset(&Server.saddr, 0, Server.sock_server_size);

	Server.s0 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	

	Server.saddr.sin_family = AF_INET;
	Server.saddr.sin_port = htons(PORT);
	Server.saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(Server.s0, (struct sockaddr *)&Server.saddr, Server.sock_server_size) == -1){
        perror("Bind");
        exit(1);
    }
    else{
        printf("Bind ok!\n");
    }

    if(getsockname(Server.s0, (struct sockaddr *)&Server.saddr, &Server.sock_server_size) == -1){
    	perror("Get sock name");
    	exit(1);
    }
    else{
   		printf("Port: %d\n", htons(Server.saddr.sin_port));	
 	}
    system("caminho=`pwd`\"/ip.txt\"; hostname -I | cut -d ' ' -f 1 > $caminho");
	
	file = fopen ("ip.txt", "r");
    while(!feof(file)){
		fscanf(file, "%s", Server.ip);
	}
	fclose(file);
    system("caminho=`pwd`\"/ip.txt\"; rm $caminho");
	printf("IP (%s)\n", Server.ip);
}

void serverBody()
{
	FILE *fd;
	int i, bytes;
	request_t request;
	char buffer[MAXBUFF], ack = OK, echo = 0;
	Buffer_t buff;
	int buff_control_size = sizeof(buff) - sizeof(buff.data);
	int x = 0;

	while(1) {
		Server.saddr.sin_addr.s_addr = htonl(INADDR_ANY);
		printf("\nEsperando requisicao de servico\n");
		/* aguardando requisicao de servido */
		recvfrom(Server.s0, &request, sizeof(request), 0, (struct sockaddr *)&Server.saddr, &Server.sock_server_size);	
		printf("Requisicao  %s - arquivo: %s\n", request.ip, request.filename);
			
		if (!strcmp(request.ip, Server.ip)) {
			strcpy(buff.filename, request.filename);
			buff.flags  = 0;
			buff.pkg_n  = 0;
			buff.data_n = 0;
			buff.end_block = 0;

			/* tenho um trabalho a fazer */
			Server.saddr.sin_addr.s_addr = inet_addr(request.clientip);
 #ifdef __SECURITY__
			buff.flags  = RR;
			buff.end_block = RR;		    
 #endif

			if (request.op == UPLOAD){
 #ifdef __SECURITY__
				ack = OK;
		    	sendto(Server.s0, &ack, sizeof(ack), 0, (struct sockaddr *)&Server.saddr, Server.sock_server_size);
 #endif
				printf("Fazendo UPLOAD arquivo %s\n", request.filename);//debug
				bytes = 0;
				fd = fopen(request.filename, "wb");
				if (!fd){
					fprintf(stderr, "Arquivo '%s' nao foi criando\n", request.filename);
				} else {
					i = MAXBUFF;
		       		printf("Escrevendo %s\n", request.filename);//debug
					while(i > 0){
		       		 	recvfrom(Server.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Server.saddr, &Server.sock_server_size);
		       		 	i = buff.data_n;
						loading();
 #ifdef __SECURITY__
						ack = (verify(buff.flags) && verify(buff.end_block)) ? OK : REPEAT;
						//ack = (buff.flags == OK && buff.end_block == OK) ? OK : REPEAT;
	#ifdef __DELAY__
	   		 	usleep(tempo);
	#endif
		       		 	sendto(Server.s0, &ack, sizeof(ack), 0, (struct sockaddr *)&Server.saddr, Server.sock_server_size);
 						//printf("[%s] acknowledge\n", (ack == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mREPEAT\e[;m");
 						while (ack != OK) {
		       		 		recvfrom(Server.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Server.saddr, &Server.sock_server_size);
							
							ack = (verify(buff.flags) && verify(buff.end_block)) ? OK : REPEAT;
							//ack = (buff.flags == OK && buff.end_block == OK) ? OK : REPEAT;
	#ifdef __DELAY__
				   		 	usleep(tempo);
	#endif
		       		 		sendto(Server.s0, &ack, sizeof(ack), 0, (struct sockaddr *)&Server.saddr, Server.sock_server_size);
 							printf("[%s] acknowledge\n", (ack == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mREPEAT\e[;m");
 						}
						i = buff.data_n;
 #endif
		       		 	bytes += i;
						fwrite(buff.data, sizeof(char), buff.data_n, fd);
					}
					printf("\nBytes recebidos: %d\n", bytes);
				}
				fclose(fd);
			} else if (request.op == DOWNLOAD){
				printf("Fazendo DOWNLOAD arquivo %s\n", request.filename);//debug
				bytes = 0;
				fd = fopen(request.filename, "rb");
 #ifdef __SECURITY__
				ack = (!fd)? FAIL : OK;
		    	sendto(Server.s0, &ack, sizeof(ack), 0, (struct sockaddr *)&Server.saddr, Server.sock_server_size);
 #endif
				if (!fd){
					fprintf(stderr, "Arquivo '%s' nao foi encontrado\n", request.filename);
				} else {
					i = MAXBUFF;
		       		printf("Lendo %s\n", request.filename);//debug
					x = 0;
					while(i > 0){
						buff.data_n = fread(&buff.data, sizeof(char), MAXBUFF, fd);
		       		 	i = buff.data_n;
		       		 	bytes += i;
						//loading();
	#ifdef __DELAY__
		       		 	usleep(tempo);
	#endif
		       		 	sendto(Server.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Server.saddr, Server.sock_server_size);
 #ifdef __SECURITY__
		       		 	recvfrom(Server.s0, &echo,sizeof(echo), 0, (struct sockaddr *)&Server.saddr, &Server.sock_server_size);
 						printf("[%s] echo\n", (echo == OK)? "\e[32;1mOK\e[;m" : "\e[31;1mREPEAT\e[;m");

 						while(echo == REPEAT){
	#ifdef __DELAY__
	   					 	usleep(tempo);
	#endif
		       		 		sendto(Server.s0, &buff, sizeof(buff), 0, (struct sockaddr *)&Server.saddr, Server.sock_server_size);
 							printf("Entrou no laco repeat\n");
		       		 		recvfrom(Server.s0, &echo,sizeof(echo), 0, (struct sockaddr *)&Server.saddr, &Server.sock_server_size);

 						}
 						echo = NOTHING;
 #endif
					}
					printf("\nBytes enviados: %d\n", bytes);
				}
				fclose(fd);
			}
		}
	}
}

void serverStop()
{
	shutdown(Server.s0, 2);
	close(Server.s0);
}

