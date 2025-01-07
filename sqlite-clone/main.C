#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_TABLE_NAME 32
#define MAX_COLUMN_NAME 32
#define MAX_COLUMNS 50
#define MAX_STRING_LENGTH 255
#define TABLE_MAX_PAGES 100
#define PAGE_SIZE 4096
#define SCHEMA_HEADER_SIZE (sizeof(uint32_t))  // Number of schemas
#define SCHEMA_SIZE (sizeof(TableSchema))
#define DATA_START_OFFSET (PAGE_SIZE)  // Start data after first page

typedef enum {
    COLUMN_INT,
    COLUMN_STRING,
    COLUMN_BOOL,
    COLUMN_FLOAT
} ColumnType;

typedef struct {
    char name[MAX_COLUMN_NAME];
    ColumnType type;
    uint32_t size;
    bool nullable;
} Column;

typedef struct {
    char name[MAX_TABLE_NAME];
    uint32_t num_columns;
    Column columns[MAX_COLUMNS];
    uint32_t row_size;
} TableSchema;

typedef struct {
    void* data;
    uint32_t size;
} Value;

typedef struct {
    Value** values;  // Array of pointers to values
    uint32_t num_values;
} Row;

typedef struct {
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID,
    PREPARE_DUPLICATE_TABLE,
    PREPARE_TABLE_NOT_FOUND,
    PREPARE_TYPE_MISMATCH
} PrepareResult;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY,
    EXECUTE_INVALID_EMAIL,
    EXECUTE_FAILURE
} ExecuteResult;

typedef enum {
    STATEMENT_CREATE,
    STATEMENT_INSERT,
    STATEMENT_SELECT,
    STATEMENT_DELETE,
    STATEMENT_UPDATE
} StatementType;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
    TableSchema* schemas;
    uint32_t num_schemas;
} Pager;

typedef struct {
    Pager* pager;
    char current_table[MAX_TABLE_NAME];
    uint32_t num_rows;
    uint32_t pages_count;
} Table;

typedef struct {
    StatementType type;
    char table_name[MAX_TABLE_NAME];
    Row row;
    TableSchema* schema;  // Points to the schema being operated on
    char* create_query;   // Used for CREATE TABLE
    Table* table;        // Reference to the current table
} Statement;

void serialize_schema(TableSchema* schema, void* destination) {
    memcpy(destination, schema, sizeof(TableSchema));
}

void deserialize_schema(void* source, TableSchema* schema) {
    memcpy(schema, source, sizeof(TableSchema));
}

Value* create_value(const char* str_val, Column* column) {
    Value* value = (Value*)malloc(sizeof(Value));
    value->size = column->size;
    value->data = malloc(column->size);
    
    switch (column->type) {
        case COLUMN_INT: {
            int val = atoi(str_val);
            memcpy(value->data, &val, sizeof(int));
            break;
        }
        case COLUMN_FLOAT: {
            float val = atof(str_val);
            memcpy(value->data, &val, sizeof(float));
            break;
        }
        case COLUMN_BOOL: {
            bool val = (strcasecmp(str_val, "true") == 0 || strcmp(str_val, "1") == 0);
            memcpy(value->data, &val, sizeof(bool));
            break;
        }
        case COLUMN_STRING: {
            if (strlen(str_val) >= MAX_STRING_LENGTH) {
                free(value->data);
                free(value);
                return NULL;
            }
            char* str_data = (char*)value->data;
            strncpy(str_data, str_val, MAX_STRING_LENGTH - 1);
            str_data[MAX_STRING_LENGTH - 1] = '\0';
            break;
        }
    }
    return value;
}

void print_value(Value* value, ColumnType type) {
    switch (type) {
        case COLUMN_INT:
            printf("%d", *(int*)value->data);
            break;
        case COLUMN_FLOAT:
            printf("%.2f", *(float*)value->data);
            break;
        case COLUMN_BOOL:
            printf("%s", *(bool*)value->data ? "true" : "false");
            break;
        case COLUMN_STRING:
            printf("%s", (char*)value->data);
            break;
    }
}

