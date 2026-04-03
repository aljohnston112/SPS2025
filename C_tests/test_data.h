#ifndef TEST_DATA_H
#define TEST_DATA_H

#include "../C/csv_util.h"

#define ROWS_IN_TEST_FILE 6

typedef struct {
    Date date;
    double open, high, low, close, volume;
} ExpectedRow;

static const char* const FAKE_DATA_FILE_NAME = "./test_data/fake_data.txt";
static const char* const FAKE_DATA_FILE_NAME2 = "./test_data/fake_data2.txt";

static const ExpectedRow expected_rows_for_fake_data[ROWS_IN_TEST_FILE] = {
    {
        {1999, 11, 18},
        1, 2, 3, 4, 5
    },
    {
        {1999, 11, 19},
        6, 7, 8, 9, 10
    },
    {
        {2000, 11, 18},
        11, 12, 13, 14, 15
    },
    {
        {2000, 11, 19},
        16, 17, 18, 19, 20
    },
    {
        {2001, 11, 18},
        21, 22, 23, 24, 25
    },
    {
        {2001, 11, 19},
        26, 27, 28, 29, 30
    },
};

static const ExpectedRow expected_rows_for_fake_data2[ROWS_IN_TEST_FILE] = {
    {
        {2001, 11, 18},
        31, 32, 33, 34, 35
    },
    {
        {2001, 11, 19},
        36, 37, 38, 39, 40
    },
    {
        {2002, 11, 18},
        41, 42, 43, 44, 45
    },
    {
        {2002, 11, 19},
        46, 47, 48, 49, 50
    },
    {
        {2003, 11, 18},
        51, 52, 53, 54, 55
    },
    {
        {2003, 11, 19},
        56, 57, 58, 59, 60
    }
};

static inline void check_table(
    const StockDataTable* table,
    const ExpectedRow* expected,
    const size_t count
) {
    assert(table->row_count == count);
    for (size_t i = 0; i < count; i++) {
        const StockDataRow* row = &table->rows[i];
        const ExpectedRow expected_row = expected[i];
        assert(
            memcmp(
                &row->date, &expected_row.date,
                sizeof(Date)
            ) == 0);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        assert(row->open == expected_row.open);
        assert(row->high == expected_row.high);
        assert(row->low == expected_row.low);
        assert(row->close == expected_row.close);
        assert(row->volume == expected_row.volume);
#pragma GCC diagnostic pop
    }
}

#endif //TEST_DATA_H
