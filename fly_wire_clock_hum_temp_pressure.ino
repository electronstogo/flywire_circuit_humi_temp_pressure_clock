# 2019 - electronstogo

// Maybe you have to install some of the used libraries here.
// Because they are not native int the Arduino environment.  
#include <LiquidCrystal.h>
#include <avr/sleep.h>
#include "bme280.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>


// Initialize a 16x2 display handling object.
LiquidCrystal lcd(12, 11, 6, 5, 4, 3);

// Initialize a WS2812 LED handling object.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, 9, NEO_GRB + NEO_KHZ800);



#define RTC_I2C_ADDRESS   0x68




// calculation helping function.
// get dec format number, from bcd
byte bcd_to_dec(byte bcd_value)
{
    return (bcd_value & 0x0F) + (bcd_value >> 4) * 10;
}




// init stuff microcontroller stuff.
void init_mcu()
{
    // enable external interrupt rising edge at INT0 pin
    EICRA = 0x03;
    EIMSK = 0x01;
    EIFR  = 0x01;	
}




// external interrupt at INT0 pin, triggered by RTC 1Hz signal.
ISR (INT0_vect)
{
    // just wake up controller, for a display refresh.
}




// Receives time from RTC, and updates it on the 16x2 lcd display. 
void refresh_time_on_display()
{
    // Start reading the RTC registers from address 0x0.
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(0x0);
    Wire.endTransmission();


    // Get 3 bytes (seconds register, minutes register, hours register) 
    Wire.requestFrom(RTC_I2C_ADDRESS, 3);
    byte seconds_value = bcd_to_dec(Wire.read() & 0x7F);
    byte minutes_value = bcd_to_dec(Wire.read() & 0x7F);
    byte hours_value = bcd_to_dec(Wire.read() & 0x7F);



    // Transform a 2 digit string for display.
    char seconds_string[10];
    sprintf(seconds_string, "%i%i", seconds_value / 10, seconds_value % 10);

    // Transform a 2 digit string for display.
    char minutes_string[10];
    sprintf(minutes_string, "%i%i", minutes_value / 10, minutes_value % 10);

    // Transform a 2 digit string for display.
    char hours_string[10];
    sprintf(hours_string, "%i%i", hours_value / 10, hours_value % 10);


    lcd.print(hours_string);
    lcd.print(":");
    lcd.print(minutes_string);
    lcd.print(":");
    lcd.print(seconds_string);
}



// Sets the time in the RTC registers, with the given arguments.
// This function should only be used, when its neccessary to set the correct time.
void set_rtc_time(int hours, int minutes, int seconds)
{
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    Wire.write(0x0);
    Wire.write(((seconds / 10) << 4) + (seconds % 10));
    Wire.write(((minutes / 10) << 4) + (minutes % 10));

    // set hours, and activate 24 hr mode.
    byte hour_byte = ((hours / 10) << 4) + (hours % 10);
    Wire.write(hour_byte & 0b10111111);
    Wire.write(hour_byte);
    Wire.endTransmission();
}




void refresh_sensor_data_on_display(BME280Sensor bme280_sensor, int32_t* humidity_buffer)
{
    // update measurements on the display.
    int32_t temperature, humidity;
    uint32_t pressure;

    bme280_sensor.do_humidity_temperature_pressure_measurement(&temperature, &pressure, &humidity);
    *humidity_buffer = humidity / 1000;

    lcd.setCursor(0, 0);

    // temperature
    lcd.print((float)temperature / 100.0, 1);
    lcd.print((char)223);
    lcd.print("C ");
    lcd.print((char)124);
    lcd.print(" ");

    // pressure
    lcd.print(pressure);
    lcd.print("mbar");

    // humidity
    lcd.setCursor(0, 1);
    lcd.print((float)(humidity) / 1000.0, 1);
    lcd.print((char)37);
    lcd.print(" ");
    lcd.print((char)124);
    lcd.print(" ");
}



void setup()
{
    // disable interrupts
    cli();

    // init microcontroller stuff
    init_mcu();

    // init arduino i2c framework class.
    Wire.begin();


    // init arduino LCD display framework
    lcd.begin(16, 2);


    // init signal WS2812 LED.
    pixels.begin();
    // Set the start color for the LED.
    pixels.setPixelColor(0, 50, 0, 0);
    pixels.show();

    // enable interrupts
    sei();
}




void loop()
{	
    // Initiate a bme280 sensor class variable.
    // Attention: This doesnt work during the setup() function, because the I2C communication in the native Wire (TWI)library class will stuck.
    BME280Sensor bme280_sensor;


    // Following communication sets the control register of the RTC, for the 1Hz interrupt.
    // This is only needed one time, at the first use!
    //Wire.beginTransmission(RTC_I2C_ADDRESS);
    //Wire.write(0x0E);
    //Wire.write(0x40);
    //Wire.endTransmission();

    // Set the RTC time here.
    // This function should only be used, when its neccessary to set the correct time.
    //set_rtc_time(10, 42, 0);

  

    while(1)
    {	
        // Read and update the values of the BME280 sensor.
        int32_t humidity_buffer;
        refresh_sensor_data_on_display(bme280_sensor, &humidity_buffer);

        // Read and update the time values of the RTC.
        refresh_time_on_display();


        // handle the LED color, up to the humidity measurement.
        // This shall show how healthy the humidity is at the moment.
        // green --> good (50 - 60% relative humidity).
        // Increasing red means the humidity is too low. Increasing blue means the humidity is too high.
        char red_value = 0;
        char blue_value = 0;
        if(humidity_buffer < 50)red_value = abs(humidity_buffer - 50) * 3;
        if(humidity_buffer > 60)blue_value = abs(humidity_buffer - 60) * 3;
        pixels.setPixelColor(0, 50, red_value, blue_value);
        pixels.show();


        // enter sleep mode, no need for an active controller, until the next display refresh.
        // Extern signal of RTC will trigger an interrupt and wake up the controller.
        SMCR |= (1 << 0);
        SMCR |= (1 << 2);
        sleep_cpu();
    }
}