void* get_page(Pager* pager, uint32_t page_num) {
    if (page_num >= TABLE_MAX_PAGES) {
        printf("Tried to fetch page number out of bounds. %d >= %d\n", page_num, TABLE_MAX_PAGES);
        exit(EXIT_FAILURE);
    }

    if (pager->pages[page_num] == NULL) {
        // Cache miss. Allocate memory and load from file.
        void* page = malloc(PAGE_SIZE);
        if (!page) {
            printf("Failed to allocate memory for page\n");
            exit(EXIT_FAILURE);
        }
        memset(page, 0, PAGE_SIZE);  // Initialize to zeros

        // Calculate how many pages are in the file
        if (pager->file_length >= DATA_START_OFFSET) {
            uint32_t num_pages = (pager->file_length - DATA_START_OFFSET) / PAGE_SIZE;

            // We might save a partial page at the end of the file
            if ((pager->file_length - DATA_START_OFFSET) % PAGE_SIZE) {
                num_pages += 1;
            }

            if (page_num < num_pages) {
                off_t offset = DATA_START_OFFSET + (page_num * PAGE_SIZE);
                lseek(pager->file_descriptor, offset, SEEK_SET);
                ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
                if (bytes_read == -1) {
                    printf("Error reading file: %d\n", errno);
                    exit(EXIT_FAILURE);
                }
            }
        }

        pager->pages[page_num] = page;
    }

    return pager->pages[page_num];
}

void serialize_row(Row* row, void* destination, TableSchema* schema) {
    uint8_t* ptr = (uint8_t*)destination;
    for (uint32_t i = 0; i < row->num_values; i++) {
        memcpy(ptr, row->values[i]->data, row->values[i]->size);
        ptr += schema->columns[i].size;
    }
}

void deserialize_row(void* source, Row* row, TableSchema* schema) {
    uint8_t* ptr = (uint8_t*)source;
    row->num_values = schema->num_columns;
    row->values = (Value**)malloc(sizeof(Value*) * schema->num_columns);
    
    for (uint32_t i = 0; i < schema->num_columns; i++) {
        row->values[i] = (Value*)malloc(sizeof(Value));
        row->values[i]->size = schema->columns[i].size;
        row->values[i]->data = malloc(schema->columns[i].size);
        memcpy(row->values[i]->data, ptr, schema->columns[i].size);
        ptr += schema->columns[i].size;
    }
}

void* row_slot(Table* table, uint32_t row_num, TableSchema* schema) {
    uint32_t rows_per_page = PAGE_SIZE / schema->row_size;
    uint32_t page_num = row_num / rows_per_page;
    void* page = get_page(table->pager, page_num);
    uint32_t row_offset = row_num % rows_per_page;
    uint32_t byte_offset = row_offset * schema->row_size;
    return (uint8_t*)page + byte_offset;
}

void free_row(Row* row) {
    if (row->values) {
        for (uint32_t i = 0; i < row->num_values; i++) {
            if (row->values[i]) {
                free(row->values[i]->data);
                free(row->values[i]);
            }
        }
        free(row->values);
        row->values = NULL;
        row->num_values = 0;
    }
}

