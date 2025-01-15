#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345
#define P 23 // A prime number (public)
#define G 5  // A generator (base)

long long mod_exp(long long base, long long exp, long long mod) {
    long long result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    return result;
}

long long generate_shared_key(long long public_key, long long private_key) {
    return mod_exp(public_key, private_key, P);
}

int main() {
    srand(time(0));
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;
    
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    // Step 1: Generate server's private key and public key
    long long server_private_key = rand() % (P - 2) + 1; // Random private key
    long long server_public_key = mod_exp(G, server_private_key, P);

    // Step 2: Send the public key to the client
    send(new_socket, &server_public_key, sizeof(server_public_key), 0);
    std::cout << "Sent public key: " << server_public_key << std::endl;

    // Step 3: Receive the client's public key
    long long client_public_key;
    recv(new_socket, &client_public_key, sizeof(client_public_key), 0);
    std::cout << "Received client's public key: " << client_public_key << std::endl;

    // Step 4: Compute shared secret key
    long long shared_secret = generate_shared_key(client_public_key, server_private_key);
    std::cout << "Shared secret key: " << shared_secret << std::endl;

    // Step 5: Encrypt and send a symmetric shared message
    std::string message = "This is a secret message!";
    std::cout << "Message to send: " << message << std::endl;

    // Simple XOR encryption with shared secret (not secure in real applications, just for example)
    for (char &ch : message) {
        ch ^= (shared_secret & 0xFF); // XOR the char with the lower byte of the shared secret
    }

    send(new_socket, message.c_str(), message.length(), 0);
    std::cout << "Encrypted message sent to client." << std::endl;

    close(new_socket);
    close(server_fd);
    return 0;
}
