/**
 * The MIT License (MIT)
 * Copyright (c) 2015 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "NTPClient.h"

NTPClient::NTPClient(UDP& udp) {
  this->_udp            = &udp;
}

NTPClient::NTPClient(UDP& udp, long timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName) {
  this->_udp            = &udp;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, long timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, long timeOffset, unsigned long updateInterval) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
  this->_updateInterval = updateInterval;
}

void NTPClient::begin() {
  this->begin(NTP_DEFAULT_LOCAL_PORT);
}

void NTPClient::begin(int port) {
  this->_port = port;

  this->_udp->begin(this->_port);

  this->_udpSetup = true;
}

bool NTPClient::forceUpdate() {
  #ifdef DEBUG_NTPClient
    Serial.println("Update from NTP Server");
  #endif

  // flush any existing packets
  while(this->_udp->parsePacket() != 0)
    this->_udp->flush();

  this->sendNTPPacket();

  // Wait till data is there or timeout...
  byte timeout = 0;
  int cb = 0;
  do {
    delay ( 10 );
    cb = this->_udp->parsePacket();
    if (timeout > 100) return false; // timeout after 1000 ms
    timeout++;
  } while (cb == 0);

  this->_lastUpdate = millis() - (10 * (timeout + 1)); // Account for delay in reading the time

  this->_udp->read(this->_packetBuffer, NTP_PACKET_SIZE);

  unsigned long highWord = word(this->_packetBuffer[40], this->_packetBuffer[41]);
  unsigned long lowWord = word(this->_packetBuffer[42], this->_packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;

  this->_currentEpoc = secsSince1900 - SEVENZYYEARS;

  return true;  // return true after successful update
}

bool NTPClient::isTimeSet() const {
  return (this->_lastUpdate != 0); // returns true if the time has been set, else false
}

bool NTPClient::update() {
  // return "TRUE" after a successful update. 
  // return "FALSE" for the following conditions:
  //  - Interval duration requirement not met
  //  - forceUpdate() times out
  if ((millis() - this->_lastUpdate >= this->_updateInterval) // Update after _updateInterval
    || this->_lastUpdate == 0) {                              // or Update if there was no update yet.
    if (!this->_udpSetup) this->begin();                      // setup the UDP client if needed
    return this->forceUpdate();
  }
  return false;   // return false if update does not occur
}

unsigned long NTPClient::getEpochTime() const {
  return this->_currentEpoc + // Epoc returned by the NTP server
         ((millis() - this->_lastUpdate) / 1000); // Time since last update
}

int NTPClient::getDay() const {
  unsigned long time = this->getEpochTime();
  time += this->_timeOffset; // User offset
  return (((time / 86400L) + 4 ) % 7); //0 is Sunday
}
int NTPClient::getHours() const {
  unsigned long time = this->getEpochTime();
  time += this->_timeOffset; // User offset
  return ((time  % 86400L) / 3600);
}
int NTPClient::getHours12() const {
  unsigned long time = getHours();
  if( time == 0 )
    return 12; // 12 midnight
  if( time  > 12)
    return time -= 12 ;
  return time;
}
int NTPClient::getPM() const {
  return (getHours() >= 12 );
}
int NTPClient::getMinutes() const {
  unsigned long time = this->getEpochTime();
  time += this->_timeOffset; // User offset
  return ((time % 3600) / 60);
}
int NTPClient::getSeconds() const {
  unsigned long time = this->getEpochTime();
  time += this->_timeOffset; // User offset
  return (time % 60);
}

String NTPClient::getFormattedTime(byte style) const {
  unsigned long rawTime = this->getEpochTime();
  rawTime += this->_timeOffset; // User offset
  unsigned long hours = (rawTime % 86400L) / 3600;
  if(style) {
    if( hours == 0 )
      hours = 12; // 12 midnight
    if( hours  > 12)
      hours -= 12 ;
  }
  String hoursStr = hours < 10 ? " " + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  String time = hoursStr + ":" + minuteStr + ":" + secondStr;
  if(style)
    time += (getPM() ? " PM" : " AM");
  return time;
}

// Based on https://github.com/PaulStoffregen/Time/blob/master/Time.cpp
// currently assumes UTC timezone, instead of using this->_timeOffset
int NTPClient::getYear() const {
  unsigned long rawTime = (this->getEpochTime() + this->_timeOffset) / 86400L;  // in days

  unsigned long days = 0, year = 1970;
  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;

  return year;
}

int NTPClient::getMonth() const {
  unsigned long rawTime = (this->getEpochTime() +this->_timeOffset) / 86400L;  // in days
  unsigned long days = 0, year = 1970;
  uint8_t month, monthLength;
  static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0

  days=0;
  for (month=0; month<12; month++) {
    if (month==1) { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }

  return month +1;  // Jan is month 1
}

int NTPClient::getDate() const {
  unsigned long rawTime = (this->getEpochTime() +this->_timeOffset) / 86400L;  // in days
  unsigned long days = 0, year = 1970;
  uint8_t month, monthLength;
  static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31};

  while((days += (LEAP_YEAR(year) ? 366 : 365)) <= rawTime)
    year++;
  rawTime -= days - (LEAP_YEAR(year) ? 366 : 365); // now it is days in this year, starting at 0
  days=0;
  for (month=0; month<12; month++) {
    if (month==1) { // february
      monthLength = LEAP_YEAR(year) ? 29 : 28;
    } else {
      monthLength = monthDays[month];
    }
    if (rawTime < monthLength) break;
    rawTime -= monthLength;
  }

  return rawTime +1;  // day of month start at 1
}

String NTPClient::getFormattedDate(byte style=0) const {
  // formatted like `2004-02-24` ISO 8601 for style = 0  (default)
  // formatted like `02-24-2004` US  for style = 1
	int day = this->getDate();
	int month = this->getMonth();
	int year = this->getYear();

	String dayStr = day < 10 ? "0" + String(day) : String(day);
	String monthStr = month < 10 ? "0" + String(month) : String(month);
	String yearStr = String(year);

  if (style)
    return monthStr + "-" + dayStr + "-" + yearStr;  // US
  else
    return yearStr + "-" + monthStr + "-" +dayStr;   // Europe
}

void NTPClient::end() {
  this->_udp->stop();

  this->_udpSetup = false;
}

void NTPClient::setTimeOffset(int timeOffset) {
  this->_timeOffset     = timeOffset;
}

void NTPClient::setUpdateInterval(unsigned long updateInterval) {
  this->_updateInterval = updateInterval;
}

void NTPClient::setPoolServerName(const char* poolServerName) {
    this->_poolServerName = poolServerName;
}

void NTPClient::sendNTPPacket() {
  // set all bytes in the buffer to 0
  memset(this->_packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  this->_packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  this->_packetBuffer[1] = 0;     // Stratum, or type of clock
  this->_packetBuffer[2] = 6;     // Polling Interval
  this->_packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  this->_packetBuffer[12]  = 49;
  this->_packetBuffer[13]  = 0x4E;
  this->_packetBuffer[14]  = 49;
  this->_packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  this->_udp->beginPacket(this->_poolServerName, 123); //NTP requests are to port 123
  this->_udp->write(this->_packetBuffer, NTP_PACKET_SIZE);
  this->_udp->endPacket();
}
