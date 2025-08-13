#ifndef SPS2025_RANKS_UTIL_H
#define SPS2025_RANKS_UTIL_H

#include <stdint.h>
#include <sys/types.h>

typedef struct {
    u_int16_t year;
    u_int8_t month;
    u_int8_t day;
} Date;

bool is_leap_year(uint16_t year);

void increment_date_by_one_day(Date* date);

bool is_date_less_or_equal(const Date* date1, const Date* date2);

bool is_date_less(const Date* date1,const Date* date2);

int compare_by_date(const void* a, const void* b);

#endif //SPS2025_RANKS_UTIL_H