#include "sys/rtc.h"
#include "sys/ports.h"

/*
 * CMOS RTC registers are accessed through I/O ports 0x70 (index) and 0x71 (data).
 * The RTC returns values in BCD by default.
 */

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define RTC_SECONDS  0x00
#define RTC_MINUTES  0x02
#define RTC_HOURS    0x04
#define RTC_DAY      0x07
#define RTC_MONTH    0x08
#define RTC_YEAR     0x09
#define RTC_STATUS_B 0x0B

static struct SystemDate sys_date;

static u8 cmos_read(u8 reg) {
    outb(CMOS_ADDR, reg);
    return inb(CMOS_DATA);
}

static u8 bcd_to_bin(u8 bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

void rtc_init(void) {
    u8 status_b = cmos_read(RTC_STATUS_B);
    int is_bcd = !(status_b & 0x04);

    u8 sec   = cmos_read(RTC_SECONDS);
    u8 min   = cmos_read(RTC_MINUTES);
    u8 hour  = cmos_read(RTC_HOURS);
    u8 day   = cmos_read(RTC_DAY);
    u8 month = cmos_read(RTC_MONTH);
    u8 year  = cmos_read(RTC_YEAR);

    if (is_bcd) {
        sec   = bcd_to_bin(sec);
        min   = bcd_to_bin(min);
        hour  = bcd_to_bin(hour);
        day   = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year  = bcd_to_bin(year);
    }

    sys_date.year   = 2000 + year;
    sys_date.month  = month;
    sys_date.day    = day;
    sys_date.hour   = hour;
    sys_date.minute = min;
    sys_date.second = sec;
}

void get_system_date(struct SystemDate *out) {
    out->year   = sys_date.year;
    out->month  = sys_date.month;
    out->day    = sys_date.day;
    out->hour   = sys_date.hour;
    out->minute = sys_date.minute;
    out->second = sys_date.second;
}

void set_system_date(u16 year, u8 month, u8 day) {
    sys_date.year  = year;
    sys_date.month = month;
    sys_date.day   = day;
}

void set_system_time(u8 hour, u8 minute, u8 second) {
    sys_date.hour   = hour;
    sys_date.minute = minute;
    sys_date.second = second;
}
