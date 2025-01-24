#include <openssl/ssl.h>
#include <openssl/err.h>
#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#define SERVER_PORT 12345
#define SERVER_HOST "127.0.0.1"

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX* create_context() {
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        std::cerr << "Error creating SSL context" << std::endl;
        abort();
    }
    return ctx;
}

void configure_context(SSL_CTX* ctx) {
    if (SSL_CTX_load_verify_locations(ctx, "cert.pem", NULL) != 1) {
        std::cerr << "Error loading server certificate" << std::endl;
        abort();
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    init_openssl();
    
    SSL_CTX* ctx = create_context();
    configure_context(ctx);

    SOCKET client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_HOST);
    server_addr.sin_port = htons(SERVER_PORT);

    connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_fd);

    if (SSL_connect(ssl) <= 0) {
        std::cerr << "SSL connect failed" << std::endl;
    } else {
        std::cout << "Secure connection established" << std::endl;

        // Receive server's message
        char buf[1024];
        int bytes = SSL_read(ssl, buf, sizeof(buf));
        if (bytes > 0) {
            buf[bytes] = 0;
            std::cout << "Received from server: " << buf << std::endl;
        }

        // Send message back to server
        const char* msg = "Hello, server!";
        SSL_write(ssl, msg, strlen(msg));
    }

    SSL_free(ssl);
    closesocket(client_fd);

    cleanup_openssl();
    WSACleanup();
    return 0;
}
