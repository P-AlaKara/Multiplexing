#include "bank.h" // Include the header file
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> 
#include <stdbool.h> 

Account accounts[MAX_ACCOUNTS];
int account_count = 0; 

// Helper function to find an account index by account number and PIN
int find_account_index(const char* account_number, int pin) {
    for (int i = 0; i < account_count; i++) {
        // Check if the slot is active and account number matches
        if (accounts[i].is_active && accounts[i].account_number != NULL &&
            strcmp(accounts[i].account_number, account_number) == 0 &&
            accounts[i].pin == pin) {
            return i; // Account found
        }
    }
    return -1; // Account not found or PIN incorrect
}


// Helper function to generate a unique account number 
char* generate_account_number() {
    // srand() should be called once in main server process
    char* acc_num = NULL;
    bool unique = false;
    int attempts = 0;
    const int max_attempts = 100; // Prevent infinite loops

    while (!unique && attempts < max_attempts) {
        // Generate a large number based on time and random component
        long long random_num = time(NULL) + (rand() % 100000) + attempts;

        // Allocate memory for the account number string
        // Max possible long long is around 19 digits, plus null terminator
        acc_num = malloc(20);
        if (acc_num == NULL) {
            perror("Failed to allocate memory for account number");
            return NULL; // Return NULL on allocation failure
        }

        // Convert the number to a string
        sprintf(acc_num, "%lld", random_num);

        // Check for uniqueness against existing active accounts
        unique = true; // Assume unique until a match is found
        for(int i = 0; i < account_count; ++i) {
            if(accounts[i].is_active && accounts[i].account_number != NULL && strcmp(accounts[i].account_number, acc_num) == 0) {
                unique = false; // Found a duplicate
                free(acc_num); // Free the non-unique number
                acc_num = NULL;
                break; // Exit inner loop and try generating again
            }
        }
        attempts++;
    }

    if (!unique) {
         fprintf(stderr, "Warning: Could not generate a unique account number after %d attempts.\n", max_attempts);
         // acc_num is already NULL at this point if attempts failed
    }

    return acc_num;
}

int generate_pin_internal() {
    // srand() should be called once in main server process
    return 1000 + (rand() % 9000);
}


// Open a new bank account
// Returns Account struct on success, Account with is_active=0 and account_number=NULL on failure
Account open_account(const char* name, const char* national_id, const char* account_type, double initial_deposit, int pin) {
    Account new_account_details;
    // Initialize to a "failure" state by default as per bank.h struct
    memset(&new_account_details, 0, sizeof(Account));
    new_account_details.is_active = 0; // Indicate failure

    // Validate initial deposit based on the requirement (minimum 1000)
    if (initial_deposit < 1000.0) {
        fprintf(stderr, "Error: Initial deposit must be at least 1000.\n");
        return new_account_details; // Return failure state
    }

    // Find an available slot in the accounts array
    int account_index = -1;
    for(int i = 0; i < MAX_ACCOUNTS; ++i) {
        if (!accounts[i].is_active) {
            account_index = i;
            break;
        }
    }

    if (account_index == -1) {
        fprintf(stderr, "Error: Maximum number of accounts reached.\n");
        return new_account_details; // Return failure state
    }

    // Populate the account structure in the global array
    strncpy(accounts[account_index].name, name, MAX_NAME_LEN - 1);
    accounts[account_index].name[MAX_NAME_LEN - 1] = '\0';

    strncpy(accounts[account_index].national_id, national_id, MAX_ID_LEN - 1);
    accounts[account_index].national_id[MAX_ID_LEN - 1] = '\0';

    strncpy(accounts[account_index].account_type, account_type, MAX_ACCOUNT_TYPE_LEN - 1);
    accounts[account_index].account_type[MAX_ACCOUNT_TYPE_LEN - 1] = '\0';

    // Generate account number
    accounts[account_index].account_number = generate_account_number();
    if (accounts[account_index].account_number == NULL) {
        // Allocation or generation failed for account_number
        accounts[account_index].is_active = 0; // Mark slot as inactive
        return new_account_details; // Return failure state
    }

    // Use the provided PIN as per bank.h signature
    accounts[account_index].pin = pin;
    accounts[account_index].balance = initial_deposit;
    accounts[account_index].is_active = 1; // Mark as active

    // Initialize transaction history (Statement struct)
    accounts[account_index].statement.transaction_count = 0;
    // Record initial deposit as the first transaction
    if (accounts[account_index].statement.transaction_count < MAX_TRANSACTIONS) {
        accounts[account_index].statement.transactions[accounts[account_index].statement.transaction_count] = initial_deposit; // Store as positive for deposit
        accounts[account_index].statement.transaction_count++;
    }


    // If this is the first account in this slot, increment account_count
    if (account_index >= account_count) {
         account_count = account_index + 1;
    }
    new_account_details = accounts[account_index];

    save_accounts_to_file("accounts_data.txt"); 

    return new_account_details; // Return the populated Account struct
}

