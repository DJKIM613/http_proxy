#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <set>
#include <netdb.h>


using namespace std;

#define BUF_SIZE 1024
#define MESSAGE_SIZE 3072
#define MAX_CLIENT_NUM 10

void *request_handler(void* arg);
void send_data(int clnt_sock, char *file_name);
void send_400error(int clnt_sock);
void send_404error(int clnt_sock);
void print_error(char *message);
int hostname_to_ip(char *hostname, char *ip);

set<int> used_socket, unused_socket;
int sock_number[MAX_CLIENT_NUM];
bool is_b_option;
int main(int argc, char *argv[]) {
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_size;
	char buf[BUF_SIZE];
	pthread_t t_id;
	if (argc != 2)  print_error("http_proxy <port>");

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));
	if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1) print_error("bind error\n");

	if (listen(serv_sock, 20) == -1) print_error("listen error\n");

	for(int i= 0 ; i < MAX_CLIENT_NUM ; i++) unused_socket.insert(i);

	while (1) {
		clnt_adr_size = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, (socklen_t *)(&clnt_adr_size));
		if(unused_socket.size() == 0){
			send(clnt_sock, "client_full", strlen("client_full"), 0);
			close(clnt_sock);
			continue;
		}
		int index = *unused_socket.begin();
		sock_number[index] = clnt_sock;
		used_socket.insert(index);
		unused_socket.erase(index);

		pthread_create(&t_id, NULL, request_handler, &index);
		pthread_detach(t_id);
	}

	close(serv_sock);
	return 0;
}

void *request_handler(void *arg) {
    int index = *((int *)arg);
	int clnt_sock = sock_number[index];
    char message[MESSAGE_SIZE], URL[200], server_ip[20];
    memset(message, 0, sizeof(message));    memset(URL,0, sizeof(URL));
    
	printf("-----------------------------------\n");
	printf("connect_success!! socket index : %d client number %d\n", clnt_sock, used_socket.size());
	printf("-----------------------------------\n");

    recv(clnt_sock, message, sizeof(message), 0);
    printf("client message : \n%s\n", message);
    
    char *start_pos = strstr(message, "Host: ");   start_pos += strlen("Host: ");
    char *end_pos = strstr(start_pos, "\x0d\x0a");

    strncpy(URL, start_pos, end_pos - start_pos);
    hostname_to_ip(URL, server_ip);

    printf("URL : %s\n", URL);
    printf("server_ip : %s\n", server_ip);
    
    int serv_sock;
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);
	serv_addr.sin_port = htons(atoi("80"));
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    if (serv_sock == -1) print_error("socket error()\n");
    if (connect(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) print_error("connect() error!");
	printf("-----------------------------------\n");
	printf("connect_success!!\n");
	printf("-----------------------------------\n");

	send(serv_sock, message, strlen(message), 0);   memset(message, 0, sizeof(message));
    recv(serv_sock, message, sizeof(message), 0);   printf("server message : \n%s\n", message);
    send(clnt_sock, message, strlen(message), 0);

	used_socket.erase(index);
	unused_socket.insert(index);
	close(clnt_sock);   close(serv_sock);
}

void print_error(char *message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

int hostname_to_ip(char *hostname, char *ip){
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if((he = gethostbyname(hostname)) == NULL){
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i= 0 ; addr_list[i] != NULL ; i++){
        strcpy(ip, inet_ntoa(*addr_list[i]));
        return 0;
    }
    return 1;
}