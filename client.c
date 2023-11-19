#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/demo_socket"

void *receive_messages(void *arg) {
    int client_socket = *(int *)arg;
    char message[1024];

    while (1) {
        ssize_t received = recv(client_socket, message, sizeof(message) - 1, 0);
        if (received <= 0) {
            perror("Receive failed");
            break;
        }

        message[received] = '\0';
        printf("Received response from server: %s\n", message);
    }

    close(client_socket);
    exit(EXIT_SUCCESS);
}

int main() {
    int client_socket;
    struct sockaddr_un server_address;

    if (access(SOCKET_PATH, F_OK) == -1) {
        perror("Server socket does not exist");
        exit(EXIT_FAILURE);
    }

    if ((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SOCKET_PATH, sizeof(server_address.sun_path) - 1);

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &client_socket);
    pthread_detach(thread);

    char message[1024];

    while (1) {
        // Read input using fgets to handle spaces
        printf("Enter your message (or type 'quit' to exit): ");
        fgets(message, sizeof(message), stdin);

        // Trim trailing newline character
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';
        }

        // Send the message to the server
        send(client_socket, message, strlen(message), 0);

        if (strcmp(message, "quit") == 0) {
            printf("Client is quitting.\n");
            break;
        }
    }

    close(client_socket);

    return 0;
}