// Close an account
// Returns 0 on success, 1 on account not found/PIN incorrect
int close_account(const char* account_number, int pin) {
    int index = find_account_index(account_number, pin);

    if (index != -1) {
        // Account found, proceed to close
        free(accounts[index].account_number); // Free dynamically allocated account number
        accounts[index].account_number = NULL;
        accounts[index].is_active = 0; // Mark slot as inactive

        // No need to shift array elements if we use the is_active flag

        // Save accounts to file after a successful closure
        save_accounts_to_file("accounts_data.txt"); // Use a default filename
        return 0; // Success
    }

    return 1; // Failure (Account not found or incorrect PIN)
}

// Withdraw from account
// Returns 0 on success, non-zero on failure (1: account/pin, 3: insufficient funds, 4: not multiple of 500)
int withdraw(const char* account_number, int pin, double amount) {
    int index = find_account_index(account_number, pin);

    if (index != -1) {
        // Account found
        // Check withdrawal amount multiple (Ksh 500)
        if ((int)amount % 500 != 0 || amount <= 0) { // Also ensure amount is positive
            fprintf(stderr, "Error: Withdrawal amount must be a positive multiple of 500.\n");
            return 4; // Withdrawal amount not multiple of 500
        }

        // Check minimum balance requirement (must leave at least 1000)
        if (accounts[index].balance - amount < 1000.0) {
            fprintf(stderr, "Error: Insufficient funds or minimum balance requirement not met.\n");
            return 3; // Insufficient funds
        }

        // Perform withdrawal
        accounts[index].balance -= amount;

        // Record transaction (circular buffer for last MAX_TRANSACTIONS)
        if (accounts[index].statement.transaction_count < MAX_TRANSACTIONS) {
            accounts[index].statement.transactions[accounts[index].statement.transaction_count] = -amount; // Store as negative for withdrawal
            accounts[index].statement.transaction_count++;
        } else {
            // Shift older transactions to make space for the new one
            for (int j = 0; j < MAX_TRANSACTIONS - 1; j++) {
                accounts[index].statement.transactions[j] = accounts[index].statement.transactions[j + 1];
            }
            accounts[index].statement.transactions[MAX_TRANSACTIONS - 1] = -amount; // Store as negative
        }

        // Save accounts to file after a successful withdrawal
        save_accounts_to_file("accounts_data.txt"); // Use a default filename
        return 0; // Success
    }

    return 1; // Account not found or PIN incorrect
}

