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

        // message[received] = '\0';
        // printf("\x1b[1A"); // Di chuyển lên 1 dòng
        // printf("\x1b[K");  // Xóa dòng hiện tại
        printf("Received response from server: %s\n", message);
    }

    close(client_socket);
    exit(EXIT_SUCCESS);
}

int main() {
    int client_socket;
    struct sockaddr_un server_address;

    // Xác minh rằng file socket của server đã tồn tại
    if (access(SOCKET_PATH, F_OK) == -1) {
        perror("Server socket does not exist");
        exit(EXIT_FAILURE);
    }

    // Tạo socket client
    if ((client_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ server
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SOCKET_PATH, sizeof(server_address.sun_path) - 1);

    // Kết nối đến server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Tạo luồng riêng để nhận phản hồi từ server
    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, &client_socket);
    pthread_detach(thread);

    char message[1024];

    while (1) {
        // Nhập tin nhắn từ người dùng
        // printf("Enter your message (or type 'quit' to exit): ");
        fgets(message, sizeof(message), stdin);

        // Gửi tin nhắn đến server
        send(client_socket, message, strlen(message), 0);

        // Kiểm tra điều kiện dừng (nếu tin nhắn là "quit")
        if (strcmp(message, "quit\n") == 0) {
            printf("Client is quitting.\n");
            break;  // Dừng quá trình truyền thông
        }
    }

    // Đóng socket client
    close(client_socket);

    return 0;
}
