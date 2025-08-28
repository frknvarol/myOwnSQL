#include "../include/input_buffer.h"
#include "../include/meta_command.h"
#include "../include/statement.h"
#include <assert.h>
#include <_string.h>
#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>


void setUp(void) {
    // runs before each test
}

void tearDown(void) {
    // runs after each test
}

int test_insert(void) {
    Table* table = new_table();

    // Fake an input like "insert 1 user1 user1@example.com"
    InputBuffer* input_buffer = new_input_buffer();
    input_buffer->buffer = strdup("insert 1 user1 user1@example.com");
    input_buffer->buffer_length = strlen(input_buffer->buffer);
    input_buffer->input_length = strlen(input_buffer->buffer);

    Statement statement;
    PrepareResult prepare = prepare_statement(input_buffer, &statement);
    assert(prepare == PREPARE_SUCCESS);

    ExecuteResult exec_result = execute_statement(&statement, table);
    assert(exec_result == EXECUTE_SUCCESS);

    // Verify the row was inserted correctly
    Row row;
    deserialize_row(row_slot(table, 0), &row);

    assert(row.id == 1);
    assert(strcmp(row.username, "user1") == 0);
    assert(strcmp(row.email, "user1@example.com") == 0);

    printf("✅ Insert test passed: (%d, %s, %s)\n", row.id, row.username, row.email);

    free(input_buffer->buffer);
    free(input_buffer);
    return 0;


}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_insert);
    return UNITY_END();
}
