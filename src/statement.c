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
    const Token token = next_token(&lexer);

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


ExecuteResult execute_statement(const Statement* statement) {
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
            printf("CREATE DATABASE (to be completed)\n"); // TODO
            return EXECUTE_SUCCESS;
        case STATEMENT_SHOW_DATABASES:
            printf("SHOW DATABASES (to be completed)\n"); // TODO
            return EXECUTE_SUCCESS;
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_insert(const InsertStatement* insert_statement) {
    Table* table = find_table(&global_db, insert_statement->table_name);

    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_FAIL;
    }

    const Row* row_to_insert = &insert_statement->row;
    void* destination = row_slot(table, table->num_rows);

    serialize_row(&table->schema, row_to_insert, destination);
    table->num_rows += 1;

    // TODO as i have not implemented a primary key attribute i will for now use the row number as the key for bpt
    //bpt_insert(table->tree, (int)table->primary_key_index, destination);

    if (table->primary_key_index >= 0) {
        const uint32_t key = extract_primary_key(&table->schema, row_to_insert, table->primary_key_index);
        bpt_insert(table->tree, key, destination);
    }


    return EXECUTE_SUCCESS;
}

// TODO implement BPT search
ExecuteResult execute_select(const SelectStatement* select_statement) {
    Table* table = find_table(&global_db, select_statement->table_name);
    TableSchema* schema = &table->schema;
    Row row;
    row.data = malloc(compute_row_size(&table->schema));
    if (!row.data) return EXECUTE_FAIL;


    if (select_statement->selected_col_count == 0) {
        printf("COLUMNS:\n");
        printf("(");
        for (int column_index = 0; column_index < schema->num_columns; column_index++) {
            if (column_index > 0) printf(", ");
            printf("%s", schema->columns[column_index].name);
        }
        printf(")\n\n");
    } else {
        printf("COLUMNS:\n");
        printf("(");
        for (uint32_t i = 0; i < select_statement->selected_col_count; i++) {
            if (i > 0) printf(", ");
            const uint32_t index = select_statement->selected_col_indexes[i];
            printf("%s", schema->columns[index].name);
        }
        printf(")\n\n");
    }

    if (!select_statement->has_condition) {
        for (uint32_t row_index = 0; row_index < table->num_rows; row_index++) {
            void* row_ptr = row_slot(table, row_index);
            if (*(uint8_t*)row_ptr) continue;
            deserialize_row(&table->schema, row_slot(table, row_index), &row);
            print_row(&table->schema, &row, select_statement);
        }

        free(row.data);
        return EXECUTE_SUCCESS;
    }

    // TODO
    //  first check if the table has a primary key and the key is in the conditions
    //  if so first gather the rows with bpt_search and then implement for loop search
    //  to those rows that have been gathered by the bpt_search

    if (table->primary_key_index >= 0) {
        for (uint32_t condition_index = 0; condition_index < select_statement->condition_count; condition_index++) {
            if (select_statement->conditions[condition_index].column_index == table->primary_key_index) {
                TokenType type = select_statement->conditions[condition_index].type;
                if (type == TOKEN_EQUAL) {
                    int32_t val;
                    memcpy(&val, row.data + get_column_offset(schema, select_statement->conditions[condition_index].column_index), sizeof(int32_t));

                    char *endptr;
                    const char* value = select_statement->conditions[condition_index].value;

                    const long int target = strtol(value, &endptr, 10);

                    if (endptr == value || *endptr != '\0') continue;
                    void* row_ptr = bpt_search_equals(table->tree->root, target);
                    if (row_ptr == NULL) continue;
                    if (*(uint8_t*)row_ptr) continue;

                    deserialize_row(&table->schema, row_ptr, &row);
                    const int has_conditions = filter_rows(select_statement->conditions, select_statement->condition_count, table, row);
                    if (has_conditions) print_row(&table->schema, &row, select_statement);
                    return EXECUTE_SUCCESS;
                }
                if (type == TOKEN_GREATER || type == TOKEN_GREATER_EQUAL) {

                    int32_t val;
                    memcpy(&val, row.data + get_column_offset(schema, select_statement->conditions[condition_index].column_index), sizeof(int32_t));

                    char *endptr;
                    const char* value = select_statement->conditions[condition_index].value;

                    const long int target = strtol(value, &endptr, 10);
                    if (endptr == value || *endptr != '\0') continue;

                    const BPTreeNode* node = bpt_search_greater_equal(table->tree->root, target);
                    int index = -1;
                    for (int i = 0; i < node->num_keys; i++) {
                        if (node->keys[i] == target) {
                            index = i;
                        }
                    }
                    if (index == -1) return EXECUTE_FAIL;


                    // TODO take the insides of these if statements and put them in their own functions
                    //  so that these if block can be made into switch case
                    /*
                    if (type == TOKEN_GREATER_EQUAL) {
                        if (node->pointers[index - 1] == NULL || *(uint8_t*)node->pointers[index - 1]) continue;
                        deserialize_row(&table->schema, node->pointers[index - 1], &row);
                        const int has_conditions = filter_rows(select_statement->conditions, select_statement->condition_count, table, row);
                        if (has_conditions) print_row(&table->schema, &row, select_statement);
                    }
                    */


                    while (index != node->num_keys) {
                        if (node->pointers[index] == NULL || *(uint8_t*)node->pointers[index]) continue;
                        deserialize_row(&table->schema, node->pointers[index], &row);
                        const int has_conditions = filter_rows(select_statement->conditions, select_statement->condition_count, table, row);
                        if (has_conditions) print_row(&table->schema, &row, select_statement);
                        index++;
                    }
                    node = node->next;

                    while (node != NULL) {
                        for (int i = 0; i < node->num_keys; i++) {
                            if (node->pointers[i] == NULL || *(uint8_t*)node->pointers[i]) continue;
                            deserialize_row(&table->schema, node->pointers[i], &row);
                            const int has_conditions = filter_rows(select_statement->conditions, select_statement->condition_count, table, row);
                            if (has_conditions) print_row(&table->schema, &row, select_statement);
                        }
                    node = node->next;

                    }
                    return EXECUTE_SUCCESS;



                }
            }
        }
    }
    else {
        for (uint32_t row_index = 0; row_index < table->num_rows; row_index++) {
            void* row_ptr = row_slot(table, row_index);
            if (*(uint8_t*)row_ptr) continue;

            deserialize_row(&table->schema, row_slot(table, row_index), &row);
            const int has_conditions = filter_rows(select_statement->conditions, select_statement->condition_count, table, row);
            if (has_conditions) print_row(&table->schema, &row, select_statement);

        }
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
    if (table == NULL) {
        return EXECUTE_FAIL;
    }

    Row row;
    row.data = malloc(compute_row_size(&table->schema));
    if (!row.data) {
        return EXECUTE_FAIL;
    }


    for (uint32_t row_index = 0; row_index < table->num_rows; row_index++) {
        void* row_ptr = row_slot(table, row_index);

        //skips already deleted rows
        if (*(uint8_t*)row_ptr) continue;

        if (!delete_statement->has_condition) {
            *(uint8_t*)row_ptr = 1; //deletes all of the rows if there is no condition
            continue;
        }
        deserialize_row(&table->schema, row_slot(table, row_index), &row);
        const int has_conditions = filter_rows(delete_statement->conditions, delete_statement->condition_count, table, row);
        switch (has_conditions) {
            case 1:
                *(uint8_t*)row_ptr = 1;
                break;
            case -1:
                free(row.data);
                return EXECUTE_FAIL;
            default:
                break;
        }

    }

    free(row.data);
    return EXECUTE_SUCCESS;
}



const char* find_close_parenthesis(const char* open_parenthesis) {
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
            const uint32_t index = select_statement->selected_col_indexes[i];
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


void free_statement(const Statement* statement) {
    switch (statement->type) {
        case STATEMENT_SELECT:
            free_conditions(statement->select_stmt.condition_count, statement->select_stmt.conditions);
            break;
        case STATEMENT_DELETE:
            free_conditions(statement->delete_stmt.condition_count, statement->delete_stmt.conditions);
            break;
        default:;
    }
}

void free_conditions(const uint32_t condition_count, const Condition* conditions) {
    for (uint32_t condition_index = 0; condition_index < condition_count; condition_index++) {
        free(conditions[condition_index].value);
        free(conditions[condition_index].column_name);
    }
}


int filter_rows(const Condition* conditions, const uint32_t condition_count, Table* table, const Row row) {

    //deserialize_row(&table->schema, row_slot(table, row_index), &row);
    int has_conditions = 1;
    const TableSchema* schema = &table->schema;
    for (uint32_t condition_index = 0; condition_index < condition_count; condition_index++) {
        if (!has_conditions) continue;
        const uint32_t index = conditions[condition_index].column_index;
        if (schema->columns[index].type == COLUMN_INT) {
            int32_t val;
            memcpy(&val, row.data + get_column_offset(schema, index), sizeof(int32_t));

            char *endptr;
            const char* value = conditions[condition_index].value;

            const long int target = strtol(value, &endptr, 10);
            if (endptr == value || *endptr != '\0') {has_conditions = 0; continue;}

            switch (conditions[condition_index].type) {
                case TOKEN_EQUAL:
                    has_conditions = (target == val);
                    break;
                case TOKEN_GREATER_EQUAL:
                    has_conditions = (val >= target);
                    break;
                case TOKEN_GREATER:
                    has_conditions = (val > target);
                    break;
                case TOKEN_LESSER_EQUAL:
                    has_conditions = (val <= target);
                    break;
                case TOKEN_LESS:
                    has_conditions = (val < target);
                    break;
                case TOKEN_NOT_EQUAL:
                    has_conditions = (val != target);
                    break;
                default:
                    return -1;
            }

        }
        else if (schema->columns[index].type == COLUMN_VARCHAR) {
            char buf[257];
            memcpy(buf, row.data + get_column_offset(schema, index), 256);
            const int result = strcmp(buf, conditions[condition_index].value) != 0;

            switch (conditions[condition_index].type) {
                case TOKEN_EQUAL:
                    has_conditions = (result == 0);
                    break;
                case TOKEN_GREATER_EQUAL:
                    has_conditions = (result >= 0);
                    break;
                case TOKEN_GREATER:
                    has_conditions = (result > 0);
                    break;
                case TOKEN_LESSER_EQUAL:
                    has_conditions = (result <= 0);
                    break;
                case TOKEN_LESS:
                    has_conditions = (result < 0);
                    break;
                default:
                    return -1;
            }

        }
    }

    return has_conditions;
}