/*
 * CMOS Real-time Clock
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (3)
 */

/*
 * STUDENT NUMBER: s1532620  
 */

#define CURRENT_YEAR        2020  

#include <infos/drivers/timer/rtc.h>
#include <infos/util/lock.h>
#include <arch/x86/pio.h>
#include <infos/kernel/log.h>

using namespace infos::arch::x86;
using namespace infos::kernel;
using namespace infos::drivers;
using namespace infos::drivers::timer;
using namespace infos::util;

class CMOSRTC : public RTC {
public:
	static const DeviceClass CMOSRTCDeviceClass;

	const DeviceClass& device_class() const override
	{
		return CMOSRTCDeviceClass;
	}

	/**
	 * Interrogates the RTC to read the current date & time.
	 * @param tp Populates the tp structure with the current data & time, as
	 * given by the CMOS RTC device.
	 */
	void read_timepoint(RTCTimePoint& tp) override
	{   
        

      	unsigned char last_second;
      	unsigned char last_minute;
      	unsigned char last_hour;
      	unsigned char last_day;
      	unsigned char last_month;
      	unsigned char last_year;
      	unsigned char registerB;


		while(update_in_progress()); //hangs on this line until an update is no longer in progress

        //read all the values from the cmos clock
        unsigned char second = read_RTC_value(0x00);
        unsigned char minute = read_RTC_value(0x02);
        unsigned char hour = read_RTC_value(0x04);
        unsigned char day_of_month = read_RTC_value(0x07);
        unsigned char month = read_RTC_value(0x08);
        unsigned char year = read_RTC_value(0x09);

        ///read the values until they dont change anymore to ensure we are getting the correct one
        do {
            last_second = second;
            last_minute = minute;
            last_hour = hour;
            last_day = day_of_month;
            last_month = month;
            last_year = year;
    
 
            while (update_in_progress());           
            second = read_RTC_value(0x00);
            minute = read_RTC_value(0x02);
            hour = read_RTC_value(0x04);
            day_of_month = read_RTC_value(0x07);
            month = read_RTC_value(0x08);
            year = read_RTC_value(0x09);
            
        } while( (last_second != second) || (last_minute != minute) || (last_hour != hour) ||
                (last_day != day_of_month) || (last_month != month) || (last_year != year) );

        registerB = read_RTC_value(0x0B);

        // convert from BCD to binary if necessary
        if (!(registerB & 0x04)){ //bit 3 is set if the values are in bcd

            second = (second & 0x0F) + ((second / 16) * 10);
            minute = (minute & 0x0F) + ((minute / 16) * 10);
            hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
            day_of_month = (day_of_month & 0x0F) + ((day_of_month / 16) * 10);
            month = (month & 0x0F) + ((month / 16) * 10);
            year = (year & 0x0F) + ((year / 16) * 10);
        }

        // if the clock is not in 24 hour mode and the pm bit is set then convert
        if (!(registerB & 0x02) && (hour & 0x80)) {
            hour = ((hour & 0x7F) + 12) % 24;
        }

        //if clock is not in 24h mode and the clock is in am and its 12am 
        if ( (!(registerB & 0x02)) && (!(hour & 0x80)) && (hour == 0x0C) ){
            //then convert 12am to 0000
            hour = 0x00;
        }

        
      
        tp.seconds =(unsigned short) second;
        tp.minutes = (unsigned short) minute;
        tp.hours = (unsigned short) hour;
        tp.day_of_month = (unsigned short) day_of_month;
        tp.month = (unsigned short) month;
        tp.year = (unsigned short) year;
	}   


	//you need to write the offset to port 0x70 then read the thing you want from port 0x71
	char read_RTC_value(int offset){
        UniqueIRQLock l;
		__outb(0x70, offset);
		return __inb(0x71);
	}

	//read from status register A to see if an update is in progress
    //returns 1 if update is in progress, 0 otherwise
	int update_in_progress(){
		__outb(0x70, 0x0A);
		return (__inb(0x71) & 0x80); //0x80 is 10000000 in binary and we need to check the 7th bit in 0x0A
	}


};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
