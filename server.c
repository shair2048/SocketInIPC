#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/demo_socket"

int main() {
    int server_socket, client_socket;
    struct sockaddr_un server_address, client_address;
    socklen_t client_address_len = sizeof(client_address);

    // Tạo socket
    if ((server_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
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

    // Chấp nhận kết nối mới
    if ((client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len)) == -1) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    printf("Client connected\n");

    char message[1024];
    int continue_sending = 1;  // Biến kiểm soát quá trình truyền thông

    // Vòng lặp để nhận và gửi tin nhắn
    while (continue_sending) {
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
            printf("Server is quitting.\n");
            continue_sending = 0;  // Dừng quá trình truyền thông
        } else {
            // Gửi phản hồi về client
            send(client_socket, message, strlen(message), 0);

            // Nhập tin nhắn từ server
            printf("Enter your response (or type 'quit' to exit): ");
            fgets(message, sizeof(message), stdin);

            // Gửi tin nhắn đến client
            send(client_socket, message, strlen(message), 0);
        }
    }

    // Đóng socket
    close(client_socket);
    close(server_socket);

    // Xóa socket file
    unlink(SOCKET_PATH);

    return 0;
}
