/**
 * teatimer.c
 *
 * The tea steep timer.
 *
 *  Created on: January 6, 2018
 *      Author: Marek Mego
 *
 * Usage:
 * Push the button to wake up the tea-timer. The tea-timer goes to the setup mode.
 * The led at low brightness signals the device is on.
 * In the setup mode set the countdown time by a number of button clicks
 * where the each click adds 30 seconds.
 * The led signals a confirmed button click by a blink at full brightness.
 * Maximum delay between clicks is 3 seconds. 
 * When no clicks are performed within 10 seconds the device is put into the sleep mode.
 * The countdown is started automatically in 3 seconds after last click.
 * The led signals the countdown by a breathe mode. 
 * Clicking the button within countdown turns the device to the sleep mode.
 * When the countdown is finished an alarm is activated.
 * When the alarm is finished the device put itself into the sleep mode.
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <math.h>

// CPU frequency [Hz]; 8MHz / 8 prescaler = 1MHz
#define F_CPU 1000000UL

#include <util/delay.h>


// button pin
#define BTN_PIN PB2

// setup countdown in ms
#define SETUP_CDWN 3000
#define SETUP_IDLE 10000

// input read delay in ms (switch bounce elimination)
#define IR_DUR 10

// led attributes
#define LED_PIN PB4
#define LED_BR_SETUP 40
#define LED_BR_FULL 250
#define LED_BR_BREATHE_MIN 5
#define LED_BR_BREATHE_MAX 100
#define LED_BR_BREATHE_CYCLE 2500
#define LED_BR_NONE 0

// define speaker pin
#define SPK_PIN PB1

// counter of microseconds
volatile uint32_t micros_cnt;

// steep time
uint8_t minute_cnt;

// button last read time stamp
uint32_t btn_last_read_time;

// button last read state HIGH / LOW
uint8_t btn_pin_last_read;

// button state
enum btn_state { B_IDLE, B_PRESSED, B_HOLD, B_RELEASED };
uint8_t btn_st;

// device state
enum dev_state { D_IDLE, D_SETUP, D_COUNTDOWN, D_ALARM };
uint8_t dev_st;

// functions' prototypes
void on_wakeup();
void reset();
void micros_init();
uint32_t micros();
uint32_t millis();
void pwm_init();
void led(uint8_t br);
void led_on();
void led_off();
void spk_on();
void spk_off();
void read_button();
void go_sleep();


/**
 * Updates micros on TIMER0 overflow.
 */
ISR(TIMER0_OVF_vect)
{    
    micros_cnt++;     
}

/**
 * Wake up interrupt.
 */ 
