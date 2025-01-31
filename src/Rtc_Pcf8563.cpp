/****c*
 *  NAME
 *    Pcf8563 Real Time Clock support routines
 *  AUTHOR
 *    Joe Robertson, jmr
 *    orbitalair@gmail.com
 *
 *  CREATION DATE
 *    9/24/06,  init - built off of usart demo.  using mikroC
 *  NOTES
 *  HISTORY
 *    10/14/06 ported to CCS compiler, jmr
 *    2/21/09  changed all return values to hex val and not bcd, jmr
 *    1/10/10  ported to arduino, jmr
 *    2/14/10  added 3 world date formats, jmr
 *    1/22/10, jmr,  ported to arduino 1.0.  changed header include, wire.send -> wire.write, wire.receive -> wire.read
 *    28/02/2012 A. Pasotti
 *             fixed a bug in RTCC_ALARM_AF,
 *             added a few (not really useful) methods
 *    12/26/12, jmr, merged in fix for arduino 1.0.1 Wire.
 *                   added version method to print out code version.
 *    2/6/15  made gettime, getdate more convenient
 *                   rechecked and added to examples.  ino's are most up to date.
 *  TODO
 *    done- Add Euro date format
 *    done- Add short time (hh:mm) format
 *    Add 24h/12h format
 *    Add timer support
 ******
 *  Robodoc embedded documentation.
 *  http://www.xs4all.nl/~rfsber/Robo/robodoc.html
 */
/*  Copyright (c) 2006-2020 Joe Robertson. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 * or https://www.gnu.org/licenses/lgpl-3.0.html
 */

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#define byte uint8_t
#define send(X) write(static_cast<uint8_t>(X))
#define receive(X) read(X)
#else
#include "WProgram.h"
#endif

#include "Rtc_Pcf8563.h"
#include <Wire.h>

Rtc_Pcf8563::Rtc_Pcf8563(void)
{
    Wire.begin();
    Rtcc_Addr = RTCC_R >> 1;
}

/**
 * @brief zero out all values, disable all alarms
 *
 */
void Rtc_Pcf8563::initClock()
{
    Wire.beginTransmission(Rtcc_Addr); // Issue I2C start signal
    Wire.send(0x0); // start address

    Wire.send(0x0); // control/status1
    Wire.send(0x0); // control/status2
    Wire.send(0x01); // set seconds
    Wire.send(0x01); // set minutes
    Wire.send(0x01); // set hour
    Wire.send(0x01); // set day
    Wire.send(0x01); // set weekday
    Wire.send(0x01); // set month, century to 1
    Wire.send(0x01); // set year to 99
    Wire.send(0x80); // minute alarm value reset to 00
    Wire.send(0x80); // hour alarm value reset to 00
    Wire.send(0x80); // day alarm value reset to 00
    Wire.send(0x80); // weekday alarm value reset to 00
    Wire.send(0x0); // set SQW, see: setSquareWave
    Wire.send(0x0); // timer off
    Wire.endTransmission();
}

/* Private internal functions, but useful to look at if you need a similar func. */
byte Rtc_Pcf8563::decToBcd(byte val)
{
    return ((val / 10 * 16) + (val % 10));
}

byte Rtc_Pcf8563::bcdToDec(byte val)
{
    return ((val / 16 * 10) + (val % 16));
}

/**
 * @brief Reset statis register
 *
 */
void Rtc_Pcf8563::clearStatus()
{
    Wire.beginTransmission(Rtcc_Addr); // Issue I2C start signal
    Wire.send(0x0);
    Wire.send(0x0); // control/status1
    Wire.send(0x0); // control/status2
    Wire.endTransmission();
}

/**
 * @brief Set time
 *
 * @param hour -> 0-23
 * @param minute -> 0-59
 * @param sec -> 0-59
 */
void Rtc_Pcf8563::setTime(byte hour, byte minute, byte sec)
{
    Wire.beginTransmission(Rtcc_Addr); // Issue I2C start signal
    Wire.send(RTCC_SEC_ADDR); // send addr low byte, req'd

    Wire.send(decToBcd(sec)); // set seconds
    Wire.send(decToBcd(minute)); // set minutes
    Wire.send(decToBcd(hour)); // set hour

    Wire.endTransmission();
}
/**
 * @brief Set date
 *
 * @param day -> 0-31
 * @param weekday -> 0-6
 * @param mon -> 1-12
 * @param century -> 0=2000, 1=1900
 * @param year -> 0-99
 */
