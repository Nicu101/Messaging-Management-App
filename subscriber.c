#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/tcp.h>

#include "helpers.h"

int main(int argc, char *argv[])
{
	int sockfd, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN], send_buffer[BUFLEN];
	char delim[] = " \n";
	char *ptr, *sf;
	char *check; //folosit pentru a verifica daca comenzile au junk la final

	fd_set read_fds;    
	fd_set tmp_fds; 
	int fdmax;

	if (argc < 3) {
		fprintf(stderr,"Usage %s <ID_Client> <IP_Server> <Port_Server>\n", argv[0]);
		exit(0);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	/*Deschidere socket*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	ret = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&ret, sizeof(ret));
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect"); 

	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;  

	/* Sending Client_ID */
	ret = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(ret < 0, "send-Client_ID");
	
	while (1) {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		//citesc de la tastatura
		if (FD_ISSET(0, &tmp_fds)) {
			/* Resetam bufferul si citim datele de la tastatura */
			memset(buffer, 0 , BUFLEN);
			fgets(buffer, BUFLEN-1, stdin);

			ptr = strtok(buffer, delim);
			if (strcmp(ptr, "subscribe") == 0) {
				ptr = strtok(NULL, delim);
				if (ptr != NULL && strlen(ptr) < 50) {
					sf = strtok(NULL, delim);
					if (sf != NULL &&
						((strncmp(sf, "1", 1) == 0) || (strncmp(sf, "0", 1) == 0))) {
						/* Verificam daca comanda are junk la sfarsit*/
						check = strtok(NULL, delim);
						if (check == NULL) {
							/* Creare mesaj de 'subscribe'*/
							memset(send_buffer, 0 , BUFLEN);
							sprintf(send_buffer, "%s subscribe %s %s ", argv[1], ptr, sf);
							/* Trimitere mesaj*/
							ret = send(sockfd, send_buffer, strlen(send_buffer), 0);
							DIE(ret < 0, "send");
							/* Afisare feedback*/
							printf("Sending command:%s\n", send_buffer + strlen(argv[1]));
						} else {
							printf("Invalid command: subscribe %s %s %s\n", ptr, sf, check);
						}
					} else {
						/* Comanda invalida*/
						printf("The <subscribe> command needs SF with values: 0 or 1\n");
					}
				} else {
					printf("Topic <%s> does not exist\n", ptr);
				}
			} else if (strcmp(ptr, "unsubscribe") == 0) {
				ptr = strtok(NULL, delim);
				if ((ptr != NULL && strlen(ptr) < 50)) {
					/* Verificam daca comanda are junk la sfarsit*/
					check = strtok(NULL, delim);
					if (check == NULL) {
						/* Creare mesaj de 'unsubscribe'*/
						memset(send_buffer, 0 , BUFLEN);
						sprintf(send_buffer, "%s unsubscribe %s ", argv[1], ptr);
						/* Trimitere mesaj*/
						ret = send(sockfd, send_buffer, strlen(send_buffer), 0);
						DIE(ret < 0, "send");
						/* Afisare feedback*/
						printf("Sending command:%s\n", send_buffer + strlen(argv[1]));
					} else {
						printf("Invalid command: unsubscribe %s %s\n", ptr, check);
					}
				} else {
					/* Comanda invalida*/
					printf("Topic <%s> does not exist\n", ptr);
				}
			} else if (strcmp(ptr, "EXIT") == 0) {
				/* Verificam daca comanda are junk la sfarsit*/
				check = strtok(NULL, delim);
				if (check == NULL) {
					/* Comanda "EXIT" inchide subscriberul */
					/* Creez mesajul */
					memset(send_buffer, 0 , BUFLEN);
					sprintf(send_buffer, "%s EXIT ", argv[1]);
					/* Trimit comanda */
					ret = send(sockfd, send_buffer, strlen(send_buffer), 0);
					DIE(ret < 0, "send");
					/* Afisez feedback */
					printf("Sending command: EXIT\n");
					break;
				} else {
					printf("Invalid command: %s\n", ptr);
				}
			} else {
				/* Comanda invalida*/
				printf("The command does not exist\n");
				printf("List of commands:\n");
				printf("    ->subscribe <Topic> <SF>\n");
				printf("    ->unsubscribe <Topic>\n");
				printf("    ->EXIT\n");
			}
		} else if (FD_ISSET(sockfd, &tmp_fds)) {
			//Raspuns server
			memset(buffer, 0, BUFLEN);
			ret = recv(sockfd, buffer, sizeof(buffer), 0);
			DIE(ret < 0, "recv");

			if (ret == 0 || strncmp(buffer, "EXIT", 4) == 0) {
				/* Am primit comanda "EXIT" de la server, inchid clientul*/
				printf("Received command: <EXIT>\n");
				break;
			} else {
				printf("%s", buffer);
			}
		}
	}

	close(sockfd);
	return 0;
}
