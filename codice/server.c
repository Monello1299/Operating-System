#include "lib.h"

#define PENDING 100


int fd[PENDING];													//mantiene tutti i socket client descriptor
int busy[PENDING];													//tiene traccia dei client che stanno occupati a rispondere a una richiesta di chat
int connessione_richiesta[PENDING];									//tiene traccia se un client propone di chattare
void *identificazione(void *argument);								//identifica il client
void client_online(void *argument);									//stampa i client disponibili online
int cerca_client(int, char *, int);									//cerca il client selezionato
void comunicazione(int client1, long client2, long i);				//mette in comunicazione i due client



struct tabella{
	char *nome_client;
	char *indirizzo_ip;
	int disponibile;		//-1 non attivo, 1 disponibile, 0 occupato
	pthread_t id_thread;
};

struct value{
	int dsc;
	int numero;
	pthread_t testa;
};


struct tabella **tabella_thread;
pthread_mutex_t *sem;
int passaggio[3];
int entry, exit_client;
char **risposta_connessione_richiesta;


void client_exit_attesa(){
	pthread_exit(NULL);
}

void dealloca_memoria(int i){
	
	pthread_mutex_lock(sem);
	free(tabella_thread[i]->nome_client);
	free(tabella_thread[i]->indirizzo_ip);
	tabella_thread[i]->disponibile = -1;
	fd[i] = -1;
	busy[i] = -1;
	connessione_richiesta[i] = 0;
	risposta_connessione_richiesta[i] = (char *)malloc(MAX_READER);
	sprintf(risposta_connessione_richiesta[i], "%s", "quit client");
	pthread_mutex_unlock(sem);
	
	pthread_exit(NULL);
}


void *listen_client(void *argument){
	struct value *head_listen = (struct value *) argument;
	char *ric_attesa;
	int cl, id_l = head_listen->numero;
	
	
	while(cl >= 0){
		ric_attesa = (char *)malloc(MAX_READER);
		cl = recv(head_listen->dsc, ric_attesa, MAX_READER, 0);
		ric_attesa[cl] = '\0';
	
		exit_client = 1;
		while(entry == 0);
		
		if(cl == 0){
 			pthread_kill(head_listen->testa, SIGUSR1);
			printf("Client[%d] si è disconnesso dal server\n", id_l);
			
			free(head_listen);
			free(ric_attesa);
			dealloca_memoria(id_l);
			
			return((void *)0);
		}
		exit_client = 0;
	}
	
	pthread_exit(NULL);
}



