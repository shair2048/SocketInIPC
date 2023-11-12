#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/demo_socket"

int main() {
    int client_socket;
    struct sockaddr_un server_address;

    // Tạo socket
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

    char message[1024];
    int continue_sending = 1;  // Biến kiểm soát quá trình truyền thông

    // Vòng lặp để nhập và gửi tin nhắn
    while (continue_sending) {
        // Nhập tin nhắn từ người dùng
        printf("Enter your message (or type 'quit' to exit): ");
        fgets(message, sizeof(message), stdin);

        // Gửi tin nhắn đến server
        send(client_socket, message, strlen(message), 0);

        // Kiểm tra điều kiện dừng (nếu tin nhắn là "quit")
        if (strcmp(message, "quit\n") == 0) {
            printf("Client is quitting.\n");
            continue_sending = 0;  // Dừng quá trình truyền thông
        } else {
            // Nhận phản hồi từ server
            ssize_t received = recv(client_socket, message, sizeof(message) - 1, 0);
            if (received <= 0) {
                perror("Receive failed");
                break;
            }

            message[received] = '\0';
            printf("Received response from server: %s\n", message);
        }
    }

    // Đóng socket
    close(client_socket);

    return 0;
}