// Deposit into account
// Returns 0 on success, non-zero on failure (1: account/pin, 3: minimum deposit not met)
int deposit(const char* account_number, int pin, double amount) {
    int index = find_account_index(account_number, pin);

    if (index != -1) {
        // Account found
        // Check minimum deposit amount (Ksh 500)
        if (amount < 500.0) {
            fprintf(stderr, "Error: Minimum deposit amount is 500.\n");
            return 3; // Minimum deposit amount not met
        }

        // Perform deposit
        accounts[index].balance += amount;

        // Record transaction (circular buffer for last MAX_TRANSACTIONS)
        if (accounts[index].statement.transaction_count < MAX_TRANSACTIONS) {
            accounts[index].statement.transactions[accounts[index].statement.transaction_count] = amount; // Store as positive for deposit
            accounts[index].statement.transaction_count++;
        } else {
            // Shift older transactions to make space for the new one
            for (int j = 0; j < MAX_TRANSACTIONS - 1; j++) {
                accounts[index].statement.transactions[j] = accounts[index].statement.transactions[j + 1];
            }
            accounts[index].statement.transactions[MAX_TRANSACTIONS - 1] = amount; // Store as positive
        }

        // Save accounts to file after a successful deposit
        save_accounts_to_file("accounts_data.txt"); // Use a default filename
        return 0; // Success
    }

    return 1; // Account not found or PIN incorrect
}

// Check account balance
// Returns balance on success, -1.0 on error (account not found/PIN incorrect)
double check_balance(const char* account_number, int pin) {
    int index = find_account_index(account_number, pin);

    if (index != -1) {
        // Account found, return balance
        return accounts[index].balance;
    }

    return -1.0; // Error indicator as per bank.h signature
}

// Get account statement (last MAX_TRANSACTIONS)
// Returns 0 on success, 1 on account not found/PIN incorrect, 2 on buffer too small
int get_statement(const char* account_number, int pin, char* output, size_t output_size) {
    int index = find_account_index(account_number, pin);

    if (index != -1) {
        // Account found, generate statement string
        int written = 0;
        written += snprintf(output + written, output_size - written, "Statement for Account %s (Balance: %.2f):\n",
                            account_number, accounts[index].balance);

        if (written >= output_size) return 2; // Buffer too small

        if (accounts[index].statement.transaction_count == 0) {
            written += snprintf(output + written, output_size - written, "No transactions yet.\n");
             if (written >= output_size) return 2; // Buffer too small
        } else {
            written += snprintf(output + written, output_size - written, "Last %d Transactions:\n", accounts[index].statement.transaction_count);
             if (written >= output_size) return 2; // Buffer too small

            for (int j = 0; j < accounts[index].statement.transaction_count; j++) {
                // Determine transaction type based on sign (as type is not stored in bank.h Statement)
                const char* type = (accounts[index].statement.transactions[j] >= 0) ? "Deposit" : "Withdrawal";
                double amount = (accounts[index].statement.transactions[j] >= 0) ? accounts[index].statement.transactions[j] : -accounts[index].statement.transactions[j]; // Use absolute value for display

                written += snprintf(output + written, output_size - written, "%d. %s: %.2f\n",
                                    j + 1, type, amount);
                 if (written >= output_size) return 2; // Buffer too small
            }
        }

        return 0; // Success
    }

    return 1; // Account not found or PIN incorrect
}

// Save accounts data to file
// Returns 0 on success, 1 on failure
int save_accounts_to_file(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening file for writing");
        return 1; // Failure
    }

    // Write the highest index used + 1 (which is account_count)
    fprintf(file, "%d\n", account_count);

    // Iterate through all possible slots up to account_count
    for (int i = 0; i < account_count; i++) {
         // Write account data only if the slot is active
         if (accounts[i].is_active) {
            fprintf(file, "%d\n", accounts[i].is_active); // Write active status
            fprintf(file, "%s\n", accounts[i].name);
            fprintf(file, "%s\n", accounts[i].national_id);
            fprintf(file, "%s\n", accounts[i].account_type);
            // Ensure account_number is not NULL before writing
            fprintf(file, "%s\n", accounts[i].account_number ? accounts[i].account_number : "NULL");
            fprintf(file, "%d\n", accounts[i].pin);
            fprintf(file, "%.2f\n", accounts[i].balance);
            fprintf(file, "%d\n", accounts[i].statement.transaction_count);
            for (int j = 0; j < accounts[i].statement.transaction_count; j++) {
                // Save only the amount as per bank.h Statement struct
                fprintf(file, "%.2f\n", accounts[i].statement.transactions[j]);
            }
            fprintf(file, "---\n"); // Separator
         } else {
             // If the slot is not active, still write the is_active status
             // This helps maintain the correct index when loading
             fprintf(file, "%d\n", accounts[i].is_active);
             // For simplicity in loading, we'll just write the inactive flag and separator.
             fprintf(file, "---\n");
         }
    }

    fclose(file);
    return 0; // Success
}