int main(){
    struct sockaddr_in server, client[PENDING];
    int sd;
	long id;
    socklen_t addrlen;
	pthread_t thread[PENDING];

	printf("\n\t\t\e[91m\e[1m@@@@@@   @@@@@@   @@@@@@   @@    @@   @@@@@@   @@@@@@\e[22m\e[39m\n");
	printf("\t\t\e[91m\e[1m@@       @@       @@  @@   @@    @@   @@       @@  @@\e[22m\e[39m\n");
	printf("\t\t\e[91m\e[1m@@@@@@   @@@@@@   @@@@@@   @@    @@   @@@@@@   @@@@@@\e[22m\e[39m\n");
	printf("\t\t\e[91m\e[1m    @@   @@       @@  @@    @@  @@    @@       @@  @@\e[22m\e[39m\n");
	printf("\t\t\e[91m\e[1m@@@@@@   @@@@@@   @@   @@     @@      @@@@@@   @@   @@\e[22m\e[39m\n");
    
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("Errore init socket\n");
        exit(EXIT_FAILURE);
    }
    
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
    

    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))<0){
    	printf("Error setsockopt(SO_REUSEADDR)");
    	printf("Error code: %d\n", errno);
    	exit(EXIT_FAILURE);
    } 
	
	
    if( (bind(sd, (struct sockaddr *)&server, sizeof(server)))== -1){
        printf("Error binding\n");
        printf("Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }
    

    if( listen(sd, PENDING) == -1){
		printf("error listen\n");
		printf("error code %d\n", errno);
		exit(EXIT_FAILURE);
	}
	
	printf("\nServer in ascolto sulla porta %d\n", PORT);
	

	addrlen = sizeof(client[0]);
	sem = malloc(sizeof(pthread_mutex_t *));
	tabella_thread = malloc(PENDING*sizeof(struct tabella *));
	risposta_connessione_richiesta = (char **)malloc(PENDING*sizeof(char*));
	
	for(int x=0; x<PENDING; x++){
		tabella_thread[x] = malloc(sizeof(struct tabella *));
		fd[x] = -1;
		busy[x] = -1;
		connessione_richiesta[x] = 0;
	}

	id = 0;
	int ct;
	while(1){
		while(1){
			pthread_mutex_lock(sem);
			ct = fd[id];
			pthread_mutex_unlock(sem);
			
			if(ct == -1){					//trovato un posto per un client
				break;
			}
			id = (id+1)%PENDING;
		}
		

		while((ct = accept(sd, (struct sockaddr *)&client[id], &addrlen)) == -1 );
		
		pthread_mutex_lock(sem);
		fd[id] = ct;
		tabella_thread[id]->indirizzo_ip = (char *)malloc(strlen(inet_ntoa(client[id].sin_addr)));
		strcpy(tabella_thread[id]->indirizzo_ip, inet_ntoa(client[id].sin_addr));
		printf("\nConnesione aperta sul server\n");
		printf("Ip Client[%ld]: %s\n\n", id, tabella_thread[id]->indirizzo_ip);
		pthread_mutex_unlock(sem);
		
		pthread_create(&thread[id], NULL, (void *)identificazione, (void *)id);
		id = (id+1)%PENDING;
	}

	printf("Chiusura della connessione\n");

    close(sd);
    return 0;
}

void *identificazione(void *argument){
	long i = (long) argument;					//indice del thread
	int c;
	
	char *nome;
	nome = (char *)malloc(MAX_READER);
	c = recv(fd[i], nome, MAX_READER, 0);
	if(c == 0){
		free(nome);
		fd[i] = -1;
		pthread_exit(NULL);
	}
	nome[c] = '\0';
	
	pthread_mutex_lock(sem);
	tabella_thread[i]->nome_client = (char *)malloc(MAX_READER);
	strcpy(tabella_thread[i]->nome_client, nome);
	tabella_thread[i]->nome_client[strlen(tabella_thread[i]->nome_client)] = '\0';
	tabella_thread[i]->disponibile = 1;
	tabella_thread[i]->id_thread = pthread_self();
	pthread_mutex_unlock(sem);
	
	free(nome);
	
	
	client_online((void *)i);
	return((void *)0);
}

void client_online(void *argument){
	long i = (long)argument;
	int c, dsc, count;
	
	pthread_mutex_lock(sem);
	dsc = fd[i];
	pthread_mutex_unlock(sem);
	
	
	char *stampa;
	restart_list:
		nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
	
	send(dsc, "\n   --- Elenco delle persone disponibili ---", strlen("\n   --- Elenco delle persone disponibili ---"), 0);

	count = 0;
	for(int j=0; j<PENDING; j++){
		count++;
		pthread_mutex_lock(sem);
		c = tabella_thread[j]->disponibile;
		pthread_mutex_unlock(sem);
		
		if(c == 1){
			count--;
			stampa = (char *)malloc(MAX_READER);
			
			pthread_mutex_lock(sem);
				sprintf(stampa, " - \e[1m%-15s\e[22mIndirizzo IP:  \e[1m%s\e[22m", tabella_thread[j]->nome_client, tabella_thread[j]->indirizzo_ip);
			pthread_mutex_unlock(sem);
			
			nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);
			send(dsc, stampa, strlen(stampa), 0);
			
			free(stampa);
		}
	}
	
	pthread_mutex_lock(sem);
	c = tabella_thread[1]->disponibile;
	pthread_mutex_unlock(sem);
	
	if(count == PENDING-1 && c == -1){
 		signal(SIGUSR1, client_exit_attesa);
		struct value *head;
		head = malloc(sizeof(struct value *));
		pthread_t thread_ric_attesa;
		
		head->dsc = dsc;
		head->numero = (int) i;
		head->testa = pthread_self();

  		pthread_create(&thread_ric_attesa, NULL, (void *)listen_client, (void *)head);
		
		int x = 0;
		exit_client = 0;
		while(1){
			if(i!=x){
				
				entry = 0;
				
				pthread_mutex_lock(sem);
				c = tabella_thread[x]->disponibile;
				pthread_mutex_unlock(sem);
				
				if(c == 1){
  					pthread_kill(thread_ric_attesa, SIGUSR1);
					free(head);
					goto restart_list;
				}
				
				entry = 1;
				while(exit_client == 1);
			}
			x = (x+1)%PENDING;
		}
	}
	
	char *seleziona;
	while(1){
		nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
		send(dsc, "\n   --- Digita il \e[1mnome\e[22m della persona da contattare ---", strlen("\n   --- Digita il \e[1mnome\e[22m della persona da contattare ---"), 0);
		nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
		send(dsc, "   *** Digita '\e[1maggiorna\e[22m' per aggiornare la lista  ***", strlen("   *** Digita '\e[1maggiorna\e[22m' per aggiornare la lista  ***"), 0);
		
		start_from_here:
			seleziona = (char *)malloc(MAX_READER);
		
		c = recv(dsc, seleziona, MAX_READER, 0);
		seleziona[c] = '\0';
		
		if(c == 0 || strcmp(seleziona, "quit") == 0){
			printf("Client[%ld] si è disconnesso dal server\n", i);
			free(seleziona);
			nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);
			
			dealloca_memoria(i);
			
			pthread_exit(NULL);
		}
		
		pthread_mutex_lock(sem);
		
		if(connessione_richiesta[i] == 1){
			
			risposta_connessione_richiesta[i] = (char *)malloc(MAX_READER);
			sprintf(risposta_connessione_richiesta[i], "%s", seleziona);
			risposta_connessione_richiesta[i][strlen(risposta_connessione_richiesta[i])] = '\0';
			connessione_richiesta[i] = 0;
			
			pthread_mutex_unlock(sem);
			
			while(1){
				
				if(strcmp(seleziona, "Si") == 0 || strcmp(seleziona, "si") == 0 || strcmp(seleziona, "SI") == 0 || strcmp(seleziona, "sI") == 0 || strcmp(seleziona, "yes") == 0){
					free(seleziona);
					while(1){
						pthread_mutex_lock(sem);
						c = connessione_richiesta[i];
						pthread_mutex_unlock(sem);
						
						nanosleep((const struct timespec[]){{0, 50000000L}}, NULL);
												
						if(c == 2) comunicazione(-1, -1, -1);
					}
					goto start_from_here;
				}
				
				if(strcmp(seleziona, "No") == 0 || strcmp(seleziona, "no") == 0 || strcmp(seleziona, "NO") == 0){
					free(seleziona);
					goto restart_list;
				}
				
				send(dsc, "   --- Devi rispondere '\e[1mSi\e[22m' o '\e[1mNo\e[22m'! ---", strlen("   --- Devi rispondere '\e[1mSi\e[22m' o '\e[1mNo\e[22m'! ---"), 0);
				free(seleziona);
				
				seleziona = (char *)malloc(MAX_READER);
				c = recv(dsc, seleziona, MAX_READER, 0);
				seleziona[c] = '\0';
				
				pthread_mutex_lock(sem);
				risposta_connessione_richiesta[i] = (char *)malloc(MAX_READER);
				sprintf(risposta_connessione_richiesta[i], "%s", seleziona);
				risposta_connessione_richiesta[i][strlen(risposta_connessione_richiesta[i])] = '\0';
				risposta_connessione_richiesta[strlen(risposta_connessione_richiesta[i])] = '\0';
				connessione_richiesta[i] = 0;
				pthread_mutex_unlock(sem);
			}
				
			goto start_from_here;
		}
		
		c = busy[i];
		pthread_mutex_unlock(sem);
		
		
		if(strcmp(seleziona, "aggiorna") == 0){
			free(seleziona);
			goto restart_list;
		}
		
		if((count = cerca_client(i, seleziona, dsc)) && c == 0){
			free(seleziona);
			if(count == 0) goto restart_list;
		}
	}
}


