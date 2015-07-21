#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
typedef bool Activity;
#define active true
#define inactive false
#define DEFAULT_ACTIVITY false
#define ACTIVE_STRING "active"
#define INACTIVE_STRING "inactive"
#define MAX_NUMBER_OF_CONNECTION 255
typedef unsigned char ID;
#define SOCK_FLG 0
#define MESSAGE_SIZE 2
#define SOCK_PROTO 0
#define PORT 3000
#define DISPLAY_PORT 3001
#define CONNECTION_FINISHED 0
#define READ_FAILED -1
#define SOCKET_ERROR -1
#define ACTION_LIST 0
#define FIRST_ID 1
#define NOT_CONNECTED -1
#define MAX_LIST_LENGTH MAX_NUMBER_OF_CONNECTION * 2 + 1
typedef int Socket;
typedef int IOresult;
//structure to be passed to the thread
struct ClientIdPair {
	Socket client;
	ID id;
};

Socket clients[MAX_NUMBER_OF_CONNECTION];
Activity robotsActivity[MAX_NUMBER_OF_CONNECTION];
//unsigned int numberOfConnection = 0;
ID nextId = FIRST_ID;

//one thread per connection. Application need more speed than memory efficiency
pthread_t threads[MAX_NUMBER_OF_CONNECTION];
pthread_t threadsDisplay[MAX_NUMBER_OF_CONNECTION];
void sendMessage(Socket client, char* message) {
	write(client, message,strlen(message));
}

ID getNextId() {
	static ID next = FIRST_ID;
	return next++;
}

char* activityToString(Activity activity) {
	if (activity == inactive)
		return INACTIVE_STRING;
	return ACTIVE_STRING;
}

void setEtat(ID id, Activity activity) {
	printf("%d become %s\n", id, activityToString(activity));
	robotsActivity[id] = activity;
}


void sendState(Socket socket, ID id) {
	char message[2];
	printf("sending %d -> %d",id,robotsActivity[id]);
	message[0] = id;
	message[1] = robotsActivity[id];
	write(socket, message, 2);
}

void sendList(Socket connection) {
	char message[MAX_LIST_LENGTH];
	for (int i = 0; i < MAX_LIST_LENGTH;++i)
		message[i] = 0;
	printf("sending %u states\n",nextId - 1);
	for (ID id = FIRST_ID; id <= nextId - 1; ++id) {
		message[((id - 1) * 2) + 1] = id;
		message[((id - 1) * 2) + 2] = robotsActivity[id];
	}
	write(connection, message, MAX_LIST_LENGTH);
}

void handle_message(ID id, Activity activity) {
	setEtat(id,activity);
}

void sendId(Socket client, ID id) {
	char message[1];
	message[0] = id;
	write(client, message, 1);
}
/**
 * @brief handle_client handle a robot during his connection
 * @param passedClient the socket member
 */
void* handle_robot(void* passedClient) {
	struct ClientIdPair clientIdPaire = *(struct ClientIdPair*) passedClient;
	sendId(clientIdPaire.client, clientIdPaire.id);
	char message[MESSAGE_SIZE];
	Activity activity;
	while (true) {
		IOresult result = read(clientIdPaire.client,message,MESSAGE_SIZE);
		if (result == READ_FAILED)
			break;
		if (result == READ_FAILED || result == CONNECTION_FINISHED)
			break;
		printf("message received : %s\n",message);
		activity = message[1];
		printf("translated to : id = %u; tpm = %u -> activity = %u;\n",
				message[0], message[1], activity);
		setEtat(clientIdPaire.id,activity);
	}
	close(clientIdPaire.client);
	return 0;
}
void* handle_display(void* passedClient) {
	struct ClientIdPair clientIdPaire = *(struct ClientIdPair*) passedClient;
	//sendId(clientIdPaire.client, clientIdPaire.id);
	char message[MESSAGE_SIZE];
	unsigned char action;
	while (true) {
		IOresult result = read(clientIdPaire.client,message,MESSAGE_SIZE);
		if(result == READ_FAILED)
			break;
		if(result == READ_FAILED || result == CONNECTION_FINISHED)
			break;
		printf("message received : %s\n",message);

		action = message[0];
		printf("translated to %d\n",action);
		if (action == ACTION_LIST) {
			printf("send list\n");
			sendList(clientIdPaire.client);
		} else {
			printf("send state");
			sendState(clientIdPaire.client, action);
		}
	}
	close(clientIdPaire.client);
	return 0;
}
void waitConnection(Socket server,void (*toDo(void*)),ID id, pthread_t thread) {
	struct sockaddr_in clientAdress;
	struct ClientIdPair clientIdPair;
	//ID i = 0;

	unsigned int size = sizeof(clientAdress);
	printf("waiting for connection\n");
	clientIdPair.client = accept(server, (struct sockaddr*)&clientAdress, &size);
	//check for error
	if (clientIdPair.client == SOCKET_ERROR){
		perror("accept");
		exit(1);
	}
	clientIdPair.id = id;
	pthread_create(&thread,NULL,toDo,(void*)&clientIdPair);
	//a faire dans un nouveau thread
	printf("connected : socket %d\n",clientIdPair.client);
	//handle_client((void*)&client);
}

Socket prepareServer(int port) {
	struct sockaddr_in serverAdress;
	Socket server;

	printf("starting server\n");
	printf("preparing taken adress\n");
	bzero((char *)&serverAdress, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	serverAdress.sin_addr.s_addr = INADDR_ANY;
	serverAdress.sin_port = htons(port);

	server = socket(AF_INET,SOCK_STREAM, SOCK_PROTO);

	if (server == -1) {
		perror("erreur lors de la creation du serveur\n");
		return server;
	}
	printf("bind server to port %d\n", port);
	bind(server,(struct sockaddr*)&serverAdress,sizeof(serverAdress));
	printf("sarting to listen\n");
	listen(server,MAX_NUMBER_OF_CONNECTION);
	printf("server started\n");
	return server;
}

void waitConnectionsGlobal(Socket server, void* (*toDO)(void*),pthread_t threads[]) {
	for(nextId = FIRST_ID; nextId < MAX_NUMBER_OF_CONNECTION; ++nextId)
		waitConnection(server, toDO, nextId,threads[nextId]);
}

void waitConnections(Socket server, void* (*toDO)(void*),pthread_t threads[]) {
	for(ID id = FIRST_ID; id < MAX_NUMBER_OF_CONNECTION; ++id)
		waitConnection(server, toDO, nextId,threads[nextId]);
}


void closeServer(Socket server) {
	shutdown(server,SHUT_RDWR);// clos en RW
	close(server);
}

void runServer() {
	Socket server = prepareServer(PORT);
	waitConnectionsGlobal(server, handle_robot, threads);
	closeServer(server);
}
void* runDisplayServer(void* t) {
	Socket server = prepareServer(DISPLAY_PORT);
	waitConnections(server, handle_display, threadsDisplay);
	closeServer(server);
	return 0;
}

int main(void)
{
	pthread_t displayThread;
	pthread_create(&displayThread, NULL, runDisplayServer, (void*)NULL);

	runServer();
	return 0;
}
