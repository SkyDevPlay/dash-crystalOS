#ifndef RTC_H
#define RTC_H

#include "../types.h"

struct SystemDate {
    u16 year;
    u8  month;
    u8  day;
    u8  hour;
    u8  minute;
    u8  second;
};

void rtc_init(void);
void get_system_date(struct SystemDate *out);
int set_system_date(u16 year, u8 month, u8 day);
void set_system_time(u8 hour, u8 minute, u8 second);

#endif