int cerca_client(int i, char *seleziona, int descpritor){
	char *risposta, **cerca;
	int dsc[2], c;
	
	dsc[0] = descpritor;
	
	cerca = (char **)malloc(3*sizeof(char *));
	
	for(long j=0; j<PENDING; j++){
		
		if(i == j){
			pthread_mutex_lock(sem);
			if(tabella_thread[j]->nome_client == NULL){
				pthread_mutex_unlock(sem);
				continue;
			}
			
			if(strcmp(tabella_thread[j]->nome_client, seleziona) == 0){
				if(busy[j] == 1 || tabella_thread[j]->disponibile == 0){
					send(dsc[0], "\n   --- \e[1mUtente occupato in un'altra richiesta!\e[22m ---\a", strlen("\n   --- \e[1mUtente occupato in un'altra richiesta!\e[22m ---\a"), 0);
					pthread_mutex_unlock(sem);
					free(cerca);
					return 1;
				}
				send(dsc[0], "\n   --- Non è possibile parlare con \e[1mse stessi :)\e[22m ---", strlen("\n   --- Non è possibile parlare con \e[1mse stessi :)\e[22m ---"), 0);
				free(cerca);
				pthread_mutex_unlock(sem);
			
				return 1;
			}
			
			pthread_mutex_unlock(sem);
		}
			
		if(i!=j){
			
			pthread_mutex_lock(sem);
			if(tabella_thread[j]->nome_client == NULL){
				pthread_mutex_unlock(sem);
				continue;
			}
			
			if(strcmp(tabella_thread[j]->nome_client, seleziona) == 0){
				if(busy[j] == 1 || tabella_thread[j]->disponibile == 0){
					send(dsc[0], "\n   --- \e[1mUtente occupato in un'altra richiesta!\e[22m ---\a", strlen("\n   --- \e[1mUtente occupato in un'altra richiesta!\e[22m ---\a"), 0);
					pthread_mutex_unlock(sem);
					free(cerca);
					return 1;
				}
			}
			cerca[0] = (char *)malloc(MAX_READER);
			strcpy(cerca[0], tabella_thread[j]->nome_client);
			pthread_mutex_unlock(sem);
			
			cerca[0][strlen(cerca[0])] = '\0';
			
			if(strcmp(seleziona, cerca[0]) == 0){
				free(seleziona);
				free(cerca[0]);
				
				send(dsc[0], "   --- \e[1m\e[5mInvio della richiesta, attendere...\e[25m\e[22m ---\a", strlen("\t--- \e[1m\e[5mInvio della richiesta, attendere...\e[25m\e[22m ---\a"), 0);
				
				pthread_mutex_lock(sem);
				dsc[1] = fd[j];
				busy[j] = 1;
				pthread_mutex_unlock(sem);
				
				connessione_richiesta[j] = 0;
				cerca[1] = (char*)malloc(MAX_READER);
				
				pthread_mutex_lock(sem);
				sprintf(cerca[1], "\n\t--- \e[1m\e[5mHai una richiesta di chat da %s\e[25m\e[22m ---\a", tabella_thread[i]->nome_client);
				pthread_mutex_unlock(sem);
				
				send(dsc[1], cerca[1], strlen(cerca[1]), 0);
				nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
				free(cerca[1]);
send(dsc[1], "  --- Digitare '\e[1mSi\e[22m' per accettare altrimenti digitare '\e[1mNo\e[22m' ---", strlen("  --- Digitare '\e[1mSi\e[22m' per accettare altrimenti digitare'\e[1mNo\e[22m'---"), 0);


				while(1){
					
					pthread_mutex_lock(sem);
					connessione_richiesta[j] = 1;
					pthread_mutex_unlock(sem);
					
					while(1){
						pthread_mutex_lock(sem);
						c = connessione_richiesta[j];
						pthread_mutex_unlock(sem);
						
						nanosleep((const struct timespec[]){{0, 5000000L}}, NULL);
						
						if(c == 0) break;
					}
					
					risposta = (char *)malloc(MAX_READER);
										
					pthread_mutex_lock(sem);
					sprintf(risposta, "%s", risposta_connessione_richiesta[j]);
					free(risposta_connessione_richiesta[j]);
					pthread_mutex_unlock(sem);
					
					risposta[strlen(risposta)] = '\0';					
					
					if(strcmp(risposta, "quit client") == 0){
						send(dsc[0], "\n   --- L'utente si è appena \e[1mscollegato\e[22m dal server :( ---", strlen("   --- L'utente si è appena \e[1mscollegato\e[22m dal server :( ---"), 0);
						free(risposta);
						free(cerca);
						return 0;
					}
					
					
					if(strcmp(risposta, "Si") == 0 || strcmp(risposta, "si") == 0 || strcmp(risposta, "SI") == 0 || strcmp(risposta, "sI") == 0 || strcmp(risposta, "yes") == 0){
						free(risposta);
						
						passaggio[0] = dsc[1];
						passaggio[1] = dsc[0];
						passaggio[2] = j;
						
						pthread_mutex_lock(sem);
						connessione_richiesta[j] = 2;			//via libera per entrare in funzione
						pthread_mutex_unlock(sem);
						
						
						cerca[1] = (char *)malloc(MAX_READER);
						send(dsc[0], "     ------------------------------------", strlen("     ------------------------------------"), 0);
						nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
						
						pthread_mutex_lock(sem);
						sprintf(cerca[1], "   --- \e[1m%s ha accettato la richiesta!\e[22m ---", tabella_thread[j]->nome_client);
						tabella_thread[i]->disponibile = 0;
						tabella_thread[j]->disponibile = 0;
						pthread_mutex_unlock(sem);
						
						send(dsc[1], "     ------------------------------------", strlen("     ------------------------------------"), 0);
						send(dsc[0], cerca[1], strlen(cerca[1]), 0);
						free(cerca[1]);
						nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
						
						send(dsc[1], "\n\t*** \e[91m\e[1mDigita per parlare!\e[22m\e[39m ***\n", strlen("\n\t*** \e[91m\e[1mDigita per parlare!\e[22m\e[39m ***\n"), 0);
						send(dsc[0], "\n\t*** \e[91m\e[1mDigita per parlare!\e[22m\e[39m ***\n", strlen("\n\t*** \e[91m\e[1mDigita per parlare!\e[22m\e[39m ***\n"), 0);
						
						pthread_mutex_lock(sem);
						busy[j] = 0;
						connessione_richiesta[j] = 2;			//via libera di sicurezza
						pthread_mutex_unlock(sem);
						
						free(cerca);

						comunicazione(dsc[0], dsc[1], i);
						pthread_exit(NULL);
					}
					
					if(strcmp(risposta, "No") == 0 || strcmp(risposta, "no") == 0 || strcmp(risposta, "NO") == 0){
						free(risposta);
						free(cerca);
						risposta = (char *) malloc(MAX_READER);
						
						pthread_mutex_lock(sem);
						busy[j] = 0;
						sprintf(risposta, "\n   --- \e[1m%s ha rifiutato la richiesta!\e[22m ---\a", tabella_thread[j]->nome_client);
						pthread_mutex_unlock(sem);
						
						send(dsc[0], risposta, strlen(risposta), 0);
						//free(risposta);
						
						return 0;
					}
					free(risposta);
				}
			}
			
			free(cerca[0]);
		}
	}
	send(dsc[0], "\n\t--- \e[1mNome non trovato!\e[22m ---\a", strlen("\n\t--- \e[1mNome non trovato!\e[22m ---\a\n"), 0);
	free(cerca);
	return 1;
}



