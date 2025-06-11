#ifndef BANK_H
#define BANK_H

#include <stddef.h>

#define MAX_NAME_LEN 50
#define MAX_ID_LEN 20
#define MAX_ACCOUNT_TYPE_LEN 10
#define MAX_ACCOUNTS 100
#define MAX_TRANSACTIONS 5

typedef struct {
    double transactions[MAX_TRANSACTIONS];
    int transaction_count;
} Statement;

typedef struct {
    char name[MAX_NAME_LEN];
    char national_id[MAX_ID_LEN];
    char account_type[MAX_ACCOUNT_TYPE_LEN];
    char* account_number;
    double balance;
    int pin;
    Statement statement;
    int is_active;
} Account;

extern Account accounts[MAX_ACCOUNTS];
extern int account_count;

// Function declarations
Account open_account(const char* name, const char* national_id, const char* account_type, double initial_deposit, int pin);
int close_account(const char* account_number, int pin);
int deposit(const char* account_number, int pin, double amount);
int withdraw(const char* account_number, int pin, double amount);
double check_balance(const char* account_number, int pin);
int get_statement(const char* account_number, int pin, char* output, size_t output_size);
int save_accounts_to_file(const char* filename);
int load_accounts_from_file(const char* filename);

#endif
