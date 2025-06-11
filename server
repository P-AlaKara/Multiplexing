/* CONCEPTUAL SERVER ALGORITHM (CONNECTION-ORIENTED) */

/*
1.  Server Setup:
    * Create a socket and bind to a well-known address.
    * Put the socket in passive mode and initialize the queue size.

2.  Main Event Loop:
    * Prepare Watch List:
        * Reset the list of sockets to watch for readability.
        * Add the listening socket to this list (to detect new connection requests).
        * Add all currently active client sockets to this list (to detect incoming data).
    * Wait for Activity:
        * Call select() to block until one or more sockets are ready for reading.
    * Process Ready Sockets:
        * If the listening socket is ready:
            * Accept the new client connection.
            * Add the new client's socket to the list of active client sockets.
        * For each active client socket:
            * If the client socket is ready:
                * Read data from the client.
                * Parse the received command and its arguments.
                * Execute the command by calling a function(e.g., open account, withdraw funds).
                * Formulate and send the response back to the client.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include "bank.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS FD_SETSIZE
#define MAX_ARGS 10
#define DATA_FILE "accounts_data.txt"

void trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = 0;
}

void to_lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower((unsigned char)str[i]);
    }
}

int handle_command(int client_fd, char *buffer) { //decode, process, and respond to requests
    char command[50] = {0};
    char args[MAX_ARGS][100] = {0};
    int arg_count = 0;
    char response[BUFFER_SIZE * 2] = {0};

    int len = strlen(buffer);
    if (len == 0 || buffer[len - 1] != ';') {
        snprintf(response, sizeof(response), "ERROR Invalid protocol format: Missing semicolon.;\n");
        send(client_fd, response, strlen(response), 0);
        printf("[!] Sent error to socket %d: %s\n", client_fd, response); // Log sent error
        return 0;
    }
    buffer[len - 1] = '\0';

    char *token = strtok(buffer, ",");
    int token_index = 0;
    while (token != NULL && token_index < MAX_ARGS + 1) {
        trim_whitespace(token);
        if (token_index == 0) {
            strncpy(command, token, sizeof(command) - 1);
            to_lowercase(command);
        } else {
            strncpy(args[arg_count], token, sizeof(args[arg_count]) - 1);
            arg_count++;
        }
        token = strtok(NULL, ",");
        token_index++;
    }

    printf("[+] Processing command '%s' from socket %d with %d arguments\n", command, client_fd, arg_count); // Log command processing

    if (strcmp(command, "open") == 0 && arg_count == 5) {
        char *name = args[0];
        char *nid = args[1];
        char *type = args[2];
        double deposit = atof(args[3]);
        int pin = atoi(args[4]);

        to_lowercase(type);
        if (strcmp(type, "savings") && strcmp(type, "checking")) {
            snprintf(response, sizeof(response), "ERROR Invalid account type.\n");
        } else {
            Account acc = open_account(name, nid, type, deposit, pin);
            if (acc.is_active) {
                snprintf(response, sizeof(response), "OK,Account Number:%s,PIN:%d;\n", acc.account_number, acc.pin);
            } else {
                snprintf(response, sizeof(response), "ERROR 2 Failed to open account.\n");
            }
        }
    } else if (strcmp(command, "close") == 0 && arg_count == 2) {
        int res = close_account(args[0], atoi(args[1]));
        snprintf(response, sizeof(response), res == 0 ? "OK,Account %s closed.;\n" : "ERROR 1 Account not found or wrong PIN.;\n", args[0]);

    } else if (strcmp(command, "withdraw") == 0 && arg_count == 3) {
        int res = withdraw(args[0], atoi(args[1]), atof(args[2]));
        switch (res) {
            case 0: snprintf(response, sizeof(response), "OK,Withdrawal successful.;\n"); break;
            case 1: snprintf(response, sizeof(response), "ERROR 1 Account not found or incorrect PIN.;\n"); break;
            case 3: snprintf(response, sizeof(response), "ERROR 3 Insufficient funds or minimum balance not met.;\n"); break;
            case 4: snprintf(response, sizeof(response), "ERROR 4 Withdrawal must be a positive multiple of 500.;\n"); break;
            default: snprintf(response, sizeof(response), "ERROR Unknown withdrawal error code: %d.;\n", res);
        }
    } else if (strcmp(command, "deposit") == 0 && arg_count == 3) {
        int res = deposit(args[0], atoi(args[1]), atof(args[2]));
        snprintf(response, sizeof(response), res == 0 ? "OK,Deposit successful.;\n" : "ERROR %d Deposit failed.;\n", res);

    } else if (strcmp(command, "balance") == 0 && arg_count == 2) {
        double bal = check_balance(args[0], atoi(args[1]));
        snprintf(response, sizeof(response), bal >= 0.0 ? "OK,Balance:%.2f;\n" : "ERROR 1 Account not found or incorrect PIN.;\n", bal);

    } else if (strcmp(command, "statement") == 0 && arg_count == 2) {
        char output[BUFFER_SIZE * 2] = {0};
        int res = get_statement(args[0], atoi(args[1]), output, sizeof(output));
        if (res == 0) {
            snprintf(response, sizeof(response), "OK,%s;\n", output);
        } else {
            snprintf(response, sizeof(response), "ERROR %d Failed to get statement.;\n", res);
        }

    } else if (strcmp(command, "quit") == 0) {
        snprintf(response, sizeof(response), "OK,Connection terminated.;\n");
        send(client_fd, response, strlen(response), 0);
        printf("[-] Sending termination response to socket %d\n", client_fd); // Log termination response
        return -1; // signal to close socket
    } else {
        snprintf(response, sizeof(response), "ERROR Unknown or badly formatted command: %s;\n", command);
    }

    send(client_fd, response, strlen(response), 0);
    printf("[*] Sent response to socket %d: %s\n", client_fd, response); // Log sent response
    return 0;
}

int main() {
    int server_fd, client_fd, max_fd, activity;
    int client_sockets[MAX_CLIENTS] = {0};
    struct sockaddr_in addr;
    fd_set read_fds;
    char buffer[BUFFER_SIZE];
    socklen_t addrlen = sizeof(addr);

    srand(time(NULL));
    load_accounts_from_file(DATA_FILE);
    printf("Banking server started on port %d\n", PORT);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) { perror("Socket failed"); exit(EXIT_FAILURE); }
    printf("[+] Server socket created: %d\n", server_fd); // Log server socket creation

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Bind failed"); exit(EXIT_FAILURE);
    }
    printf("[+] Server socket bound to port %d\n", PORT); // Log bind success

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed"); exit(EXIT_FAILURE);
    }
    printf("[+] Server listening for incoming connections...\n"); // Log listening state

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &read_fds);
                if (client_sockets[i] > max_fd) max_fd = client_sockets[i];
            }
        }

        printf("\n[*] Waiting for activity on sockets (max_fd: %d)...\n", max_fd); // Log before select call
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        printf("[*] Select returned with activity: %d\n", activity); // Log after select returns

        if (activity < 0 && errno != EINTR) {
            perror("select error");
            printf("[-] Select error occurred. Continuing loop.\n"); // Log select error
            continue;
        }

        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(server_fd, &read_fds)) {
            client_fd = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
            if (client_fd < 0) { perror("accept"); printf("[-] Failed to accept new connection.\n"); continue; }

            printf("[+] New connection accepted: socket %d (from %s:%d)\n", client_fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)); // Detailed new connection log
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = client_fd;
                    printf("[+] Adding socket %d to client_sockets[%d]\n", client_fd, i); // Log adding client to array
                    break;
                }
            }
        }

        // Else it's some IO operation on some other socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sockfd = client_sockets[i];
            if (sockfd > 0 && FD_ISSET(sockfd, &read_fds)) {
                printf("[*] Socket %d is ready for reading.\n", sockfd); // Log that a client socket is ready
                memset(buffer, 0, BUFFER_SIZE);
                int bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);

                if (bytes_read <= 0) {
                    if (bytes_read == 0) {
                        printf("[-] Client on socket %d disconnected gracefully.\n", sockfd); // Log graceful disconnect
                    } else {
                        perror("read error");
                        printf("[-] Read error on socket %d.\n", sockfd); // Log read error
                    }
                    printf("[-] Closing socket %d\n", sockfd);
                    close(sockfd);
                    client_sockets[i] = 0;
                } else {
                    printf("[*] Received %d bytes from socket %d: \"%s\"\n", bytes_read, sockfd, buffer); // Log received data
                    trim_whitespace(buffer);
                    int result = handle_command(sockfd, buffer);
                    if (result == -1) {
                        printf("[-] QUIT command received from socket %d. Closing connection.\n", sockfd);
                        close(sockfd);
                        client_sockets[i] = 0;
                    }
                }
            }
        }
    }

    close(server_fd);
    save_accounts_to_file(DATA_FILE);
    printf("Server shutting down.\n"); // Log server shutdown
    return 0;
}