ISR(INT0_vect)
{
	// remove the interrupt mask for button so the interrupt is handled only once
	GIMSK &= ~(1<<INT0);
}

 
int main (void)
{	
	
	/*** INIT ***/
	
	
	// init timer1 - PWM
	pwm_init();
	
	// init timer0 - real time counter
	micros_init();
	
	// init button pin pull-up
	PORTB |= (1 << BTN_PIN);
	
	// initial button state
	btn_st = B_IDLE;
	
	//btn_last_read_time = millis();
	//btn_pin_last_read = (PINB & (1 << BTN_PIN));	
	
	// set sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	// reset device	
	reset();
	
	// go to sleep mode
	go_sleep();	
	
	
	/*** THE MAIN LOOP ***/
	
	
	while (1)
	{
	
	  	
  	/*** SETUP ***/
  	
  		
  		if (dev_st == D_IDLE)
  		{  		
	  		uint32_t sc = millis();
	  		uint8_t setup = 0;
	  		  		
	  		while (1)
	  		{	  		
				read_button();
				
				// handle click
				if (btn_st == B_PRESSED)
				{
					led(LED_BR_FULL);    	
					spk_on();
					
					_delay_ms(70);
				
					led(LED_BR_SETUP);
					spk_off();
									
					minute_cnt++;
					
					// wait for button release
					while (btn_st != B_RELEASED)
					{
						read_button();
					}
					
					sc = millis();				
				
					if (dev_st == D_IDLE) dev_st = D_SETUP;
				}
				
				// go sleep when idle for 10s
				if (dev_st == D_IDLE && (millis() - sc) > SETUP_IDLE)
				{
					go_sleep();
					break;
				}
				
				// start countdown 3s after last click
				if (dev_st == D_SETUP && (millis() - sc) > SETUP_CDWN)
				{
					dev_st = D_COUNTDOWN;
					break;
				}				 
			}
		}
		
		
	/*** COUNTDOWN ***/
	
		
		if (dev_st == D_COUNTDOWN)
		{			
			uint32_t cdwn = minute_cnt * 1000UL * 5;
			uint8_t led_br = 0;		
			uint8_t led_rg = LED_BR_BREATHE_MAX - LED_BR_BREATHE_MIN; // brightness range		
			uint8_t led_ph = 1;			
			uint32_t sc = millis();						
			
			while(1)
			{
				// button check
				read_button();
				
				if (btn_st == B_RELEASED)
				{				
					go_sleep();
					break;				
				}
				
				// check countdown finished		
				if ((millis() - sc) > cdwn)
				{
					dev_st = D_ALARM;
					break;
				}
		
				// led breathe
				uint8_t tmp_led_br = (((millis() - sc) % LED_BR_BREATHE_CYCLE) * led_rg) / LED_BR_BREATHE_CYCLE;
						
				// check for phase change
				if (led_br > tmp_led_br)
				{				
					led_ph ^= 1;
				}
						
				led_br = tmp_led_br;
			
				// assign brightness
				if (led_ph)
				{				
					led(LED_BR_BREATHE_MIN + (led_rg * (1 + cos( M_PI + ((double)led_br / led_rg) * ( M_PI / 2 ) ))));
				}
				else
				{				
					led(LED_BR_BREATHE_MAX - (led_rg * (cos( (3 * M_PI / 2) + ((double)led_br / led_rg) * ( M_PI / 2 ) ))));				
				}				
					
			}			
		}
		
		
	/*** ALARM ***/
	
		
		if (dev_st == D_ALARM)
		{			
			uint8_t alm_sch = 0b0000101;
			uint8_t alm_sch_pos = 0;
			int16_t alm_b_dur = 70;				
			uint8_t alm_rpt = 3;		
		
			// ensures immediate start
			uint32_t sc = millis() - alm_b_dur;
					
			while(1)
			{				
				// button check
				read_button();
				
				if (btn_st == B_RELEASED)
				{
					go_sleep();	
					break;	
				}
				
				// check timer			
				if (millis() - sc > alm_b_dur)
				{
					// check end of cycle
					if ((1 << alm_sch_pos) & 0b10000000)
					{
						alm_rpt--;
					
						// alarm finished go sleep
						if (alm_rpt < 1) {
						
							go_sleep();
							break;
						}
						else
						{
							alm_sch_pos = 0;
						}
					}
				
					if ((1 << alm_sch_pos) & alm_sch)
					{
						led(LED_BR_FULL);	  		
		  				spk_on();
					}
					else
					{
						led(LED_BR_NONE);
		  				spk_off();
					}
				
					// process the next bit
					alm_sch_pos++;
				
					// reset timer
					sc = millis();
				}
					
			}
		}
		
	}
 
  	return 1;
}


/**
 * Inits TIMER0 for micros.
 */
void micros_init()
{
	// init counter
	micros_cnt = 0;
	
	// enable TIMER0, set prescaler to CK/1	
	TCCR0B |= (1<<CS00);
		
	// enable TIMER0 overflow interrupt
	TIMSK |= (1<<TOIE0);
	
	// enable interrupts
	sei();
}

/**
 * Returns number of microseconds from start.
 * The micros func counts the number of timer0 overflows.
 * The timer0 cycle takes 256 us.
 * TCNT0 is register with a timer0 actual value.
 */
uint32_t micros()
{	
	return micros_cnt * 256 + TCNT0;
}

