#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdint>

using namespace std;

// === REPL ===
MetaCommandResult do_meta_command(InputBuffer* ib, Table* table) {
    if (strcmp(ib->buffer, ".exit") == 0) {
        db_close(table);
        exit(EXIT_SUCCESS);
    }
    return META_COMMAND_UNRECOGNIZED_COMMAND;
}

PrepareResult prepare_statement(InputBuffer* ib, Statement* stmt) {
    if (strncmp(ib->buffer, "insert", 6) == 0) {
        stmt->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if (strcmp(ib->buffer, "select") == 0) {
        stmt->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_statement(Statement* stmt, Table* table) {
    // stub: always success
    return EXECUTE_SUCCESS;
}

void print_prompt() { cout << "db > "; }

void read_input(InputBuffer* ib) {
    ssize_t bytes = getline(&ib->buffer, &ib->buffer_length, stdin);
    if (bytes <= 0) die("Error reading input");
    ib->input_length = bytes - 1;
    ib->buffer[bytes - 1] = '\0';
}

InputBuffer* new_input_buffer() {
    return new InputBuffer{nullptr, 0, 0};
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Must supply a database filename.\n";
        exit(EXIT_FAILURE);
    }
    Table* table = db_open(argv[1]);
    InputBuffer* ib = new_input_buffer();

    while (true) {
        print_prompt();
        read_input(ib);
        if (ib->buffer[0] == '.') {
            if (do_meta_command(ib, table) != META_COMMAND_SUCCESS) {
                cout << "Unrecognized command '" << ib->buffer << "'\n";
            }
            continue;
        }
        Statement stmt;
        switch (prepare_statement(ib, &stmt)) {
            case PREPARE_SUCCESS: break;
            case PREPARE_SYNTAX_ERROR:
                cout << "Syntax error.\n";
                continue;
            default:
                cout << "Unrecognized keyword at start of '" << ib->buffer << "'\n";
                continue;
        }
        switch (execute_statement(&stmt, table)) {
            case EXECUTE_SUCCESS:
                cout << "Executed.\n";
                break;
            case EXECUTE_DUPLICATE_KEY:
                cout << "Error: Duplicate key.\n";
                break;
            case EXECUTE_TABLE_FULL:
                cout << "Error: Table full.\n";
                break;
        }
    }
}
