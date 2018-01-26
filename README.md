# Tea-Timer
Tea timer by ATtiny85V AVR 8bit microcontroller.
![product](https://github.com/jejdacz/Tea-Timer/blob/master/tea_timer_product.jpg)

## Problem statement
We need a timer for tea steeping. 
The timer should be as simple as possible and it has to meet the following requirements:
* Adjustable countdown time, that can be set in 30s steps.
* Visual and audio feedback on user and device activity.
* Single button control.
* Automatic power down when device is idle.

## Preamble:
For this application I decided to use ATtiny85V, partly because I have had the one at home and because I have wanted to do a mini project with this controller.
I use USBASP programmer on my PC with Ubuntu 16.04. I decided to do not use any IDE. So I used only Gedit text-editor and Makefile.
The debugging was done only by an observation of the device behaviour, but the standard is to use a serial communication with a computer for that.
The biggest pitfall of the project was to make flashing with AVRDUDE functional. Because ATtiny was not able to cooperate at full speed of USBASP it responded with a wrong device signature.
Which I realized after a few hours of crawling the Internet. The solution was using the option -B with a working bit-rate value.
The next hard thing was setting the timers. I'm very grateful for tutorials at https://maxembedded.wordpress.com/2011/06/22/introduction-to-avr-timers/ .

## Analysis:
The parts we use for this project are: 1x ATtiny85V , 1x LED, 1x piezo buzzer, 1x pushbutton, 1x resistor 10kOhm and 1x resistor 1kOhm.
A suitable power source could be lithium 3V battery.
CPU clock source of ATtiny is internal oscillator by default running at 8MHz prescaled to 1MHz which is fully acceptable.
We need to use the both ATtiny's timers here. The first one for timings and the second one as a PWM generator.
PWM is necessary for the piezo buzzer and for controlling a brightness of the led. The timer1 provides outputs from comparator A and B which are used for a pulse width adjustment. 
The tea-timer will be controlled only by a single pushbutton. We need to turn on internal pull up resistor for a pushbutton pin.

#### Wiring:
![wiring](https://github.com/jejdacz/Tea-Timer/blob/master/sketch_teatimer.jpg)

A gist of the firmware will be handling the states of the device.
I have made a state diagram for better understanding.

#### State diagram:
![state diagram](https://github.com/jejdacz/Tea-Timer/blob/master/state_d.jpg)

For the details check the source code.

## Usage:
Push the button to wake up the tea-timer. The tea-timer goes to the setup mode.
The led at low brightness signals the device is on.
In the setup mode set the countdown time by a number of button clicks where the each click adds 30 seconds.
The led signals a confirmed button click by a blink at full brightness.
Maximum delay between clicks is 3 seconds. When no clicks are performed within 10 seconds the device is put into the sleep mode.
The countdown is started automatically in 3 seconds after last click.
The led signals the countdown by a breathe mode. Clicking the button within countdown turns the device to the sleep mode.
When the countdown is finished an alarm is activated.When the alarm is finished the device put itself into the sleep mode.

