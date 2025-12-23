#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>

#define PORT 1234
#define DEST_IP "192.168.1.120"
#define SOCKET_FILE "/tmp/tcp_client_socket.lock"

int create_or_get_socket() {
    static int sock = -1;
    
    // 如果socket已经创建，直接返回
    if (sock >= 0) {
        return sock;
    }
    
    // 创建文件锁，确保只有一个进程在创建socket
    int lock_fd = open(SOCKET_FILE, O_CREAT | O_RDWR, 0666);
    if (lock_fd < 0) {
        perror("Failed to create lock file");
        return -1;
    }
    
    // 获取排他锁
    if (flock(lock_fd, LOCK_EX) < 0) {
        perror("Failed to lock file");
        close(lock_fd);
        return -1;
    }
    
    // 再次检查socket是否已被其他进程创建
    if (sock < 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);  // 创建TCP套接字
        if (sock < 0) {
            perror("Socket creation failed");
            flock(lock_fd, LOCK_UN);
            close(lock_fd);
            return -1;
        }
        
        // 设置socket选项，允许地址重用
        int opt = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("Setsockopt failed");
            close(sock);
            sock = -1;
            flock(lock_fd, LOCK_UN);
            close(lock_fd);
            return -1;
        }
        
        printf("New socket created: %d\n", sock);
    }
    
    // 释放锁
    flock(lock_fd, LOCK_UN);
    close(lock_fd);
    
    return sock;
}

void cleanup_socket() {
    // 在实际应用中，您可以选择不关闭socket以保持复用
    // 或者通过其他机制管理socket生命周期
    printf("Socket cleanup completed\n");
}

void send_command(int sock, const char *cmd) {
    ssize_t sent = send(sock, cmd, strlen(cmd), 0);  // 使用TCP的send函数
    if (sent < 0) {
        perror("Send failed");
    } else {
        printf("Sent via socket %d: %s\n", sock, cmd);
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr;
    
    // 获取或创建socket
    int sock = create_or_get_socket();
    if (sock < 0) {
        return -1;
    }

    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, DEST_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    // 命令行参数模式
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (argv[i][0] == '-') {
                send_command(sock, argv[i] + 1);
            } else {
                send_command(sock, argv[i]);
            }
        }
        // 注意：这里不再关闭socket，保持复用
        close(sock);  // 关闭连接
        return 0;
    }

    // 交互模式
    char message[100];
    while(1) {
        printf("Enter command (prefix with - or type exit): ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;

        if (strcmp(message, "exit") == 0) break;
        
        if (message[0] == '-') {
            send_command(sock, message + 1);
        } else {
            send_command(sock, message);
        }
    }

    // 程序结束时进行清理
    close(sock);  // 关闭连接
    cleanup_socket();
    return 0;
}