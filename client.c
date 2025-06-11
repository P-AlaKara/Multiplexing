#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <arpa/inet.h> 
#include <sys/socket.h> 
#include <ctype.h> 

//declare variables (serverIP, serverPort, buffer for sending/receiving data)
#define SERVER_IP "192.168.1.99" 
#define PORT 8080 
#define BUFFER_SIZE 1024 

// Helper function to trim leading/trailing whitespace from a string
void trim_whitespace(char *str) {
    char *end;

    while(isspace((unsigned char)*str)) str++;

    if(*str == 0)  
        return;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;

    *(end+1) = 0;
}


int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_received;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error in socket creation");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error in connecting to server");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the banking server at %s:%d\n", SERVER_IP, PORT);
    printf("Available commands: OPEN, CLOSE, WITHDRAW, DEPOSIT, BALANCE, STATEMENT, QUIT\n");

    // Send requests and receive responses
    while (1) {
        printf("> "); 

        // Get command from user
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            perror("Error reading input");
            break; 
        }

        // Remove newline and whitespace
        buffer[strcspn(buffer, "\n")] = 0;
        trim_whitespace(buffer);

        // Check if the user wants to quit
        if (strcmp(buffer, "QUIT") == 0) {
            send(client_socket, buffer, strlen(buffer), 0); 
            bytes_received = read(client_socket, buffer, BUFFER_SIZE - 1);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                printf("%s", buffer);
            }
            break; 
        }

        // Send the command to the server
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Error sending data");
            break; 
        }

        // Clear the buffer for receiving
        memset(buffer, 0, BUFFER_SIZE);

        // Receive the response from the server
        bytes_received = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            break; 
        }

        // print response
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }

    // Close the client socket
    close(client_socket);
    printf("Client disconnected.\n");

    return 0;
}
