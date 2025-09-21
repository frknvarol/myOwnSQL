#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "statement.h"

#include <stdbool.h>

#include "input_buffer.h"
#include "table.h"
#include "database.h"
#include "lexer.h"
#include "binary_plus_tree.h"


Database global_db;


PrepareResult prepare_statement(const InputBuffer* input_buffer, Statement* statement) {

    Lexer lexer;
    init_lexer(&lexer, input_buffer->buffer);
    Token token = next_token(&lexer);

    if (token.type == TOKEN_INSERT) {
        InsertStatement insert_statement;
        token = next_token(&lexer);
        if (token.type != TOKEN_INTO) return PREPARE_SYNTAX_ERROR;

        token = next_token(&lexer);
        if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
        strncpy(insert_statement.table_name, token.text, sizeof(insert_statement.table_name));

        const Table* table = find_table(&global_db, token.text);
        const TableSchema schema = table->schema;

        const Row* row = create_row(&table->schema);
        insert_statement.row = *row;


        token = next_token(&lexer);
        if (token.type != TOKEN_VALUES) return PREPARE_SYNTAX_ERROR;

        token = next_token(&lexer);
        if (token.type != TOKEN_OPEN_PAREN || strcmp(token.text, "(") != 0)
            return PREPARE_SYNTAX_ERROR;


        int col_index = 0;
        while (true) {
            token = next_token(&lexer);

            const ColumnType column_type = schema.columns[col_index].type;

            switch (column_type) {
                case COLUMN_INT: {
                    char *endptr;

                    const long int num = strtol(token.text, &endptr, 10);
                    if (endptr == token.text || *endptr != '\0' || token.type != TOKEN_NUMBER) {
                        return PREPARE_INSERT_TYPE_ERROR;
                    }
                    set_int_value(&table->schema, &insert_statement.row, col_index, (int32_t)num);
                    break;
                }
                case COLUMN_VARCHAR: {
                    if (token.type != TOKEN_STRING) return PREPARE_INSERT_TYPE_ERROR;
                    if (schema.columns[col_index].size < strlen(token.text)) return PREPARE_INSERT_VARCHAR_SIZE_ERROR;
                    set_text_value(&table->schema, &insert_statement.row, col_index, token.text);
                    break;
                }
                default:
                    return PREPARE_SYNTAX_ERROR;
            }

            col_index++;

            const Token next = next_token(&lexer);
            if (strcmp(next.text, ")") == 0) break;
            if (next.type != TOKEN_COMMA) return PREPARE_SYNTAX_ERROR;
        }

        statement->type = STATEMENT_INSERT;
        statement->insert_stmt = insert_statement;
        return PREPARE_SUCCESS;
    }

    if (token.type == TOKEN_SELECT) {

        SelectStatement select_statement;
        select_statement.selected_col_count = 0;

        Lexer selected_col_lexer = lexer;
        token = next_token(&lexer);

        while (token.type != TOKEN_FROM) {
            token = next_token(&lexer);
        }

        token = next_token(&lexer);
        strncpy(select_statement.table_name, token.text, sizeof(select_statement.table_name));

        Table* table = find_table(&global_db, token.text);
        const TableSchema schema = table->schema;


        token = next_token(&lexer);
        if (token.type == TOKEN_WHERE) {
            select_statement.condition_count = 0;
            uint32_t index = 0;
            while (true) {

                token = next_token(&lexer);
                if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
                select_statement.conditions[index].column_name = strdup(token.text);

                token = next_token(&lexer);
                if (token.type != TOKEN_EQUAL) return PREPARE_SYNTAX_ERROR;

                token = next_token(&lexer);
                if (token.type != TOKEN_NUMBER && token.type != TOKEN_STRING) return PREPARE_SYNTAX_ERROR;
                select_statement.conditions[index].value = strdup(token.text);

                index++;
                select_statement.condition_count++;

                token = next_token(&lexer);
                if (token.type == TOKEN_SEMICOLON) break;
                if (token.type == TOKEN_EOF) break;
                if (token.type != TOKEN_AND) break;

            }
            select_statement.has_condition = 1;
        }
        else select_statement.has_condition = 0;

        Token col_token = next_token(&selected_col_lexer);
        if (col_token.type != TOKEN_STAR) {
            uint32_t col_index = 0;
            while (col_token.type != TOKEN_FROM) {
                if (col_token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
                for (uint32_t i = 0; i < schema.num_columns; i++ ) {
                    if (strcmp(schema.columns[i].name, col_token.text) == 0) {
                        select_statement.selected_col_indexes[col_index] = i;
                        col_index++;
                    }

                    if (select_statement.has_condition) {
                        for (uint32_t j = 0; j < select_statement.condition_count; j++ ) {
                            if (strcmp(schema.columns[i].name, select_statement.conditions[j].column_name) == 0) {
                                select_statement.conditions[j].column_index = i;
                            }
                        }
                    }
                }
                col_token = next_token(&selected_col_lexer);

                if (col_token.type == TOKEN_FROM) break;
                if (col_token.type != TOKEN_COMMA) return PREPARE_SYNTAX_ERROR;

                col_token = next_token(&selected_col_lexer);
            }

            select_statement.selected_col_count = col_index;
        }
        else {

            select_statement.selected_col_count = 0;
            for (uint32_t i = 0; i < schema.num_columns; i++ ) {

                if (select_statement.has_condition) {
                    for (uint32_t j = 0; j < select_statement.condition_count; j++ ) {
                        if (strcmp(schema.columns[i].name, select_statement.conditions[j].column_name) == 0) {
                            select_statement.conditions[j].column_index = i;
                        }
                    }
                }
            }

        }




        statement->type = STATEMENT_SELECT;
        statement->select_stmt = select_statement;

        return PREPARE_SUCCESS;
    }


    if (token.type == TOKEN_CREATE) {

        token = next_token(&lexer);

        if (token.type == TOKEN_TABLE) {
            CreateTableStatement create_statement;

            token = next_token(&lexer);
            if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;
            strncpy(create_statement.table_name, token.text, sizeof(create_statement.table_name));

            token = next_token(&lexer);
            if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;

            char* open_paren = strchr(input_buffer->buffer, '(');
            if (!open_paren) return PREPARE_SYNTAX_ERROR;

            if (open_paren != strstr(input_buffer->buffer, token.text)) {
                return PREPARE_SYNTAX_ERROR;
            }

            char* close_paren = find_close_parenthesis(open_paren);
            if (!close_paren) return PREPARE_SYNTAX_ERROR;


            char column_definitions[256];
            strncpy(column_definitions, open_paren+ 1, close_paren - open_paren - 1);
            column_definitions[close_paren - open_paren - 1] = '\0';

            create_statement.num_columns = 0;
            while (true) {
                token = next_token(&lexer);

                if (token.type == TOKEN_IDENTIFIER) {
                    strcpy(create_statement.columns[create_statement.num_columns].name, token.text);
                    create_statement.columns[create_statement.num_columns].index = create_statement.num_columns;

                    token = next_token(&lexer);
                    if (token.type == TOKEN_INT) {
                        create_statement.columns[create_statement.num_columns].type = COLUMN_INT;
                        create_statement.num_columns++;
                    }

                    else if (token.type == TOKEN_VARCHAR) {

                        token = next_token(&lexer);

                        if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;

                        token = next_token(&lexer);

                        if (token.type != TOKEN_NUMBER) return PREPARE_SYNTAX_ERROR;

                        char *endptr;
                        const long val = strtol(token.text, &endptr, 10);
                        if (endptr == token.text || *endptr != '\0' || val > UINT32_MAX) return PREPARE_SYNTAX_ERROR;

                        uint32_t size = (uint32_t)val;

                        create_statement.columns[create_statement.num_columns].type = COLUMN_VARCHAR;
                        create_statement.columns[create_statement.num_columns].size = size;

                        token = next_token(&lexer);
                        if (token.type != TOKEN_CLOSE_PAREN) return PREPARE_SYNTAX_ERROR;
                        create_statement.num_columns++;


                    }
                }

                else if (token.type == TOKEN_PRIMARY) {
                    token = next_token(&lexer);
                    if (token.type != TOKEN_KEY) return PREPARE_SYNTAX_ERROR;

                    token = next_token(&lexer);
                    if (token.type != TOKEN_OPEN_PAREN) return PREPARE_SYNTAX_ERROR;


                    token = next_token(&lexer);
                    if (token.type != TOKEN_IDENTIFIER) return PREPARE_SYNTAX_ERROR;


                    create_statement.primary_col_index = create_statement.num_columns;

                    token = next_token(&lexer);
                    if (token.type != TOKEN_CLOSE_PAREN) return PREPARE_SYNTAX_ERROR;

                }


                token = next_token(&lexer);

                if (token.type == TOKEN_CLOSE_PAREN) break;
                if (token.type != TOKEN_COMMA) return PREPARE_SYNTAX_ERROR;
            }

            statement->type = STATEMENT_CREATE_TABLE;

            statement->create_table_stmt = create_statement;
            return PREPARE_SUCCESS;
        }

        //TODO add searching and selecting a database
        if (token.type == TOKEN_DATABASE) {
            token = next_token(&lexer);

            Database db;
            init_database(&db, token.text);

        }

    }

    if (token.type == TOKEN_DROP) {
        token = next_token(&lexer);
        if (token.type != TOKEN_TABLE) return PREPARE_SYNTAX_ERROR;
        token = next_token(&lexer);

        DropTableStatement drop_table_statement;
        drop_table_statement.table_name = token.text;

        statement->type = STATEMENT_DROP_TABLE;
        statement->drop_table_stmt = drop_table_statement;
        return PREPARE_SUCCESS;
    }

    if (token.type == TOKEN_SHOW) {
        token = next_token(&lexer);
        if (token.type != TOKEN_TABLES) return PREPARE_SYNTAX_ERROR;
        token = next_token(&lexer);
        if (token.type != TOKEN_EOF && token.type != TOKEN_SEMICOLON) return PREPARE_SYNTAX_ERROR;

        statement->type = STATEMENT_SHOW_TABLES;
        return PREPARE_SUCCESS;

    }


    return PREPARE_UNRECOGNIZED_STATEMENT;
}


void serialize_row(const TableSchema* schema, const Row* source, void* destination) {
    size_t offset = 0;
    for (int i = 0; i < schema->num_columns; i++) {
        size_t col_size = (schema->columns[i].type == COLUMN_INT) ? sizeof(int32_t) : 256;
        memcpy((char*)destination + offset, source->data + offset, col_size);
        offset += col_size;
    }
}

void deserialize_row(const TableSchema* schema, void* source, Row* destination) {
    size_t offset = 0;
    for (int i = 0; i < schema->num_columns; i++) {
        size_t col_size = (schema->columns[i].type == COLUMN_INT) ? sizeof(int32_t) : 256;
        memcpy(destination->data + offset, (char*)source + offset, col_size);
        offset += col_size;
    }
}

// Table row allocator function
void* row_slot(Table* table, uint32_t row_num) {
    size_t row_size = compute_row_size(&table->schema);
    size_t rows_per_page = PAGE_SIZE / row_size;

    size_t page_num = row_num / rows_per_page;
    if (table->pages[page_num] == NULL) {
        table->pages[page_num] = malloc(PAGE_SIZE);
        if (!table->pages[page_num]) {
            perror("malloc failed");
            exit(1);
        }
    }
    void* page = table->pages[page_num];


    size_t row_offset = row_num % rows_per_page;
    size_t byte_offset = row_offset * row_size;
    return (char*)page + byte_offset;
}




ExecuteResult execute_statement(Statement* statement) {
    switch (statement->type) {
        case STATEMENT_INSERT:
            return execute_insert(&statement->insert_stmt);
        case STATEMENT_SELECT:
            return execute_select(&statement->select_stmt);
        case STATEMENT_CREATE_TABLE:
            return execute_create_table(&statement->create_table_stmt);
        case STATEMENT_DROP_TABLE:
            return execute_drop_table(&statement->drop_table_stmt);
        case STATEMENT_SHOW_TABLES:
            return execute_show_tables(&statement->show_tables_stmt);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(InsertStatement* insert_statement) {
    Table* table = find_table(&global_db, insert_statement->table_name);

    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_FAIL;
    }

    Row* row_to_insert = &(insert_statement->row);
    void* destination = row_slot(table, table->num_rows);

    serialize_row(&table->schema, row_to_insert, destination);
    table->num_rows += 1;

    // TODO as i have not implemented a primary key attribute i will for now use the row number as the key for bpt
    //bpt_insert(table->tree, (int)table->primary_key_index, destination);

    //int key = extract_primary_key(&table->schema, row_to_insert, table->primary_key_index);
    //bpt_insert(table->tree, key, destination);


    return EXECUTE_SUCCESS;
}

// TODO implement BPT search (first i have to get the lexer to do column search and where statements)
ExecuteResult execute_select(const SelectStatement* select_statement) {
    Table* table = find_table(&global_db, select_statement->table_name);
    TableSchema* schema = &table->schema;
    Row row;
    row.data = malloc(compute_row_size(&table->schema));


    if (select_statement->selected_col_count == 0) {
        printf("COLUMNS:\n");
        printf("(");
        for (int i = 0; i < schema->num_columns; i++) {
            if (i > 0) printf(", ");
            printf("%s", schema->columns[i].name);
        }
        printf(")\n\n");
    } else {
        printf("COLUMNS:\n");
        printf("(");
        for (uint32_t i = 0; i < select_statement->selected_col_count; i++) {
            if (i > 0) printf(", ");
            uint32_t index = select_statement->selected_col_indexes[i];
            printf("%s", schema->columns[index].name);
        }
        printf(")\n\n");
    }

    if (select_statement->has_condition) {
        for (uint32_t i = 0; i < table->num_rows; i++) {
            deserialize_row(&table->schema, row_slot(table, i), &row);
            int has_conditions = 1;
            for (uint32_t condition_index = 0; condition_index < select_statement->condition_count; condition_index++) {
                if (!has_conditions) continue;
                uint32_t index = select_statement->conditions[condition_index].column_index;
                if (schema->columns[index].type == COLUMN_INT) {
                    int32_t val;
                    memcpy(&val, row.data + get_column_offset(schema, index), sizeof(int32_t));

                    char *endptr;
                    char* value = select_statement->conditions[condition_index].value;

                    const long int num = strtol(value, &endptr, 10);
                    if (endptr == value || *endptr != '\0') continue;
                    if (num != val) has_conditions = 0;


                }
                else if (schema->columns[index].type == COLUMN_VARCHAR) {
                    char buf[257];
                    memcpy(buf, row.data + get_column_offset(schema, index), 256);
                    if (strcmp(buf, select_statement->conditions[condition_index].value) != 0) has_conditions = 0;

                }
            }

            if (has_conditions) {
                print_row(&table->schema, &row, select_statement);
            }

        }
        return EXECUTE_SUCCESS;


    }

    for (uint32_t i = 0; i < table->num_rows; i++) {
        deserialize_row(&table->schema, row_slot(table, i), &row);
        print_row(&table->schema, &row, select_statement);
    }
    free(row.data);
    return EXECUTE_SUCCESS;

}

ExecuteResult execute_create_table(const CreateTableStatement* create_statement) {
    Table* new_table = malloc(sizeof(Table));
    if (!new_table) {
        printf("Error: memory allocation failed.\n");
        return EXECUTE_FAIL;
    }

    strncpy(new_table->name, create_statement->table_name, sizeof(new_table->name));
    new_table->schema.num_columns = create_statement->num_columns;
    for (int i = 0; i < create_statement->num_columns; i++) {
        new_table->schema.columns[i] = create_statement->columns[i];
    }

    //TODO don't forget to add free_table function to DROP TABLE statement when you implement it
    new_table->tree = malloc(sizeof(BPTree));
    new_table->tree->root = NULL;

    new_table->num_rows = 0;
    new_table->primary_key_index = create_statement->primary_col_index;


    if (add_table(&global_db, new_table) != 0) {
        free(new_table);
        return EXECUTE_FAIL;
    }

    printf("Table %s created with %d columns.\n",
           new_table->name, new_table->schema.num_columns);
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_drop_table(const DropTableStatement* drop_table_statement) {
    Table* table = find_table(&global_db, drop_table_statement->table_name);
    if (table == NULL) {
        return EXECUTE_FAIL;
    }
    free_table(table);

    for (uint32_t i = 0; i < global_db.num_tables; i++) {
        if (global_db.tables[i] == table) {
            for (uint32_t j = i; j < global_db.num_tables - 1; j++) {
                global_db.tables[j] = global_db.tables[j + 1];
            }
            global_db.tables[global_db.num_tables - 1] = NULL;
            global_db.num_tables--;
            break;
        }
    }

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_show_tables() {
    for (uint32_t i = 0; i < global_db.num_tables; i++) {
        printf("%s\n", global_db.tables[i]->name);
    }
    return EXECUTE_SUCCESS;
}


void print_row(const TableSchema* schema, const Row* row, const SelectStatement* select_statement) {

    printf("(");
    if (select_statement->selected_col_count == 0) {
        for (int i = 0; i < schema->num_columns; i++) {
            if (i > 0) printf(", ");
            if (schema->columns[i].type == COLUMN_INT) {
                int32_t val;
                memcpy(&val, row->data + get_column_offset(schema, i), sizeof(int32_t));
                printf("%d", val);
            } else if (schema->columns[i].type == COLUMN_VARCHAR){
                char buf[257];
                memcpy(buf, row->data + get_column_offset(schema, i), 256);
                buf[256] = '\0';
                printf("%s", buf);
            }
        }
    } else {
        for (uint32_t i = 0; i < select_statement->selected_col_count; i++) {
            if (i > 0) printf(", ");
            uint32_t index = select_statement->selected_col_indexes[i];
            if (schema->columns[index].type == COLUMN_INT) {
                int32_t val;
                memcpy(&val, row->data + get_column_offset(schema, index), sizeof(int32_t));
                printf("%d", val);
            } else if (schema->columns[index].type == COLUMN_VARCHAR){
                char buf[257];
                memcpy(buf, row->data + get_column_offset(schema, index), 256);
                buf[256] = '\0';
                printf("%s", buf);
            }
        }
    }
    printf(")\n");

}



char* find_close_parenthesis(char* open_parenthesis) {
    /*Function to find the matching closing parenthesis of an open parenthesis*/
    int open_paren_count = 0;

    while (*open_parenthesis != '\0') {
        open_parenthesis++;

        if (*open_parenthesis == '(') {
            open_paren_count++;
        } else if (*open_parenthesis == ')') {
            if (open_paren_count == 0) {
                return open_parenthesis;
            }
            open_paren_count--;
        }
    }

    return NULL;
}