void comunicazione(int client1, long client2, long i){
	int c;
	

	if(client1 == -1 && client2 == -1 && i == -1){
		client1 = passaggio[0];
		client2 = passaggio[1];
		i = passaggio[2];
	}
	
	char *messaggio, *uscita;
	uscita = (char *)malloc(MAX_READER);
	
	pthread_mutex_lock(sem);
	sprintf(uscita, "%s: quit", tabella_thread[i]->nome_client);
	pthread_mutex_unlock(sem);

	while(1){
		messaggio = (char *)malloc(MAX_READER);
		c = recv(client1, messaggio, MAX_READER, 0);
		messaggio[c] = '\0';
		
		if(strcmp(messaggio, "***exItnOw***") == 0){
			free(messaggio);
			free(uscita);
			
			pthread_mutex_lock(sem);
			tabella_thread[i]->disponibile = 1;
			pthread_mutex_unlock(sem);
			
			client_online((void *)i);
			pthread_exit(NULL);
		}
		
		if(strcmp(messaggio, uscita) == 0 || c == 0){
			send(client1, "\n     ------------------------------------", strlen("\n     ------------------------------------"), 0);
			send(client2, "\n     ------------------------------------", strlen("\n     ------------------------------------"), 0);
			free(messaggio);
			messaggio = (char *)malloc(MAX_READER);
			nanosleep((const struct timespec[]){{0, 500000L}}, NULL);	
			
			pthread_mutex_lock(sem);
			sprintf(messaggio, "\n\t\e[1m%s ha abbandonato la conversazione\e[22m\n\a", tabella_thread[i]->nome_client);
			pthread_mutex_unlock(sem);
			
			send(client2, messaggio, strlen(messaggio), 0);
			send(client1, messaggio, strlen(messaggio), 0);
			nanosleep((const struct timespec[]){{0, 500000L}}, NULL);
			send(client2, "***exItnOw***", strlen("***exItnOw***"), 0);
			free(messaggio);
			free(uscita);
			
			pthread_mutex_lock(sem);
			tabella_thread[i]->disponibile = 1;
			pthread_mutex_unlock(sem);
			
			client_online((void *)i);
			pthread_exit(NULL);
		}
		
		send(client2, messaggio, strlen(messaggio), 0);
		free(messaggio);
	}
	
	pthread_exit(NULL);
	
}