void Rtc_Pcf8563::setDate(byte day, byte weekday, byte mon, byte century, byte year)
{
    /* year val is 00 to 99, xx
        with the highest bit of month = century
        0=20xx
        1=19xx
        */
    month = decToBcd(mon);
    if (century == 1) {
        month |= RTCC_CENTURY_MASK;
    } else {
        month &= ~RTCC_CENTURY_MASK;
    }
    Wire.beginTransmission(Rtcc_Addr); // Issue I2C start signal
    Wire.send(RTCC_DAY_ADDR);
    Wire.send(decToBcd(day)); // set day
    Wire.send(decToBcd(weekday)); // set weekday
    Wire.send(month); // set month, century to 1
    Wire.send(decToBcd(year)); // set year to 99
    Wire.endTransmission();
}

/**
 * @brief enable alarm interrupt whenever the clock matches these values an int will be sent out pin 3 of the Pcf8563 chip
 *
 */
void Rtc_Pcf8563::enableAlarm()
{

    // set status2 AF val to zero
    status2 &= ~RTCC_ALARM_AF;
    // enable the interrupt
    status2 |= RTCC_ALARM_AIE;

    // enable the interrupt
    Wire.beginTransmission(Rtcc_Addr); // Issue I2C start signal
    Wire.send(RTCC_STAT2_ADDR);
    Wire.send(status2);
    Wire.endTransmission();
}

/**
 * @brief Read status byte, internal usage only here. see alarmEnabled, alarmActive
 *
 */
byte Rtc_Pcf8563::readStatus2()
{
    /* set the start byte of the status 2 data */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.send(RTCC_STAT2_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 1); // request 1 bytes
    return Wire.read();
}

/**
 * @brief Returns true if AIE is on
 * @return Returns true if AIE is on
 */
boolean Rtc_Pcf8563::alarmEnabled()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_AIE;
}

/**
 * @brief Returns true if AF is on
 * @return Returns true if AF is on
 */
boolean Rtc_Pcf8563::alarmActive()
{
    return Rtc_Pcf8563::readStatus2() & RTCC_ALARM_AF;
}
/**
 * @brief set the alarm values whenever the clock matches these values an int will be sent out pin 3 of the Pcf8563 chip
 *
 */
void Rtc_Pcf8563::setAlarm(byte min, byte hour, byte day, byte weekday)
{
    if (min < 99) {
        min = constrain(min, 0, 59);
        min = decToBcd(min);
        min &= ~RTCC_ALARM;
    } else {
        min = 0x0;
        min |= RTCC_ALARM;
    }

    if (hour < 99) {
        hour = constrain(hour, 0, 23);
        hour = decToBcd(hour);
        hour &= ~RTCC_ALARM;
    } else {
        hour = 0x0;
        hour |= RTCC_ALARM;
    }

    if (day < 99) {
        day = constrain(day, 1, 31);
        day = decToBcd(day);
        day &= ~RTCC_ALARM;
    } else {
        day = 0x0;
        day |= RTCC_ALARM;
    }

    if (weekday < 99) {
        weekday = constrain(weekday, 0, 6);
        weekday = decToBcd(weekday);
        weekday &= ~RTCC_ALARM;
    } else {
        weekday = 0x0;
        weekday |= RTCC_ALARM;
    }

    Rtc_Pcf8563::enableAlarm();

    // TODO bounds check  the inputs first
    Wire.beginTransmission(Rtcc_Addr); // Issue I2C start signal
    Wire.send(RTCC_ALRM_MIN_ADDR);
    Wire.send(min); // minute alarm value reset to 00
    Wire.send(hour); // hour alarm value reset to 00
    Wire.send(day); // day alarm value reset to 00
    Wire.send(weekday); // weekday alarm value reset to 00
    Wire.endTransmission();
}

/**
 *  @brief Update alarm values, set values to RTCC_NO_ALARM (99) if alarm flag is not set
 */
void Rtc_Pcf8563::getAlarm()
{
    /* set the start byte of the alarm data */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.send(RTCC_ALRM_MIN_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 4); // request 4 bytes
    alarm_minute = Wire.read();
    if (B10000000 & alarm_minute) {
        alarm_minute = RTCC_NO_ALARM;
    } else {
        alarm_minute = bcdToDec(alarm_minute & B01111111);
    }
    alarm_hour = Wire.read();
    if (B10000000 & alarm_hour) {
        alarm_hour = RTCC_NO_ALARM;
    } else {
        alarm_hour = bcdToDec(alarm_hour & B00111111);
    }
    alarm_day = Wire.read();
    if (B10000000 & alarm_day) {
        alarm_day = RTCC_NO_ALARM;
    } else {
        alarm_day = bcdToDec(alarm_day & B00111111);
    }
    alarm_weekday = Wire.read();
    if (B10000000 & alarm_weekday) {
        alarm_weekday = RTCC_NO_ALARM;
    } else {
        alarm_weekday = bcdToDec(alarm_weekday & B00000111);
    }
}

