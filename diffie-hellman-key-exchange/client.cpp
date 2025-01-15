#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
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
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Creating socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Step 1: Generate client's private key and public key
    long long client_private_key = rand() % (P - 2) + 1; // Random private key
    long long client_public_key = mod_exp(G, client_private_key, P);

    // Step 2: Send the public key to the server
    send(sock, &client_public_key, sizeof(client_public_key), 0);
    std::cout << "Sent public key: " << client_public_key << std::endl;

    // Step 3: Receive the server's public key
    long long server_public_key;
    recv(sock, &server_public_key, sizeof(server_public_key), 0);
    std::cout << "Received server's public key: " << server_public_key << std::endl;

    // Step 4: Compute shared secret key
    long long shared_secret = generate_shared_key(server_public_key, client_private_key);
    std::cout << "Shared secret key: " << shared_secret << std::endl;

    // Step 5: Receive and decrypt the message
    char buffer[1024] = {0};
    int len = recv(sock, buffer, sizeof(buffer), 0);
    std::string encrypted_message(buffer, len);
    std::cout << "Encrypted message received: " << encrypted_message << std::endl;

    // Simple XOR decryption with shared secret (same method as encryption)
    for (char &ch : encrypted_message) {
        ch ^= (shared_secret & 0xFF); // XOR the char with the lower byte of the shared secret
    }

    std::cout << "Decrypted message: " << encrypted_message << std::endl;

    close(sock);
    return 0;
}
