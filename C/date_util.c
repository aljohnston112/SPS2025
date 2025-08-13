#include "date_util.h"

#include "csv_util.h"

/**
 * Checks if the given year is a leap year.
 *
 * @param year The year to check.
 * @return true if given year is a leap year, else false.
 */
bool is_leap_year(const uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * Increments the given date.
 *
 * @param date The date to increment to the next date.
 */
void increment_date_by_one_day(Date* date) {
    static const uint8_t days_in_month[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    const uint16_t year = date->year;
    const uint8_t month = date->month;
    const uint8_t day = date->day;
    uint8_t days_in_current_month = days_in_month[month - 1];
    if (month == 2 && is_leap_year(year)) {
        days_in_current_month = 29;
    }

    if (day < days_in_current_month) {
        date->day++;
    } else {
        date->day = 1;
        if (month < 12) {
            date->month++;
        } else {
            date->month = 1;
            date->year++;
        }
    }
}

/**
 * Compares the first date with the second.
 *
 * @param date1 First date
 * @param date2 Second date
 * @return true if the first date is less than or equal to the last, else false.
 */
bool is_date_less_or_equal(
    const Date* date1,
    const Date* date2
) {
    const uint16_t year1 = date1->year;
    const uint16_t year2 = date2->year;
    if (year1 != year2) {
        return year1 < year2;
    }
    const uint16_t month1 = date1->month;
    const uint16_t month2 = date2->month;
    if (month1 != month2) {
        return month1 < month2;
    }
    return date1->day <= date2->day;
}

/**
 * Compares the first date with the second.
 *
 * @param date1 First date
 * @param date2 Second date
 * @return true if the first date is less than the last, else false.
 */
bool is_date_less(
    const Date* date1,
    const Date* date2
) {
    const uint16_t year1 = date1->year;
    const uint16_t year2 = date2->year;
    if (year1 != year2) {
        return year1 < year2;
    }
    const uint8_t month1 = date1->month;
    const uint8_t month2 = date2->month;
    if (month1 != month2) {
        return month1 < month2;
    }
    return date1->day < date2->day;
}

/**
 * Compares two dates for qsort.
 * @param a One date.
 * @param b The other date.
 * @return A negative number if a is less than b,
 *         a positive number if b is greater than a, else 0.
 */
int compare_by_date(const void* a, const void* b) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
    const StockDataRow* row_a = (*(StockDataTable**)a)->rows;
    const StockDataRow* row_b = (*(StockDataTable**)b)->rows;
#pragma GCC diagnostic pop

    const uint16_t a_year = row_a->date.year;
    const uint16_t b_year = row_b->date.year;
    if (a_year != b_year) {
        return a_year - b_year;
    }

    const uint8_t a_month = row_a->date.month;
    const uint8_t b_month = row_b->date.month;
    if (a_month != b_month) {
        return a_month - b_month;
    }

    return row_a->date.day - row_b->date.day;
}