/**
 * @brief Set the square wave pin output
 */
void Rtc_Pcf8563::setSquareWave(byte frequency)
{
    Wire.beginTransmission(Rtcc_Addr); // Issue I2C start signal
    Wire.send(RTCC_SQW_ADDR);
    Wire.send(frequency);
    Wire.endTransmission();
}

void Rtc_Pcf8563::clearSquareWave()
{
    Rtc_Pcf8563::setSquareWave(SQW_DISABLE);
}

/**
 * @brief Reset the alarm leaving interrupt unchanged
 */
void Rtc_Pcf8563::resetAlarm()
{
    // set status2 AF val to zero to reset alarm
    status2 &= ~RTCC_ALARM_AF;
    Wire.beginTransmission(Rtcc_Addr);
    Wire.send(RTCC_STAT2_ADDR);
    Wire.send(status2);
    Wire.endTransmission();
}

/**
 * @brief Clear alarm flag
 *
 */
void Rtc_Pcf8563::clearAlarm()
{
    // set status2 AF val to zero to reset alarm
    status2 &= ~RTCC_ALARM_AF;
    // turn off the interrupt
    status2 &= ~RTCC_ALARM_AIE;

    Wire.beginTransmission(Rtcc_Addr);
    Wire.send(RTCC_STAT2_ADDR);
    Wire.send(status2);
    Wire.endTransmission();
}
/**
 * @brief call this first to load current date values to variables
 *
 */
void Rtc_Pcf8563::getDate()
{
    /* set the start byte of the date data */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.send(RTCC_DAY_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 4); // request 4 bytes
    // 0x3f = 0b00111111
    day = bcdToDec(Wire.receive() & 0x3f);
    // 0x07 = 0b00000111
    weekday = bcdToDec(Wire.receive() & 0x07);
    // get raw month data byte and set month and century with it.
    month = Wire.receive();
    if (month & RTCC_CENTURY_MASK) {
        century = 1;
    } else {
        century = 0;
    }
    // 0x1f = 0b00011111
    month = month & 0x1f;
    month = bcdToDec(month);
    year = bcdToDec(Wire.receive());
}
/**
 * @brief call this first to load current time values to variables
 *
 */
void Rtc_Pcf8563::getTime()
{
    /* set the start byte , get the 2 status bytes */
    Wire.beginTransmission(Rtcc_Addr);
    Wire.send(RTCC_STAT1_ADDR);
    Wire.endTransmission();

    Wire.requestFrom(Rtcc_Addr, 5); // request 5 bytes
    status1 = Wire.receive();
    status2 = Wire.receive();
    // 0x7f = 0b01111111
    sec = bcdToDec(Wire.receive() & 0x7f);
    minute = bcdToDec(Wire.receive() & 0x7f);
    // 0x3f = 0b00111111
    hour = bcdToDec(Wire.receive() & 0x3f);
}

/**
 * @brief Get library Version
 *
 * @return const char* -> Library version eg. "Pcf8563 v1.x.x"
 */
const char* Rtc_Pcf8563::version()
{
    return RTCC_VERSION;
}

/**
 * @brief Read time and return fomatted string
 *
 * @param style RTCC_TIME_HM -> HH:MM ; RTCC_TIME_HMS -> HH:MM:SS
 * @return char* -> Time formated as HH:MM or HH:MM:SS
 */
char* Rtc_Pcf8563::formatTime(byte style)
{
    getTime();
    switch (style) {
    case RTCC_TIME_HM:
        strOut[0] = '0' + (hour / 10);
        strOut[1] = '0' + (hour % 10);
        strOut[2] = ':';
        strOut[3] = '0' + (minute / 10);
        strOut[4] = '0' + (minute % 10);
        strOut[5] = '\0';
        break;
    case RTCC_TIME_HMS:
    default:
        strOut[0] = '0' + (hour / 10);
        strOut[1] = '0' + (hour % 10);
        strOut[2] = ':';
        strOut[3] = '0' + (minute / 10);
        strOut[4] = '0' + (minute % 10);
        strOut[5] = ':';
        strOut[6] = '0' + (sec / 10);
        strOut[7] = '0' + (sec % 10);
        strOut[8] = '\0';
        break;
    }
    return strOut;
}

/**
 * @brief Read date and return fomatted string
 *
 * @param style RTCC_DATE_ASIA -> yyyy-mm-dd ; RTCC_DATE_US -> mm/dd/yyyy ; RTCC_DATE_WORLD -> dd-mm-yyyy
 * @return char* -> Date formated as yyyy-mm-dd or mm/dd/yyyy or dd-mm-yyyy
 */
