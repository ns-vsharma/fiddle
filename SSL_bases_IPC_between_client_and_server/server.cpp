#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

#define SERVER_PORT 12345

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX* create_context() {
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        std::cerr << "Error creating SSL context" << std::endl;
        abort();
    }
    return ctx;
}

void configure_context(SSL_CTX* ctx) {
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading certificate" << std::endl;
        abort();
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "private.pem", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Error loading private key" << std::endl;
        abort();
    }
    if (!SSL_CTX_check_private_key(ctx)) {
        std::cerr << "Private key does not match the certificate" << std::endl;
        abort();
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    init_openssl();
    
    SSL_CTX* ctx = create_context();
    configure_context(ctx);
    
    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);
    
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 1);

    std::cout << "Server listening on port " << SERVER_PORT << std::endl;

    SOCKET client_fd = accept(server_fd, NULL, NULL);
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client_fd);

    if (SSL_accept(ssl) <= 0) {
        std::cerr << "SSL accept failed" << std::endl;
    } else {
        std::cout << "Secure connection established" << std::endl;

        // Send and receive messages
        const char* msg = "Hello, secure client!";
        SSL_write(ssl, msg, strlen(msg));

        char buf[1024];
        int bytes = SSL_read(ssl, buf, sizeof(buf));
        if (bytes > 0) {
            buf[bytes] = 0;
            std::cout << "Received message from client: " << buf << std::endl;
        }
    }

    SSL_free(ssl);
    closesocket(client_fd);
    closesocket(server_fd);

    cleanup_openssl();
    WSACleanup();
    return 0;
}
