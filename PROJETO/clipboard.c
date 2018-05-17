#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "clipboard.h"
#include "threads.h"
#include "regions.h"

int main(int argc, char **argv){		
	int server_fd_recv;
	client_socket *CS_un;
	client_socket *CS_in[2];

//INITIALIZE LOCAL CLIPBOARD
	//connected mode init
	if (argc == 4){
		if(strcmp(argv[1], "-c") == 0){
			//connect with clipboard "server"
			server_fd_recv = connected_clipboard_init(argv[2], argv[3]);
		}
		else{
		printf("invalid mode\n");
		exit(-2);
		}
	}
	//single mode init
	else if (argc == 1){
		regions_init(-1);
		server_fd_recv = redundant_server();
	}
	else{
		printf("invalid number of arguments\n");
		exit(-2);
	}

	//initializes the mutex//////////////////////////// 
	init_locks();

//LAUNCH CLIPBOARDS AND APPS SERVERS
	//launch the server to handle local(unix) apps connections
	pthread_t thread_id_un;
	if (pthread_create(&thread_id_un, NULL, server_init, (void *) UNIX) != 0){
		perror("pthread_create: ");
		exit(-1);
	}
	//launch the servers to handle remote(inet) clipboards (recv[0] and send[1]])
	pthread_t thread_id_in[2];
	if (pthread_create(&thread_id_in[0], NULL, server_init, (void *) INET_RECV) != 0){
		perror("pthread_create: ");
		exit(-1);
	}
	if (pthread_create(&thread_id_in[1], NULL, server_init, (void *) INET_SEND) != 0){
		perror("pthread_create: ");
		exit(-1);
	}

//HANDLE LOCAL APPS AND REMOTE COMMUNICATIONS
	//handle local apps (one thread per app)
	pthread_join(thread_id_un, (void **) &CS_un);printf("join\n");
	if (pthread_create(&thread_id_un, NULL, accept_clients, CS_un) != 0){
		perror("pthread_create: ");
		exit(-1);
	}
	pthread_join(thread_id_in[0], (void **) &CS_in[0]);
	pthread_join(thread_id_in[1], (void **) &CS_in[1]);
	//put the the second port in CS_in[0] to inform the clipboard "client"
	//where to perform the second connection
	CS_in[0]->port = CS_in[1]->port;
	//handle remote clipboards (one thread per clipboard) RECV
	if (pthread_create(&thread_id_in[0], NULL, accept_clients, CS_in[0]) != 0){
		perror("pthread_create: ");
		exit(-1);
	}
	//handle remote clipboards (one thread per clipboard) SEND
	if (pthread_create(&thread_id_in[1], NULL, accept_clients, CS_in[1]) != 0){
		perror("pthread_create: ");
		exit(-1);
	}

	//recieve updates from the clipboard "server"
	connection_handle(server_fd_recv, UP);
	server_fd_recv = redundant_server();
	connection_handle(server_fd_recv, UP);
	
 exit (-1);
}