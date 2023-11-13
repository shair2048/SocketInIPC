#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/demo_socket"

int client_sockets[10]; // Mảng chứa socket của các client
int num_clients = 0; // Số lượng client hiện tại

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char message[1024];

    while (1) {
        // Nhận tin nhắn từ client
        ssize_t received = recv(client_socket, message, sizeof(message) - 1, 0);
        if (received <= 0) {
            perror("Receive failed");
            break;
        }

        message[received] = '\0';
        printf("Received message from client: %s\n", message);

        // Kiểm tra điều kiện dừng (ví dụ: nếu tin nhắn là "quit")
        if (strcmp(message, "quit") == 0) {
            printf("Client is quitting.\n");
            break;  // Kết thúc kết nối của client
        } else {
            // Gửi tin nhắn đến tất cả client khác
            for (int i = 0; i < num_clients; i++) {
                if (client_sockets[i] != client_socket) {
                    send(client_sockets[i], message, strlen(message), 0);
                }
            }
        }
    }

    close(client_socket);

    // Loại bỏ client_socket khỏi mảng
    for (int i = 0; i < num_clients; i++) {
        if (client_sockets[i] == client_socket) {
            for (int j = i; j < num_clients - 1; j++) {
                client_sockets[j] = client_sockets[j + 1];
            }
            num_clients--;
            break;
        }
    }

    return NULL;
}

int main() {
    int server_socket;
    struct sockaddr_un server_address;
    socklen_t client_address_len;
    struct sockaddr_un client_address;

    // Kiểm tra xem file socket đã tồn tại chưa, nếu có thì xóa
    if (access(SOCKET_PATH, F_OK) != -1) {
        unlink(SOCKET_PATH);
    }

    // Tạo socket
    if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Sử dụng tùy chọn SO_REUSEADDR
    int reuse = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SOCKET_PATH, sizeof(server_address.sun_path) - 1);

    // Bind socket đến địa chỉ
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening...\n");

    while (1) {
        client_address_len = sizeof(client_address);
        // Chấp nhận kết nối mới
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Accept failed");
            continue;
        }

        // Thêm client_socket vào mảng
        client_sockets[num_clients] = client_socket;
        num_clients++;

        printf("New client connected!!!\n");

        // Tạo một luồng mới để xử lý client
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, &client_socket);
        pthread_detach(thread);
    }

    // Đóng socket
    close(server_socket);

    // Xóa socket file
    unlink(SOCKET_PATH);

    return 0;
}
