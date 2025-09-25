#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "statement.h"
#include "parser.h"
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

    switch (token.type) {
        case TOKEN_INSERT:
            return parse_insert(&lexer, statement, token);
        case TOKEN_SELECT:
            return parse_select(&lexer, statement, token);
        case TOKEN_CREATE:
            return parse_create(&lexer, statement, input_buffer, token);
        case TOKEN_DROP:
            return parse_drop(&lexer, statement, token);
        case TOKEN_SHOW:
            return parse_show(&lexer, statement, token);
        case TOKEN_DELETE:
            return parse_delete(&lexer, statement, token);
        default:
            return PREPARE_UNRECOGNIZED_STATEMENT;
    }

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
            return execute_show_tables();
        case STATEMENT_DELETE:
            return execute_delete(&statement->delete_stmt);
        case STATEMENT_CREATE_DATABASE:
            printf("CREATE DATABASE (to be completed)\n");
            return EXECUTE_SUCCESS;
        case STATEMENT_SHOW_DATABASES:
            printf("SHOW DATABASES (to be completed)\n");
            return EXECUTE_SUCCESS;
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
            void* row_ptr = row_slot(table, i);
            //uint8_t deleted = *(uint8_t*)row_ptr;
            if (*(uint8_t*)row_ptr) continue;

            deserialize_row(&table->schema, row_slot(table, i), &row);
            int has_conditions = 1;
            for (uint32_t condition_index = 0; condition_index < select_statement->condition_count; condition_index++) {
                if (!has_conditions) continue;
                uint32_t index = select_statement->conditions[condition_index].column_index;
                if (schema->columns[index].type == COLUMN_INT) {
                    int32_t val;
                    memcpy(&val, row.data + get_column_offset(schema, index), sizeof(int32_t));

                    char *endptr;
                    const char* value = select_statement->conditions[condition_index].value;

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
        free(row.data);
        return EXECUTE_SUCCESS;


    }

    for (uint32_t i = 0; i < table->num_rows; i++) {
        void* row_ptr = row_slot(table, i);
        //uint8_t deleted = *(uint8_t*)row_ptr;
        if (*(uint8_t*)row_ptr) continue;
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
        printf("Table not found.\n");
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


ExecuteResult execute_delete(const DeleteStatement* delete_statement) {
    Table* table = find_table(&global_db, delete_statement->table_name);
    const TableSchema* schema = &table->schema;

    for (uint32_t row_index = 0; row_index < table->num_rows; row_index++) {
        void* row_ptr = row_slot(table, row_index);

        //skips already deleted rows
        if (*(uint8_t*)row_ptr) continue;

        if (delete_statement->has_condition) {
            int has_conditions = 1;

            for (uint32_t condition_index = 0; condition_index < delete_statement->condition_count; condition_index ++) {
                uint32_t col_index = delete_statement->conditions[condition_index].column_index;

                if (schema->columns[col_index].type == COLUMN_INT) {
                    int32_t val;
                    memcpy(&val, (char*)row_ptr + 1 + get_column_offset(schema, col_index), sizeof(int32_t));

                    char* endptr;
                    long target = strtol(delete_statement->conditions[condition_index ].value, &endptr, 10);
                    if (endptr == delete_statement->conditions[condition_index ].value || *endptr != '\0' || val != (int32_t)target) {
                        has_conditions = 0;
                        break;
                    }
                } else if (schema->columns[col_index].type == COLUMN_VARCHAR) {
                    char buf[257] = {0};
                    memcpy(buf, (char*)row_ptr + 1 + get_column_offset(schema, col_index), schema->columns[col_index].size);
                    if (strcmp(buf, delete_statement->conditions[condition_index ].value) != 0) {
                        has_conditions = 0;
                        break;
                    }
                }
            }

            if (has_conditions) {
                *(uint8_t*)row_ptr = 1;
            }

        } else {
            *(uint8_t*)row_ptr = 1; //deletes all of the rows if there is no condition
        }
    }

    return EXECUTE_SUCCESS;
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
