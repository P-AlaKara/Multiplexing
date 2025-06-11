# Banking Server

This project is a simple C-based banking system using TCP sockets for client-server communication. The server handles account operations, and the client acts as a terminal for sending commands.

## Features

- Server uses I/O multiplexing using (`select()`) to handle multiple clients
- Supports operations like:
  - `OPEN`
  - `CLOSE`
  - `DEPOSIT`
  - `WITHDRAW`
  - `BALANCE`
  - `STATEMENT`
  - `QUIT`

## Build Instructions

You'll need a Unix environment and `gcc` installed

### 1. Clone the Repo

```bash
git clone https://github.com/P-AlaKara/Multiplexing
cd Multiplexing
```

### 2. Compile the Server

```bash
gcc server.c bank.c -o server
```

### 3. Compile the Client

```bash
gcc client.c -o client
```

TIP: Make sure `bank.c` and `bank.h` are present in the same directory as `server.c`.
TIP2: Update the IP in `client.c` to your server's IP (default is `192.168.1.99`). If localhost, use `127.0.0.1`

## Running the Programs

### 1. Start the Server

```bash
./server
```

> Listens on port `8080` by default.

### 2. Connect with a Client

```bash
./client
```

## Command Format

All commands must end with a **semicolon `;`**, and fields are **comma-separated**.

### Example Commands

```text
OPEN, John, 123456789, savings, 10000, 1234;
DEPOSIT, 100001, 1234, 2000;
WITHDRAW, 100001, 1234, 500;
BALANCE, 100001, 1234;
STATEMENT, 100001, 1234;
CLOSE, 100001, 1234;
QUIT;
```

> DON'T FORGET THE SEMICOLON!!

## Appendix

* Programming language: C
* Networking: TCP (IPv4)
* Server concurrency: `select()`-based I/O multiplexing
* Data storage: File-based (`accounts_data.txt`)
* Max clients: Defined by `FD_SETSIZE`
  