PrepareResult prepare_create_table(InputBuffer* input_buffer, Statement* statement) {
    statement->type = STATEMENT_CREATE;
    statement->create_query = strdup(input_buffer->buffer);
    
    // Skip "CREATE TABLE "
    char* ptr = input_buffer->buffer + 13;
    
    // Get table name
    char* space = strchr(ptr, ' ');
    if (!space) return PREPARE_SYNTAX_ERROR;
    
    size_t name_length = space - ptr;
    if (name_length > MAX_TABLE_NAME) return PREPARE_STRING_TOO_LONG;
    
    strncpy(statement->table_name, ptr, name_length);
    statement->table_name[name_length] = '\0';
    
    // Check if table already exists
    for (uint32_t i = 0; i < statement->table->pager->num_schemas; i++) {
        if (strcmp(statement->table->pager->schemas[i].name, statement->table_name) == 0) {
            return PREPARE_DUPLICATE_TABLE;
        }
    }
    
    return PREPARE_SUCCESS;
}

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
    statement->type = STATEMENT_INSERT;
    
    // Initialize row
    statement->row.values = NULL;
    statement->row.num_values = 0;
    
    // Parse: INSERT INTO table_name VALUES (val1, val2, ...)
    char* ptr = input_buffer->buffer;
    
    // Skip "INSERT INTO"
    ptr = strstr(ptr, "INSERT INTO");
    if (!ptr) return PREPARE_SYNTAX_ERROR;
    ptr += 11;  // Length of "INSERT INTO"
    
    // Skip whitespace
    while (*ptr == ' ') ptr++;
    
    // Get table name
    char* space = strchr(ptr, ' ');
    if (!space) return PREPARE_SYNTAX_ERROR;
    
    size_t name_length = space - ptr;
    if (name_length >= MAX_TABLE_NAME) return PREPARE_STRING_TOO_LONG;
    
    strncpy(statement->table_name, ptr, name_length);
    statement->table_name[name_length] = '\0';
    
    // Find the table schema
    statement->schema = NULL;
    for (uint32_t i = 0; i < statement->table->pager->num_schemas; i++) {
        if (strcmp(statement->table->pager->schemas[i].name, statement->table_name) == 0) {
            statement->schema = &statement->table->pager->schemas[i];
            break;
        }
    }
    if (!statement->schema) return PREPARE_TABLE_NOT_FOUND;
    
    // Skip to VALUES
    ptr = strstr(space, "VALUES");
    if (!ptr) return PREPARE_SYNTAX_ERROR;
    ptr += 6;  // Length of "VALUES"
    
    // Skip whitespace and find opening parenthesis
    while (*ptr == ' ') ptr++;
    if (*ptr != '(') return PREPARE_SYNTAX_ERROR;
    ptr++;  // Skip '('
    
    // Parse values
    Row* row = &statement->row;
    row->values = (Value**)malloc(sizeof(Value*) * statement->schema->num_columns);
    if (!row->values) {
        printf("Failed to allocate memory for row values\n");
        return PREPARE_SYNTAX_ERROR;
    }
    
    // Initialize all values to NULL
    for (uint32_t i = 0; i < statement->schema->num_columns; i++) {
        row->values[i] = NULL;
    }
    
    char value_buffer[MAX_STRING_LENGTH];
    size_t value_len = 0;
    bool in_quotes = false;
    
    while (*ptr) {
        // Skip leading whitespace
        while (*ptr == ' ') ptr++;
        
        if (*ptr == '\'') {
            in_quotes = !in_quotes;
            ptr++;
            continue;
        }
        
        if (!in_quotes && (*ptr == ',' || *ptr == ')')) {
            // End of value
            value_buffer[value_len] = '\0';
            
            // Trim whitespace
            char* start = value_buffer;
            char* end = start + value_len - 1;
            while (*start == ' ') start++;
            while (end > start && *end == ' ') end--;
            *(end + 1) = '\0';
            
            if (row->num_values >= statement->schema->num_columns) {
                free_row(row);
                return PREPARE_SYNTAX_ERROR;
            }
            
            Value* value = create_value(start, &statement->schema->columns[row->num_values]);
            if (!value) {
                free_row(row);
                return PREPARE_TYPE_MISMATCH;
            }
            
            row->values[row->num_values++] = value;
            value_len = 0;
            
            if (*ptr == ')') break;
            ptr++;
            continue;
        }
        
        if (value_len < MAX_STRING_LENGTH - 1) {
            value_buffer[value_len++] = *ptr;
        }
        ptr++;
    }
    
    if (row->num_values != statement->schema->num_columns) {
        free_row(row);
        return PREPARE_SYNTAX_ERROR;
    }
    
    return PREPARE_SUCCESS;
}

