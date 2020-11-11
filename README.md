A fly wire circuit, including a BME280 sensor, a 16x2 LCD display, a RTC clock and one WS2812 8mm LED. Everything controlled by an Arduino Nano.
The LED uses the relative humidity sensor measurement to signalize the quality of the humidity in the air. Green means a good value of 50-60%.
Increasing from this is signalized by the LED getting more blue, while the LED is getting more red when the humidity is decreasing under 50%.
