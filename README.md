# Tea-Timer
Tea timer by ATtiny85V AVR 8bit microcontroller.

## Problem statement
We need a timer for tea steeping. 
The timer should be as simple as possible and it has to meet the following requirements:
* Adjustable countdown time, that can be set in 30s steps.
* Visual and audio feedback on user and device activity.
* Single button control.
* Automatic power down when device is idle.

## Usage:
Push the button to wake up the tea-timer. The tea-timer goes to the setup mode.
The led at low brightness signals the device is on.
In the setup mode set the countdown time by a number of button clicks where the each click adds 30 seconds.
The led signals a confirmed button click by a blink at full brightness.
Maximum delay between clicks is 3 seconds. When no clicks are performed within 10 seconds the device is put into the sleep mode.
The countdown is started automatically in 3 seconds after last click.
The led signals the countdown by a breathe mode. Clicking the button within countdown turns the device to the sleep mode.
When the countdown is finished an alarm is activated.When the alarm is finished the device put itself into the sleep mode.

## State diagram
![State diagram](https://github.com/jejdacz/Tea-Timer/blob/master/state_d.jpg)
