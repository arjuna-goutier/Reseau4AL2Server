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
#define FIRST_ID 1
#define SOCK_FLG 0
#define MESSAGE_SIZE 2
#define SOCK_PROTO 0
#define PORT 3000
#define CONNECTION_FINISHED 0
#define READ_FAILED -1
#define SOCKET_ERROR -1
typedef int Socket;
typedef int IOresult;
//structure to be passed to the thread
struct ClientIdPair {
	Socket client;
	ID id;
};

Socket clients[MAX_NUMBER_OF_CONNECTION];
Activity robotsActivity[MAX_NUMBER_OF_CONNECTION];
unsigned int number_of_connection = 0;
Socket server;

//one thread per connection. Application need more speed than memory efficiency
pthread_t threads[MAX_NUMBER_OF_CONNECTION];
struct sockaddr_in serverAdress;

void sendMessage(Socket client, char* message) {
	write(client, message,strlen(message));
}

ID getNextId() {
	static ID next = FIRST_ID;
	return next++;
}

void disconect(ID id) {
	printf("%d disconnected\n",id);
}

void connection() {
	robotsActivity[getNextId()] = inactive;
	++number_of_connection;
}

char* activityToString(Activity activity) {
	if(activity == inactive)
		return INACTIVE_STRING;
	return ACTIVE_STRING;
}

void setEtat(ID id, Activity activity) {
	printf("%d become %s\n",id,activityToString(activity));
	robotsActivity[id] = activity;
}

void printEtat(ID id) {
	printf("%d%d",id,robotsActivity[id]);
}

void send_state(ID id) {
	printEtat(id);
}

void send_list() {
	for(unsigned int i = 0; i < number_of_connection;++i)
		printEtat((ID)i);
}

void handle_message(ID id, Activity activity) {
	setEtat(id,activity);
}
void receiveConnection() {

}

void sendId(Socket client, ID id) {
	char message[1];
	sprintf(message, "%c",id);
	sendMessage(client, message);
}
/**
 * @brief handle_client handle a robot during his connection
 * @param passedClient the socket member
 */
void* handle_client(void* passedClient) {
	struct ClientIdPair clientIdPaire = *(struct ClientIdPair*) passedClient;

	sendId(clientIdPaire.client, clientIdPaire.id);
	char message[MESSAGE_SIZE];
	Activity activity;
	while(true) {
		IOresult result = read(clientIdPaire.client,message,MESSAGE_SIZE);
		if(result == READ_FAILED)
			break;
		if(result == READ_FAILED || result == CONNECTION_FINISHED)
			break;
		printf("message received : %s\n",message);
		activity = message[1];
		printf("translated to : id = %u; tpm = %u -> activity = %u; test = %d;\n",
				message[0], message[1], activity,message[2]);
		setEtat(clientIdPaire.id,activity);
	}
	close(clientIdPaire.client);
	return 0;
}

void waitConnection() {
	struct sockaddr_in clientAdress;
	struct ClientIdPair clientIdPair;
	unsigned int size = sizeof(clientAdress);
	printf("waiting for connection\n");
	clientIdPair.client = accept(server, (struct sockaddr*)&clientAdress, &size);
	//check for error
	if(clientIdPair.client == SOCKET_ERROR) {
		perror("accept");
		exit(1);
	}
	clientIdPair.id = getNextId();
	pthread_create(&threads[clientIdPair.id],NULL,handle_client,(void*)&clientIdPair);
	//a faire dans un nouveau thread
	printf("connected : socket %d\n",clientIdPair.client);
	//handle_client((void*)&client);
}
void test() {
	void test2();
}

void prepareServer() {
	printf("starting server\n");
	printf("preparing taken adress\n");
	bzero((char *)&serverAdress, sizeof(serverAdress));
	serverAdress.sin_family = AF_INET;
	serverAdress.sin_addr.s_addr = INADDR_ANY; /* hôte local */
	serverAdress.sin_port = htons(PORT); /* PORT */

	server = socket(AF_INET,SOCK_STREAM, SOCK_PROTO); /* UDP */

	if (server == -1) {
		perror("erreur lors de la creation du serveur\n");
		return;
	}
	printf("bind server to port %d\n",PORT);
	bind(server,(struct sockaddr*)&serverAdress,sizeof(serverAdress));
	printf("sarting to listen\n");
	listen(server,MAX_NUMBER_OF_CONNECTION);
	printf("server started\n");
}

void waitConnections() {
	while(true)
		waitConnection();
}

void closeServer() {
	shutdown(server,SHUT_RDWR);// clos en RW
	close(server);
}

void runServer() {
	prepareServer();
	waitConnections();
	closeServer();
}

int main(void)
{
	runServer();
	return 0;
}
