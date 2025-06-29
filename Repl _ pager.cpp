#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <cstdint>

using namespace std;

// === Constants and Types ===
constexpr size_t COLUMN_USERNAME_SIZE = 32;
constexpr size_t COLUMN_EMAIL_SIZE    = 255;
constexpr uint32_t PAGE_SIZE          = 4096;
constexpr uint32_t TABLE_MAX_PAGES    = 100;
constexpr uint32_t INVALID_PAGE_NUM   = UINT32_MAX;

struct InputBuffer {
    char*   buffer;
    size_t  buffer_length;
    ssize_t input_length;
};
enum MetaCommandResult { META_COMMAND_SUCCESS, META_COMMAND_UNRECOGNIZED_COMMAND };
enum PrepareResult     { PREPARE_SUCCESS, PREPARE_SYNTAX_ERROR, PREPARE_STRING_TOO_LONG, PREPARE_NEGATIVE_ID, PREPARE_UNRECOGNIZED_STATEMENT };
enum StatementType     { STATEMENT_INSERT, STATEMENT_SELECT };
enum ExecuteResult     { EXECUTE_SUCCESS, EXECUTE_DUPLICATE_KEY, EXECUTE_TABLE_FULL };

struct Row {
    uint32_t id;
    char     username[COLUMN_USERNAME_SIZE + 1];
    char     email[COLUMN_EMAIL_SIZE + 1];
};

struct Statement {
    StatementType type;
    Row           row_to_insert;
};

// === Row Serialization ===
constexpr uint32_t ID_SIZE       = sizeof(Row::id);
constexpr uint32_t USERNAME_SIZE = COLUMN_USERNAME_SIZE + 1;
constexpr uint32_t EMAIL_SIZE    = COLUMN_EMAIL_SIZE + 1;
constexpr uint32_t ROW_SIZE      = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
constexpr uint32_t ROW_OFFSET_ID = 0;
constexpr uint32_t ROW_OFFSET_USER = ID_SIZE;
constexpr uint32_t ROW_OFFSET_EMAIL = ID_SIZE + USERNAME_SIZE;

void serialize_row(const Row& source, void* dest) {
    memcpy((char*)dest + ROW_OFFSET_ID, &source.id, ID_SIZE);
    memcpy((char*)dest + ROW_OFFSET_USER, source.username, USERNAME_SIZE);
    memcpy((char*)dest + ROW_OFFSET_EMAIL,  source.email,    EMAIL_SIZE);
}

void deserialize_row(const void* src, Row& dest) {
    memcpy(&dest.id,        (char*)src + ROW_OFFSET_ID,    ID_SIZE);
    memcpy(dest.username,   (char*)src + ROW_OFFSET_USER,  USERNAME_SIZE);
    memcpy(dest.email,      (char*)src + ROW_OFFSET_EMAIL, EMAIL_SIZE);
}

// === Pager ===
typedef struct {
    int      file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void*    pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    uint32_t root_page_num;
    Pager*   pager;
} Table;

static void die(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

Pager* pager_open(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd < 0) die("Unable to open file");
    off_t file_len = lseek(fd, 0, SEEK_END);
    Pager* pager = (Pager*)malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length     = file_len;
    pager->num_pages       = file_len / PAGE_SIZE;
    if (file_len % PAGE_SIZE) die("Db file not page aligned");
    memset(pager->pages, 0, sizeof(pager->pages));
    return pager;
}

void* pager_get_page(Pager* pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) die("Page num out of bounds");
    if (!pager->pages[page_num]) {
        void* page = malloc(PAGE_SIZE);
        if (!page) die("Malloc failed");
        if (page_num < pager->num_pages) {
            ssize_t bytes = pread(pager->file_descriptor, page, PAGE_SIZE, page_num * PAGE_SIZE);
            if (bytes < 0) die("Error reading file");
        }
        pager->pages[page_num] = page;
        if (page_num >= pager->num_pages) pager->num_pages = page_num + 1;
    }
    return pager->pages[page_num];
}

void pager_flush(Pager* pager, uint32_t page_num) {
    if (!pager->pages[page_num]) return;
    ssize_t bytes = pwrite(pager->file_descriptor, pager->pages[page_num], PAGE_SIZE, page_num * PAGE_SIZE);
    if (bytes < 0) die("Error writing file");
    free(pager->pages[page_num]);
    pager->pages[page_num] = nullptr;
}

Table* db_open(const char* filename) {
    Pager* pager = pager_open(filename);
    Table* table = (Table*)malloc(sizeof(Table));
    table->pager = pager;
    table->root_page_num = 0;
    // init root as leaf
    memset(pager_get_page(pager, 0), 0, PAGE_SIZE);
    return table;
}

void db_close(Table* table) {
    Pager* pager = table->pager;
    for (uint32_t i = 0; i < pager->num_pages; ++i) pager_flush(pager, i);
    close(pager->file_descriptor);
    free(pager);
    free(table);
}

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