PrepareResult prepare_select(InputBuffer* input_buffer, Statement* statement) {
    statement->type = STATEMENT_SELECT;
    
    // Parse: SELECT * FROM table_name
    char* token = strtok(input_buffer->buffer, " ");  // Skip "SELECT"
    token = strtok(NULL, " ");  // Should be "*"
    if (!token || strcmp(token, "*") != 0) return PREPARE_SYNTAX_ERROR;
    
    token = strtok(NULL, " ");  // Should be "FROM"
    if (!token || strcasecmp(token, "FROM") != 0) return PREPARE_SYNTAX_ERROR;
    
    token = strtok(NULL, " ");  // Get table name
    if (!token) return PREPARE_SYNTAX_ERROR;
    
    strncpy(statement->table_name, token, MAX_TABLE_NAME);
    
    // Find the table schema
    statement->schema = NULL;
    for (uint32_t i = 0; i < statement->table->pager->num_schemas; i++) {
        if (strcmp(statement->table->pager->schemas[i].name, statement->table_name) == 0) {
            statement->schema = &statement->table->pager->schemas[i];
            break;
        }
    }
    if (!statement->schema) return PREPARE_TABLE_NOT_FOUND;
    
    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if (strncasecmp(input_buffer->buffer, "CREATE TABLE", 12) == 0) {
        return prepare_create_table(input_buffer, statement);
    }
    
    if (strncasecmp(input_buffer->buffer, "INSERT INTO", 11) == 0) {
        return prepare_insert(input_buffer, statement);
    }
    
    if (strncasecmp(input_buffer->buffer, "SELECT", 6) == 0) {
        return prepare_select(input_buffer, statement);
    }
    
    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void pager_flush_schemas(Pager* pager) {
    // Write number of schemas at the start of file
    lseek(pager->file_descriptor, 0, SEEK_SET);
    write(pager->file_descriptor, &pager->num_schemas, sizeof(uint32_t));
    
    // Write schemas after the header
    if (pager->num_schemas > 0) {
        printf("Writing %d schemas...\n", pager->num_schemas);
        
        // Allocate a buffer for all schemas
        size_t total_size = pager->num_schemas * sizeof(TableSchema);
        uint8_t* buffer = (uint8_t*)malloc(total_size);
        if (!buffer) {
            printf("Failed to allocate schema buffer\n");
            exit(EXIT_FAILURE);
        }
        
        // Serialize each schema
        for (uint32_t i = 0; i < pager->num_schemas; i++) {
            serialize_schema(&pager->schemas[i], buffer + (i * sizeof(TableSchema)));
            printf("Schema %d: %s (%d columns)\n", i, pager->schemas[i].name, pager->schemas[i].num_columns);
        }
        
        // Write all schemas at once
        lseek(pager->file_descriptor, SCHEMA_HEADER_SIZE, SEEK_SET);
        ssize_t bytes_written = write(pager->file_descriptor, buffer, total_size);
        free(buffer);
        
        if (bytes_written == -1) {
            printf("Error writing schemas: %d\n", errno);
            exit(EXIT_FAILURE);
        }
        printf("Wrote %ld bytes of schema data\n", bytes_written);
    }
}

ExecuteResult execute_create_table(Statement* statement, Table* table) {
    TableSchema* schema = &table->pager->schemas[table->pager->num_schemas];
    strcpy(schema->name, statement->table_name);
    
    char* ptr = strstr(statement->create_query, "(");
    if (!ptr) return EXECUTE_FAILURE;
    ptr++;
    
    uint32_t column_index = 0;
    uint32_t row_size = 0;
    
    while (*ptr && *ptr != ')') {
        // Skip whitespace
        while (*ptr == ' ' || *ptr == '\n') ptr++;
        
        Column* column = &schema->columns[column_index];
        
        // Get column name
        char* space = strchr(ptr, ' ');
        if (!space) return EXECUTE_FAILURE;
        
        size_t name_length = space - ptr;
        if (name_length > MAX_COLUMN_NAME) return EXECUTE_FAILURE;
        
        strncpy(column->name, ptr, name_length);
        column->name[name_length] = '\0';
        ptr = space + 1;
        
        // Get column type
        if (strncasecmp(ptr, "INT", 3) == 0) {
            column->type = COLUMN_INT;
            column->size = sizeof(int);
            ptr += 3;
        } else if (strncasecmp(ptr, "STRING", 6) == 0) {
            column->type = COLUMN_STRING;
            column->size = MAX_STRING_LENGTH;
            ptr += 6;
        } else if (strncasecmp(ptr, "BOOL", 4) == 0) {
            column->type = COLUMN_BOOL;
            column->size = sizeof(bool);
            ptr += 4;
        } else if (strncasecmp(ptr, "FLOAT", 5) == 0) {
            column->type = COLUMN_FLOAT;
            column->size = sizeof(float);
            ptr += 5;
        } else {
            return EXECUTE_FAILURE;
        }
        
        row_size += column->size;
        column_index++;
        
        // Skip to next column or end
        while (*ptr == ' ' || *ptr == ',') ptr++;
    }
    
    schema->num_columns = column_index;
    schema->row_size = row_size;
    table->pager->num_schemas++;
    
    printf("Table '%s' created with %d columns.\n", statement->table_name, column_index);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(Statement* statement, Table* table) {
    Row* row = &statement->row;
    TableSchema* schema = statement->schema;
    
    if (!row->values || !schema) {
        return EXECUTE_FAILURE;
    }
    
    if (table->num_rows >= TABLE_MAX_PAGES * (PAGE_SIZE / schema->row_size)) {
        free_row(row);
        return EXECUTE_TABLE_FULL;
    }
    
    void* slot = row_slot(table, table->num_rows, schema);
    serialize_row(row, slot, schema);
    table->num_rows++;
    
    printf("Inserted %d values.\n", row->num_values);
    
    // Clean up row data after successful insert
    free_row(row);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_select(Statement* statement, Table* table) {
    TableSchema* schema = statement->schema;
    Row row;
    
    // Print header
    for (uint32_t i = 0; i < schema->num_columns; i++) {
        printf("%s%s", i > 0 ? " | " : "", schema->columns[i].name);
    }
    printf("\n");
    
    // Print separator
    for (uint32_t i = 0; i < schema->num_columns; i++) {
        if (i > 0) printf("-+-");
        for (uint32_t j = 0; j < strlen(schema->columns[i].name); j++) {
            printf("-");
        }
    }
    printf("\n");
    
    // Print rows
    for (uint32_t i = 0; i < table->num_rows; i++) {
        deserialize_row(row_slot(table, i, schema), &row, schema);
        
        for (uint32_t j = 0; j < row.num_values; j++) {
            if (j > 0) printf(" | ");
            print_value(row.values[j], schema->columns[j].type);
        }
        printf("\n");
        
        // Free row data
        free_row(&row);
    }
    
    printf("\n(%d rows)\n", table->num_rows);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case STATEMENT_CREATE:
            return execute_create_table(statement, table);
        case STATEMENT_INSERT:
            return execute_insert(statement, table);
        case STATEMENT_SELECT:
            return execute_select(statement, table);
        case STATEMENT_DELETE:
        case STATEMENT_UPDATE:
            printf("Operation not implemented yet.\n");
            return EXECUTE_SUCCESS;
    }
    return EXECUTE_SUCCESS;
}

TableSchema* get_table_schema(Pager* pager, const char* table_name) {
    for (uint32_t i = 0; i < pager->num_schemas; i++) {
        if (strcmp(pager->schemas[i].name, table_name) == 0) {
            return &pager->schemas[i];
        }
    }
    return NULL;
}

void pager_flush(Pager* pager, uint32_t page_num, uint32_t size) {
    if (pager->pages[page_num] == NULL) {
        return;
    }

    off_t offset = DATA_START_OFFSET + (page_num * PAGE_SIZE);
    lseek(pager->file_descriptor, offset, SEEK_SET);
    
    ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);
    if (bytes_written == -1) {
        printf("Error writing: %d\n", errno);
        exit(EXIT_FAILURE);
    }
}

Pager* pager_open(const char* filename) {
    int fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    
    if (fd == -1) {
        printf("Unable to open file\n");
        exit(EXIT_FAILURE);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);

    Pager* pager = (Pager*)malloc(sizeof(Pager));
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->num_schemas = 0;
    
    // Allocate memory for schemas
    pager->schemas = (TableSchema*)malloc(sizeof(TableSchema) * TABLE_MAX_PAGES);
    if (!pager->schemas) {
        printf("Failed to allocate schema memory\n");
        exit(EXIT_FAILURE);
    }
    memset(pager->schemas, 0, sizeof(TableSchema) * TABLE_MAX_PAGES);
    
    // Initialize pages to NULL
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    // Read schemas if file is not empty
    if (file_length > SCHEMA_HEADER_SIZE) {
        // Read number of schemas
        lseek(fd, 0, SEEK_SET);
        ssize_t bytes_read = read(fd, &pager->num_schemas, sizeof(uint32_t));
        if (bytes_read != sizeof(uint32_t)) {
            printf("Error reading schema count\n");
            exit(EXIT_FAILURE);
        }
        
        if (pager->num_schemas > TABLE_MAX_PAGES) {
            printf("Too many schemas in file: %d\n", pager->num_schemas);
            exit(EXIT_FAILURE);
        }
        
        printf("Found %d schemas\n", pager->num_schemas);
        
        // Read schemas
        if (pager->num_schemas > 0) {
            // Allocate a buffer for all schemas
            size_t total_size = pager->num_schemas * sizeof(TableSchema);
            uint8_t* buffer = (uint8_t*)malloc(total_size);
            if (!buffer) {
                printf("Failed to allocate schema buffer\n");
                exit(EXIT_FAILURE);
            }
            
            // Read all schemas at once
            lseek(fd, SCHEMA_HEADER_SIZE, SEEK_SET);
            bytes_read = read(fd, buffer, total_size);
            if (bytes_read != total_size) {
                printf("Error reading schemas: expected %ld bytes, got %ld\n", total_size, bytes_read);
                exit(EXIT_FAILURE);
            }
            
            // Deserialize each schema
            for (uint32_t i = 0; i < pager->num_schemas; i++) {
                deserialize_schema(buffer + (i * sizeof(TableSchema)), &pager->schemas[i]);
                printf("Schema %d: %s (%d columns)\n", 
                    i, pager->schemas[i].name, pager->schemas[i].num_columns);
            }
            
            free(buffer);
            printf("Read %ld bytes of schema data\n", bytes_read);
        }
    }

    return pager;
}

Table* db_open(const char* filename) {
    Pager* pager = pager_open(filename);
    Table* table = (Table*)malloc(sizeof(Table));
    table->pager = pager;
    table->num_rows = 0;  // Initialize to 0
    
    // If we have schemas, use the first one to calculate rows
    if (pager->num_schemas > 0) {
        TableSchema* first_schema = &pager->schemas[0];
        if (first_schema->row_size > 0 && pager->file_length > DATA_START_OFFSET) {
            uint32_t data_size = pager->file_length - DATA_START_OFFSET;
            table->num_rows = data_size / first_schema->row_size;
            
            // Don't let num_rows exceed what we can handle
            uint32_t max_rows = TABLE_MAX_PAGES * (PAGE_SIZE / first_schema->row_size);
            if (table->num_rows > max_rows) {
                table->num_rows = max_rows;
            }
            
            printf("Database file size: %u bytes\n", (uint32_t)pager->file_length);
            printf("Data section size: %u bytes\n", data_size);
            printf("Row size: %d bytes\n", first_schema->row_size);
            printf("Calculated %d rows\n", table->num_rows);
        }
    }
    
    strcpy(table->current_table, "");
    return table;
}

void db_close(Table* table) {
    Pager* pager = table->pager;

    // Flush schemas first
    pager_flush_schemas(pager);
    
    // Calculate number of full pages
    uint32_t num_rows_per_page = PAGE_SIZE / table->pager->schemas[0].row_size;
    uint32_t num_full_pages = table->num_rows / num_rows_per_page;

    for (uint32_t i = 0; i < num_full_pages; i++) {
        if (pager->pages[i] == NULL) {
            continue;
        }
        pager_flush(pager, i, PAGE_SIZE);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    // There may be a partial page to write to the end of the file
    uint32_t num_additional_rows = table->num_rows % num_rows_per_page;
    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        if (pager->pages[page_num] != NULL) {
            pager_flush(pager, page_num, num_additional_rows * table->pager->schemas[0].row_size);
            free(pager->pages[page_num]);
            pager->pages[page_num] = NULL;
        }
    }

    // Free remaining pages
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        void* page = pager->pages[i];
        if (page) {
            free(page);
            pager->pages[i] = NULL;
        }
    }

    close(pager->file_descriptor);
    free(pager->schemas);
    free(pager);
    free(table);
}