// Load accounts data from file
// Returns 0 on success, 1 on failure
int load_accounts_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        // No existing accounts file, start fresh. This is not an error.
        // fprintf(stderr, "No existing accounts file found. Starting again\n"); - verbose for no reason
        account_count = 0;
        // Initialize all account slots as inactive
        for(int i = 0; i < MAX_ACCOUNTS; ++i) {
            accounts[i].is_active = 0;
            accounts[i].account_number = NULL; // Ensure pointers are NULL
        }
        return 0; // Not an error if file doesn't exist
    }

    int loaded_account_count = 0;
    if (fscanf(file, "%d\n", &loaded_account_count) != 1) {
        fprintf(stderr, "Error reading number of accounts from file. Starting fresh.\n");
        fclose(file);
        account_count = 0;
         for(int i = 0; i < MAX_ACCOUNTS; ++i) {
            accounts[i].is_active = 0;
            accounts[i].account_number = NULL;
        }
        return 1; // Indicate file read error
    }

    // Cap loaded_account_count at MAX_ACCOUNTS
    if (loaded_account_count > MAX_ACCOUNTS) {
        fprintf(stderr, "Number of accounts in file (%d) exceeds MAX_ACCOUNTS (%d). Loading up to MAX_ACCOUNTS.\n", loaded_account_count, MAX_ACCOUNTS);
        loaded_account_count = MAX_ACCOUNTS;
    }

    // Initialize all account slots as inactive before loading
    for(int i = 0; i < MAX_ACCOUNTS; ++i) {
        accounts[i].is_active = 0;
        if (accounts[i].account_number != NULL) {
            free(accounts[i].account_number); // Free any existing allocated memory
            accounts[i].account_number = NULL;
        }
    }


    account_count = 0; // Reset account_count before loading

    for (int i = 0; i < loaded_account_count; i++) {
        int is_active;
        if (fscanf(file, "%d\n", &is_active) != 1) {
            fprintf(stderr, "Error reading active status for account %d. Stopping load.\n", i);
            break; // Stop loading if active status can't be read
        }

        accounts[i].is_active = is_active; // Set active status

        if (accounts[i].is_active) {
            // Read active account data
            char name_buf[MAX_NAME_LEN], nat_id_buf[MAX_ID_LEN], acc_type_buf[MAX_ACCOUNT_TYPE_LEN], acc_num_buf[50]; // Increased buffer for account number just in case

            if (fgets(name_buf, sizeof(name_buf), file) == NULL) { fprintf(stderr, "Error reading name for account %d. Stopping load.\n", i); break; }
            name_buf[strcspn(name_buf, "\n")] = 0;
            strncpy(accounts[i].name, name_buf, MAX_NAME_LEN - 1);
            accounts[i].name[MAX_NAME_LEN - 1] = '\0';

            if (fgets(nat_id_buf, sizeof(nat_id_buf), file) == NULL) { fprintf(stderr, "Error reading national ID for account %d. Stopping load.\n", i); break; }
            nat_id_buf[strcspn(nat_id_buf, "\n")] = 0;
            strncpy(accounts[i].national_id, nat_id_buf, MAX_ID_LEN - 1);
            accounts[i].national_id[MAX_ID_LEN - 1] = '\0';

            if (fgets(acc_type_buf, sizeof(acc_type_buf), file) == NULL) { fprintf(stderr, "Error reading account type for account %d. Stopping load.\n", i); break; }
            acc_type_buf[strcspn(acc_type_buf, "\n")] = 0;
            strncpy(accounts[i].account_type, acc_type_buf, MAX_ACCOUNT_TYPE_LEN - 1);
            accounts[i].account_type[MAX_ACCOUNT_TYPE_LEN - 1] = '\0';

            if (fgets(acc_num_buf, sizeof(acc_num_buf), file) == NULL) { fprintf(stderr, "Error reading account number for account %d. Stopping load.\n", i); break; }
            acc_num_buf[strcspn(acc_num_buf, "\n")] = 0;
            accounts[i].account_number = strdup(acc_num_buf);
            if(accounts[i].account_number == NULL) {
                perror("strdup failed for account number during load");
                accounts[i].is_active = 0; // Mark as inactive if allocation fails
                // Attempt to read separator line before breaking
                 char separator_buf[10];
                 fgets(separator_buf, sizeof(separator_buf), file);
                break; // Stop loading
            }

            if (fscanf(file, "%d\n", &accounts[i].pin) != 1) { fprintf(stderr, "Error reading pin for account %d. Stopping load.\n", i); free(accounts[i].account_number); accounts[i].account_number = NULL; accounts[i].is_active = 0; break; }
            if (fscanf(file, "%lf\n", &accounts[i].balance) != 1) { fprintf(stderr, "Error reading balance for account %d. Stopping load.\n", i); free(accounts[i].account_number); accounts[i].account_number = NULL; accounts[i].is_active = 0; break; }
            if (fscanf(file, "%d\n", &accounts[i].statement.transaction_count) != 1) { fprintf(stderr, "Error reading transaction count for account %d. Stopping load.\n", i); free(accounts[i].account_number); accounts[i].account_number = NULL; accounts[i].is_active = 0; break; }

            // Basic sanity check for transaction count
            if (accounts[i].statement.transaction_count < 0 || accounts[i].statement.transaction_count > MAX_TRANSACTIONS) {
                 fprintf(stderr, "Warning: Invalid transaction_count %d for account %s. Setting to 0.\n", accounts[i].statement.transaction_count, accounts[i].account_number);
                 accounts[i].statement.transaction_count = 0;
            }

            for (int j = 0; j < accounts[i].statement.transaction_count; j++) {
                // Load only the amount as per bank.h Statement struct
                if (fscanf(file, "%lf\n", &accounts[i].statement.transactions[j]) != 1) {
                    fprintf(stderr, "Error reading transaction %d amount for account %s. Truncating transactions.\n", j+1, accounts[i].account_number);
                    accounts[i].statement.transaction_count = j; // Truncate transactions
                    // Attempt to read remaining transaction lines for this account to reach separator
                    char dummy_buf[100];
                    while(j < accounts[i].statement.transaction_count) {
                         fgets(dummy_buf, sizeof(dummy_buf), file); // Read and discard
                         j++;
                    }
                    break; // Stop reading transactions for this account
                }
            }
        }

        // Read the separator line regardless of whether the account was active or not
        char separator_buf[10];
        if (fgets(separator_buf, sizeof(separator_buf), file) == NULL) {
             // If we are not at the end of the expected accounts, this is an error
             if (i < loaded_account_count - 1) {
                 fprintf(stderr, "Warning: File format error or premature EOF at account %d (expected separator).\n", i+1);
             }
             // Stop loading
             account_count = i + 1; 
             break;
        }
         if (strncmp(separator_buf, "---", 3) != 0) {
             fprintf(stderr, "Warning: Missing or incorrect separator after account %d. File might be corrupt.\n", i+1);
             // Continue loading but be aware of potential issues
         }

        account_count = i + 1;
    }

    fclose(file);
    return 0; 
}
