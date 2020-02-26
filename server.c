#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <inttypes.h>

#include "helpers.h"

/* Folosita sa retin informatile transmise pe conexiunea UDP */
typedef struct node {
	char tip_data[11];
	char data[1501];
	char IP_cli_UDP[16];
	unsigned int Port_cli_UDP;
	struct node *next_info;
} Data_topic;

/* Structura in care retin topicul la care sa abonat un client*/
typedef struct topic{
	char title[51];
	int SF;
	Data_topic *info;
	struct topic *next_topic;
} Topics;

/* Lista cu clienti care au venit*/
typedef struct subscriber {
	char client_id[11];
	Topics *topics;
	int socket;
	struct subscriber *next_cli;
} Clients;

Clients *searchClientByID(Clients *list, char *client_id)
{
	Clients *cli = list;
	while (cli) {
		if (strcmp(cli->client_id, client_id) == 0)
			break;
		cli = cli->next_cli;
	}
	return cli;
}
int main(int argc, char *argv[])
{
	int sfd, sockfd, newsockfd, portno, clilen;
	char buffer[BUFLEN], title[51];
	char *ptr, *cli_id;
	struct sockaddr_in serv_addr, cli_addr;
	struct sockaddr_in my_sockaddr, from_station;
	int i, ret, insert_topic;

	/* Variabile in care retin clienti*/
	Clients *cli = NULL;
	Topics *topic = NULL;
	Data_topic *info = NULL;
	Clients *list_clients = NULL;

	if (argc < 2) {
		printf("Usage %s server_port\n", argv[0]);
		exit(0);
	}

	fd_set read_fds;	//multimea de citire folosita in select()
	fd_set tmp_fds;	//multime folosita temporar
	int fdmax;		//valoare maxima file descriptor din multimea read_fds
	/* Golim multimea de descriptori de citire (read_fds) si multimea tmp_fds */
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	/* Socketul UDP */
	sfd = socket(AF_INET, SOCK_DGRAM, 0); 
	DIE(sfd < 0, "socket");
	/* Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	portno = atoi(argv[1]);
	my_sockaddr.sin_family = AF_INET;
	my_sockaddr.sin_port = htons(portno);
	my_sockaddr.sin_addr.s_addr =inet_addr("127.0.0.1");
	memset(my_sockaddr.sin_zero, '\0', sizeof(my_sockaddr.sin_zero)); 
	/* Legare proprietati de socket */
	ret = bind(sfd,(struct sockaddr *)&my_sockaddr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind-UDP");
	int addr_len = sizeof(struct sockaddr);

	/* Socketul TCP */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	ret = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (void *)&ret, sizeof(ret));
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	// foloseste adresa IP a masinii
	serv_addr.sin_port = htons(portno);
	/* Legare proprietati de socket */
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind-TCP");

	/* Incepem sa ascultam pe port */
	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");
	/* adaugam noul file descriptor (socketul pe care se asculta conexiuni) */
	/* in multimea read_fds */
	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	FD_SET(sfd, &read_fds);
	fdmax = sockfd;
	// main loop
	while (1) {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				/* Comenzi de la tastatura*/
				if(i == 0) {
					memset(buffer, 0 , BUFLEN);
					scanf("%s", buffer);
					if(strcmp(buffer, "EXIT") == 0) {
						/* Se trimite mesaj de inchidere fiecarui socket*/
						/* Se inchide socketul*/
						for(i = 5; i <= fdmax; i++) {
							send(i, buffer, strlen(buffer), 0);
							close(i);
							FD_CLR(i, &read_fds);
						}
						/* Se inchid socketii de ascultare */
						close(sockfd);
						close(sfd);
						FD_CLR(sockfd, &read_fds);
						FD_CLR(sfd, &read_fds);
						exit(0);
					} else {
						printf("Invalid command: %s", buffer);
						printf("The only valid command is <EXIT>\n");
					}
				}
				/* Conexiune UDP*/
				if (i == sfd) {
					memset(buffer, 0 , BUFLEN);
					ret = recvfrom(i, buffer, sizeof(buffer), 0,
							(struct sockaddr *)&from_station, &addr_len);
					DIE(ret < 0, "recv-UDP");

					/*Am primit informatii despre un nou topic*/
					Data_topic *new_udp_info = malloc(sizeof(Data_topic));
					strcpy(new_udp_info->IP_cli_UDP, inet_ntoa(from_station.sin_addr));
					new_udp_info->Port_cli_UDP = ntohs(from_station.sin_port);
					new_udp_info->next_info = NULL;

					strcpy(title, buffer);
					if (buffer[50] == 3) {
						strcpy(new_udp_info->tip_data, "STRING");
						strcpy(new_udp_info->data, buffer + 51);
					}else if (buffer[50] == 2) {
						strcpy(new_udp_info->tip_data, "FLOAT");
						uint32_t var;
						uint8_t exp;
						memcpy(&var, buffer + 50 + sizeof(uint16_t), sizeof(uint32_t));
						memcpy(&exp, buffer + 50 + sizeof(uint16_t) + sizeof(uint32_t),
								sizeof(uint8_t));
						/* Calculam numarul de zecimal*/
						int pwr = 10;
						if (exp == 0) {
							pwr = 1;
						} else {
							while (exp > 1) {
								pwr = 10 * pwr;
								exp--;
							}
						}
						if (buffer[51] == 1)
							sprintf(new_udp_info->data,"-%.4f",
								(float)ntohl(var)/pwr);
						else
							sprintf(new_udp_info->data,"%.4f",
								(float)ntohl(var)/pwr);
					} else if (buffer[50] == 1) {
						strcpy(new_udp_info->tip_data, "SHORT_REAL");
						short var;
						memcpy(&var, buffer + 51, sizeof(uint16_t));
						sprintf(new_udp_info->data,"%u.%.2d", ntohs(var)/100, ntohs(var)%100);
					}else if (buffer[50] == 0) {
						strcpy(new_udp_info->tip_data, "INT");
						int var;
						memcpy(&var, buffer + 50 + sizeof(uint16_t), sizeof(uint32_t));
						if (buffer[51] == 1)
							sprintf(new_udp_info->data,"-%u", ntohl(var));
						else
							sprintf(new_udp_info->data,"%u", ntohl(var));
					}

					/* Trimitem mesajul tuturor clientilor conectati si abonati la topic*/
					/* Salvam mesajul pentru clientii care au SF = 1 si sunt deconectati*/
					cli = list_clients;
					while (cli) {
						if (cli->socket > 0) {
							/* Trimit mesajul*/
							topic = cli->topics;
							while (topic) {
								if (strncmp(topic->title, title, strlen(topic->title)) == 0) {
									/* Construim si trimitem mesajul*/
									memset(buffer, 0, BUFLEN);
									sprintf(buffer, "%s:%u - %s - %s - %s\n",
										new_udp_info->IP_cli_UDP,
										new_udp_info->Port_cli_UDP,
										topic->title,
										new_udp_info->tip_data,
										new_udp_info->data);

									ret = send(cli->socket, buffer, strlen(buffer), 0);
									DIE(ret < 0, "send");
								}
								topic = topic->next_topic;
							}
						} else {
							/* Salvez mesajul*/
							topic = cli->topics;
							while (topic) {
								if ((topic->SF == 1) &&
									(strncmp(topic->title, title, strlen(topic->title)) == 0)) {
									/* Cream un info pentru acel topic al clientului*/
									Data_topic *new_info = malloc(sizeof(Data_topic));
									strcpy(new_info->IP_cli_UDP, new_udp_info->IP_cli_UDP);
									new_info->Port_cli_UDP = new_udp_info->Port_cli_UDP;
									strcpy(new_info->tip_data, new_udp_info->tip_data);
									strcpy(new_info->data, new_udp_info->data);
									new_info->next_info = new_udp_info->next_info;
									
									if (!topic->info) {
										/* Prima intrare pe acest topic*/
										topic->info = new_info;
									} else {
										/* Urmatoarele intrari*/
										Data_topic *aux = topic->info;
										while (aux->next_info) {
											aux = aux->next_info;
										}
										aux->next_info = new_info;
									}
								}
								topic = topic->next_topic;
							}
						}
						cli = cli->next_cli;
					}
					free(new_udp_info);
				}
				/* Conexiune TCP*/
				if (i == sockfd && i != sfd) { 
					/* Am primit o conexiune noua, pe care o acceptam*/
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
					DIE(newsockfd < 0, "socket");
					ret = 1;
					setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY,
								(void *)&ret, sizeof(ret));

					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) {
						fdmax = newsockfd;
					}
					/* Primim Client_ID*/
					memset(buffer, 0, BUFLEN);
					ret = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(ret < 0, "socket");

					/* Primul Client*/
					if (!list_clients) {
						Clients *new_cli = malloc(sizeof(Clients));
						new_cli->next_cli = NULL;
						strcpy(new_cli->client_id, buffer);
						new_cli->socket = newsockfd;
						new_cli->topics = NULL;
						
						list_clients = new_cli;
						/* Afisam mesajul de conectare al unui client*/
						printf("New client (%s) connected from %s:%d.\n",
							new_cli->client_id,
							inet_ntoa(cli_addr.sin_addr),
							htons(cli_addr.sin_port));
						break;
					}
					/* Adaugam noul client in lista daca e prima data cand sa conectat*/
					cli = searchClientByID(list_clients, buffer);
					if (cli == NULL) {
						Clients *new_cli = malloc(sizeof(Clients));
						new_cli->next_cli = NULL;
						strcpy(new_cli->client_id, buffer);
						new_cli->socket = newsockfd;
						new_cli->topics = NULL;

						cli = list_clients;
						while (cli->next_cli != NULL) {
							cli = cli->next_cli;
						}
						cli->next_cli = new_cli;
						/* Afisam mesajul de conectare al unui client*/
						printf("New client (%s) connected from %s:%d.\n",
							new_cli->client_id,
							inet_ntoa(cli_addr.sin_addr),
							htons(cli_addr.sin_port));
					} else {
						/* Ii trimitem datele din topicuri pe care nu lea primit */
						printf("Client (%s) connected from %s:%d.\n",
							cli->client_id,
							inet_ntoa(cli_addr.sin_addr),
							htons(cli_addr.sin_port));
						topic = cli->topics;
						cli->socket = newsockfd;
						while (topic) {
							if (topic->SF == 1) {
								info = topic->info;
								while (info) {
									/* Construim si trimitem mesajul*/
									memset(buffer, 0, BUFLEN);
									sprintf(buffer, "%s:%u - %s - %s - %s\n",
										info->IP_cli_UDP,
										info->Port_cli_UDP,
										topic->title,
										info->tip_data,
										info->data);
									ret = send(cli->socket, buffer, strlen(buffer), 0);
									DIE(ret < 0, "send");

									/* Stergem data pe care am trimiso*/
									topic->info = info->next_info;
									free(info);
									info = topic->info;
								}
							}
							topic = topic->next_topic;
						}
					}
				} else if (i != sfd && i != 0) {
					/* Am primit date pe unul din socketii*/
					memset(buffer, 0, BUFLEN);
					ret = recv(i, buffer, sizeof(buffer), 0);
					DIE(ret < 0, "socket");

					cli_id = strtok(buffer, " ");
					ptr = strtok(NULL, " ");
					if ((ret == 0) || (strncmp(ptr, "EXIT", 4) == 0)) {
						/* Conexiunea s-a inchis*/
						/* Caut client_id pentru acel socket*/
						cli = list_clients;
						while (cli) {
							if (cli->socket == i)
								break;
							cli = cli->next_cli;
						}
						printf("Client %s disconnected.\n", cli->client_id);
						cli->socket = -1;//clientul e offline
						close(i);

						/* Scoatem din multimea de citire socketul*/
						FD_CLR(i, &read_fds);
						break;
					} else if (strncmp(ptr, "subscribe", 9) == 0) {
						/* Cautam clientul pentru care am primit subscribe*/
						cli = searchClientByID(list_clients, cli_id);
						if (cli == NULL) {
							printf("Somenthing went wrong, the client_id is not recognized.\n");
							break;
						}
						/* Adaugam topicul la client*/
						Topics *new_topic = malloc(sizeof(Topics));
						ptr = strtok(NULL, " "); //scoatem titlu
						strcpy(new_topic->title, ptr);
						ptr = strtok(NULL, " "); //scoatem SF
						new_topic->SF = atoi(ptr);
						new_topic->next_topic = NULL;
						new_topic->info = NULL;
						/* Verificam sa vedem daca Clientul e inscris la vreun topic*/
						if (cli->topics) {
							/* Clientul e inscris la anumite topicuri*/
							/* Verificam daca sa mai inscris la topicul primit*/
							topic = cli->topics;
							insert_topic = 1;
							if (strcmp(topic->title, new_topic->title) == 0) {
								/* Verificare pentru lista de 1 element*/
								free(new_topic);
								insert_topic = 0;
							}
							while (topic->next_topic) {
								if (strcmp(topic->next_topic->title, new_topic->title) == 0) {
									/* Clientul e deja inscris la acest topic*/
									//topic->SF = new_topic->SF;
									free(new_topic);
									insert_topic = 0;
								}
								topic = topic->next_topic;
							}
							if (insert_topic) {
								/* Clientul nu era deja inscris*/
								topic->next_topic = new_topic;
							}
						} else {
							cli->topics = new_topic;
						}
				 	} else if (strncmp(ptr, "unsubscribe", 11) == 0) {
						/* Cautam clientul pentru care am primit unsubscribe*/
						cli = searchClientByID(list_clients, cli_id);
						if (cli == NULL) {
							printf("Somenthing went wrong, the client_id is not recognized.\n");
							break;
						}
						/* Cautam topicul la care vrem sa dam unsubscribe, sal scoatem din list*/
						/* Daca acesta nu exista nu facem nimic*/
						ptr = strtok(NULL, " ");
						if (cli->topics) {
							topic = cli->topics;
							if (strcmp(topic->title, ptr) == 0) {
								/* Verificam primul element din lista*/
								Topics *aux = topic;
								cli->topics = topic->next_topic;
								free(aux);
							} else {
								/* Verificam restul elementelor din lista*/
								while (topic->next_topic) {
									if (strcmp(topic->next_topic->title, ptr) == 0) {
										Topics *aux = topic->next_topic;
										topic->next_topic = aux->next_topic;
										free(aux);
										break;
									}
									topic = topic->next_topic;
								}
							}
						}
					}
				}
			}
		}
	}

	/*Eliberare memorie*/
	Clients *aux_cli;
	cli = list_clients;
	while (cli) {
		Topics *aux_topics;
		topic = cli->topics;
		while (topic) {
			Data_topic *aux_info;
			info = topic->info;
			while (info) {
				aux_info = info;
				info = info->next_info;
				free(aux_info);
			}
			aux_topics = topic;
			topic = topic->next_topic;
			free(aux_topics);
		}
		aux_cli = cli;
		cli = cli->next_cli;
		free(aux_cli);
	}
	return 0;
}