/**
 * Returns number of milliseconds from start.
 */
uint32_t millis()
{
	return micros() / 1000;
}

/**
 * Inits PWM mode on TIMER1.
 */
void pwm_init()
{		
	/**
	 * Init TIMER1 at 4KHz clock.
	 * Timer1 source is set to cpu clock.
	 * CPU CLOCK: 8MHz internal OSC with prescaler 8 = 1MHz
	 *
	 * Timer1   f = prescaler * clock / OCR1C + 1
	 *			4000 = 1 * 1000.000 / 249 + 1
	 *
	 * TCCR1 - timer/counter1 control register
	 * CS10 - prescaler set to CK/1
	 * PWM1A - enable PWM mode on channel A
	 * COM1A1 - comparator settings - OC1A cleared on compare match. Set when TCNT1 = 00
	 * GTCCR - general timer/counter1 control register
	 * PWM1B - enable PWM mode on chanel B
	 * COM1B1 - comparator settings - OC1B cleared on compare match. Set when TCNT1 = 00
	 * OCR1A - comparator A match value
	 * OCR1C - timer/counter counts up to OCR1C value then TOV1 flag is set
	 * OCR1B - comparator B match value
	 */
	 
	TCCR1 |= (1<<CS10) | (1<<PWM1A) | (1<<COM1A1);
	GTCCR |= (1<<PWM1B) | (1<<COM1B1);
		
	// init compare A value
	OCR1A = 249 / 2;
	
	// timer reset value
	OCR1C = 249;			
}

/**
 * Speaker on.
 */
void spk_on()
{
	DDRB |= (1<<SPK_PIN);
}

/**
 * Speaker off.
 */
void spk_off()
{
	DDRB &= ~(1<<SPK_PIN);
}

/**
 * Led on.
 */
void led_on()
{
	DDRB |= (1<<LED_PIN);
}

/**
 * Led off.
 */
void led_off()
{
	DDRB &= ~(1<<LED_PIN);
}

/**
 * Sets led brightness.
 */
void led(uint8_t br)
{
	OCR1B = br;
}

/**
 * Reads and handles button state.
 */
void read_button() {
	
	// read pin
	uint8_t reading = (PINB & (1 << BTN_PIN));
	
	// ensure the same value was read for IR_DUR period	
	if (reading != btn_pin_last_read)
	{
		btn_last_read_time = millis();
		btn_pin_last_read = reading;
		return 0;
	}	
	
	if ((millis() - btn_last_read_time) < IR_DUR)
	{
		return 0;
	}
			
	// resolve pin reading
	if (reading == 0)
	{
		if (btn_st == B_PRESSED || btn_st == B_HOLD)
		{
			btn_st = B_HOLD;
			return 0;
		}
				
		btn_st = B_PRESSED;
		return 1;
	}
	else
	{			
		if (btn_st == B_RELEASED || btn_st == B_IDLE)
		{
			btn_st = B_IDLE;
			return 0;
		}
		btn_st = B_RELEASED;		
		return 1;			
	}							
}

/**
 * Go sleep routine.
 */
void go_sleep()
{	
	// sleep mode signalization
	led(LED_BR_FULL);	
	spk_on();
	
	_delay_ms(70);

	led(LED_BR_NONE);
	led_off();
	spk_off();
		
	// apply interrupt mask for button
	GIMSK |= (1<<INT0);
	
	// activate sleep mode
	sleep_enable();
	sleep_cpu();		  
	sleep_disable();
	
	/* WAKE UP */
	
	// reset device
	reset();
	
	// button is held on wake up
	btn_st = B_HOLD;
		
	// wait for button release on wake up	
	while (btn_st != B_IDLE)
	{
		read_button();
	}
}

/**
 * Reset routine.
 */
void reset() {
	
	// set device to defaults
	minute_cnt = 0;
	btn_last_read_time = millis();
	btn_pin_last_read = (PINB & (1 << BTN_PIN));	
			
	dev_st = D_IDLE;
	
	led_on();
	led(LED_BR_SETUP);
}
