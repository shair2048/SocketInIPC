#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/demo_socket"
#define MAX_CLIENTS 10

int client_sockets[MAX_CLIENTS];
int num_clients = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char message[1024];

    while (1) {
        ssize_t received = recv(client_socket, message, sizeof(message) - 1, 0);
        if (received <= 0) {
            perror("Receive failed");
            break;
        }

        message[received] = '\0';
        printf("Received message from client: %s\n", message);

        if (strcmp(message, "quit") == 0) {
            printf("Client is quitting.\n");
            break;
        } else {
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < num_clients; i++) {
                if (client_sockets[i] != client_socket) {
                    send(client_sockets[i], message, strlen(message), 0);
                }
            }
            pthread_mutex_unlock(&mutex);
        }
    }

    close(client_socket);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] == client_socket) {
            for (int j = i; j < num_clients - 1; j++) {
                client_sockets[j] = client_sockets[j + 1];
            }
            num_clients--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_un server_address, client_address;
    socklen_t client_address_len;

    if (access(SOCKET_PATH, F_OK) != -1) {
        unlink(SOCKET_PATH);
    }

    if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SOCKET_PATH, sizeof(server_address.sun_path) - 1);

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");

    while (1) {
        client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&mutex);
        if (num_clients < MAX_CLIENTS) {
            client_sockets[num_clients] = client_socket;
            num_clients++;
            printf("New client connected!!!\n");

            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, &client_socket);
            pthread_detach(thread);
        } else {
            printf("Maximum number of clients reached. Connection rejected.\n");
            close(client_socket);
        }
        pthread_mutex_unlock(&mutex);
    }

    close(server_socket);
    unlink(SOCKET_PATH);

    return 0;
}