void print_prompt() { 
    printf("db > "); 
}

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

void read_input(InputBuffer* input_buffer) {
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    // Ignore trailing newline
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
}

void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        db_close(table);
        exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;
    return input_buffer;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Must supply a database filename.\n");
        exit(EXIT_FAILURE);
    }

    char* filename = argv[1];
    Table* table = db_open(filename);

    InputBuffer* input_buffer = new_input_buffer();
    while (true) {
        print_prompt();
        read_input(input_buffer);
        
        if (input_buffer->buffer[0] == '.') {
            switch (do_meta_command(input_buffer, table)) {
                case (META_COMMAND_SUCCESS):
                    continue;
                case (META_COMMAND_UNRECOGNIZED_COMMAND):
                    printf("Unrecognized command '%s'\n", input_buffer->buffer);
                    continue;
            }
        }
        
        Statement statement;
        statement.table = table;
        switch (prepare_statement(input_buffer, &statement)) {
            case (PREPARE_SUCCESS):
                break;
            case (PREPARE_SYNTAX_ERROR):
                printf("Syntax error. Could not parse statement.\n");
                continue;
            case (PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n",
                       input_buffer->buffer);
                continue;
            case (PREPARE_STRING_TOO_LONG):
                printf("String is too long.\n");
                continue;
            case (PREPARE_NEGATIVE_ID):
                printf("ID must be positive.\n");
                continue;
            case (PREPARE_DUPLICATE_TABLE):
                printf("Table already exists.\n");
                continue;
            case (PREPARE_TABLE_NOT_FOUND):
                printf("Table not found.\n");
                continue;
            case (PREPARE_TYPE_MISMATCH):
                printf("Type mismatch.\n");
                continue;
        }
        
        switch (execute_statement(&statement, table)) {
            case (EXECUTE_SUCCESS):
                printf("Executed.\n");
                break;
            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full.\n");
                break;
            case (EXECUTE_DUPLICATE_KEY):
                printf("Error: Duplicate key.\n");
                break;
            case (EXECUTE_INVALID_EMAIL):
                printf("Error: Invalid email format.\n");
                break;
            case (EXECUTE_FAILURE):
                printf("Error: Unknown error.\n");
                break;
        }
    }
}