char* Rtc_Pcf8563::formatDate(byte style)
{
    getDate();

    switch (style) {

    case RTCC_DATE_ASIA:
        // do the asian style, yyyy-mm-dd
        if (century == 1) {
            strDate[0] = '1';
            strDate[1] = '9';
        } else {
            strDate[0] = '2';
            strDate[1] = '0';
        }
        strDate[2] = '0' + (year / 10);
        strDate[3] = '0' + (year % 10);
        strDate[4] = '-';
        strDate[5] = '0' + (month / 10);
        strDate[6] = '0' + (month % 10);
        strDate[7] = '-';
        strDate[8] = '0' + (day / 10);
        strDate[9] = '0' + (day % 10);
        strDate[10] = '\0';
        break;
    case RTCC_DATE_US:
        // the pitiful US style, mm/dd/yyyy
        strDate[0] = '0' + (month / 10);
        strDate[1] = '0' + (month % 10);
        strDate[2] = '/';
        strDate[3] = '0' + (day / 10);
        strDate[4] = '0' + (day % 10);
        strDate[5] = '/';
        if (century == 1) {
            strDate[6] = '1';
            strDate[7] = '9';
        } else {
            strDate[6] = '2';
            strDate[7] = '0';
        }
        strDate[8] = '0' + (year / 10);
        strDate[9] = '0' + (year % 10);
        strDate[10] = '\0';
        break;
    case RTCC_DATE_WORLD:
    default:
        // do the world style, dd-mm-yyyy
        strDate[0] = '0' + (day / 10);
        strDate[1] = '0' + (day % 10);
        strDate[2] = '-';
        strDate[3] = '0' + (month / 10);
        strDate[4] = '0' + (month % 10);
        strDate[5] = '-';

        if (century == 1) {
            strDate[6] = '1';
            strDate[7] = '9';
        } else {
            strDate[6] = '2';
            strDate[7] = '0';
        }
        strDate[8] = '0' + (year / 10);
        strDate[9] = '0' + (year % 10);
        strDate[10] = '\0';
        break;
    }
    return strDate;
}

/**
 * @brief get seconds, needs getTime() to be called before
 *
 * @return byte -> seconds from 0 to 59
 */
byte Rtc_Pcf8563::getSecond()
{
    return sec;
}

/**
 * @brief get minutes, needs getTime() to be called before
 *
 * @return byte -> minutes from 0 to 59
 */
byte Rtc_Pcf8563::getMinute()
{
    return minute;
}

/**
 * @brief get hour, needs getTime() to be called before
 *
 * @return byte -> hours from 0 to 23
 */
byte Rtc_Pcf8563::getHour()
{
    getTime();
    return hour;
}

/**
 * @brief get alarm minute, needs getAlarm() to be called before
 *
 * @return byte -> minute from 0 to 59
 */
byte Rtc_Pcf8563::getAlarmMinute()
{
    return alarm_minute;
}

/**
 * @brief get alarm hour, needs getAlarm() to be called before
 *
 * @return byte -> hour from 0 to 23
 */
byte Rtc_Pcf8563::getAlarmHour()
{
    return alarm_hour;
}

/**
 * @brief get alarm day, needs getAlarm() to be called before
 *
 * @return byte -> day from 1 to 31
 */
byte Rtc_Pcf8563::getAlarmDay()
{
    return alarm_day;
}

/**
 * @brief get alarm weekday, needs getAlarm() to be called before
 *
 * @return byte -> weekday from 0 to 6
 */
byte Rtc_Pcf8563::getAlarmWeekday()
{
    return alarm_weekday;
}

/**
 * @brief get day, needs getDate() to be called before
 *
 * @return byte -> day from 1 to 31
 */
byte Rtc_Pcf8563::getDay()
{
    return day;
}

/**
 * @brief get month, needs getDate() to be called before
 *
 * @return byte -> month from 1 to 12
 */
byte Rtc_Pcf8563::getMonth()
{
    return month;
}

/**
 * @brief get year, needs getDate() to be called before.
 *
 * @return byte -> day from 0 to 99
 */
byte Rtc_Pcf8563::getYear()
{
    return year;
}

/**
 * @brief get weekday, needs getDate() to be called before
 *
 * @return byte -> day from 0 to 6
 */
byte Rtc_Pcf8563::getWeekday()
{
    return weekday;
}

/**
 * @brief Staus register1, set during getTime() operation
 *
 * @return byte -> Staus register1
 */
byte Rtc_Pcf8563::getStatus1()
{
    return status1;
}

/**
 * @brief Staus register2, set durung getTime(), enableAlarm(), resetAlarm() and clearAlarm() operation
 *
 * @return byte -> Staus register2
 */
byte Rtc_Pcf8563::getStatus2()
{
    return status2;
}
