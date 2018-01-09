/**
 * teatimer.c
 *
 * The tea steep timer.
 *
 *  Created on: January 6, 2018
 *      Author: Marek Mego
 *
 * Usage:
 * Push the button to wake up the teatimer. The led signalizes operating mode
 * by low brightness.
 * Set time to countdown by number of button clicks where one click is 30 seconds.
 * The led signalizes a confirmed button click by a blink at 100% of brightness.
 * Maximum delay between taps is 3 seconds.
 * The countdown is started automaticaly in 3 seconds after last tap.
 * The led signalizes the countdown by a brathe mode.
 * Tapping the button within coundown turns device to sleep mode.
 * When the countdown is finished the alarm is activated.
 * When the alarm is finished the device put itself into sleep mode.
 *
 * Different button (switch) handling. January 7, 2018
 * TODO: code comments
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <math.h>

#define F_CPU 1000000UL

#include <util/delay.h>


// button pin
#define BTN_PIN PB2

// setup countdown in ms
#define SETUP_CDWN 3000
#define SETUP_IDLE 10000

// safety input read delay in us
// bounce elimination
#define IR_DUR 3*1000

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

// steep time
uint8_t minute_cnt;

uint32_t btn_last_read;

// button state
enum btn_state { B_IDLE, B_PRESSED, B_HOLDING, B_RELEASED };

enum btn_state btn_st;

// device state
enum dev_state { D_IDLE, D_SETUP, D_COUNTDOWN, D_ALARM };

enum dev_state dev_st;

// functions prototypes
void on_wakeup();

// counter of microseconds
volatile uint32_t micros_cnt;

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
 * Updates micros on TIMER0 overflow.
 */
ISR(TIMER0_OVF_vect)
{    
    micros_cnt++;     
}

/**
 * Returns number of microseconds from start.
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
	 * PWM1A - enable PWM mode on chanel A
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
 * Sets led brightness.
 */
void led(uint8_t br)
{
	OCR1B = br;
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
 * Reads and handles button state.
 */
void read_button() {
	
	if ((micros() - btn_last_read) < IR_DUR)
	{
		return;
	}
	else
	{
		btn_last_read = micros();
	}
		
		if (btn_st == B_IDLE && ((PINB & (1 << BTN_PIN)) == 0))
		{
			btn_st = B_PRESSED;			
		}				
		else if (btn_st == B_PRESSED || btn_st == B_HOLDING)
		{			
			if (PINB & (1 << BTN_PIN))
			{
				btn_st = B_RELEASED;
			}
			else
			{
				btn_st = B_HOLDING;
			}
		}		
		else if (btn_st == B_RELEASED)
		{
			btn_st = B_IDLE;
		}				
}

/**
 * Wake up interrupt.
 */ 
ISR(INT0_vect)
{
	// remove interrrupt mask for button
	GIMSK &= ~(1<<INT0);
}

/**
 * Go sleep routine.
 */
void go_sleep()
{	
	
	led(LED_BR_NONE);
	led_off();
	spk_off();
		
	// apply interrupt mask for button
	GIMSK |= (1<<INT0);
	
	sleep_enable();
	sleep_cpu();		  
	sleep_disable();
	
	on_wakeup();
}

/**
 * Wake up routine.
 */
void on_wakeup() {
		
	read_button();
	
	while (btn_st != B_RELEASED)
	{
		read_button();
		_delay_ms(5);
	}
	
	// set device to defaults
	minute_cnt = 0;
	btn_last_read = 0;
	btn_st = B_IDLE;	
	dev_st = D_IDLE;
	
	led_on();
	led(LED_BR_SETUP);
}
 
int main (void)
{		
	pwm_init();
	
	micros_init();
	
	// init button pin pullup
	PORTB |= (1 << BTN_PIN);
			
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	// wake up on power up 	
	on_wakeup();	
	
	while (1)
	{
	  	
  		/* SETUP */
  		
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
				
					sc = millis();
				
					if (dev_st == D_IDLE) dev_st = D_SETUP;						
				}
				
				// go sleep when idle for 10s
				if (dev_st == D_IDLE && (millis() - sc) > SETUP_IDLE)
				{
					led(LED_BR_FULL);    	
					spk_on();
					
					_delay_ms(70);
				
					led(LED_BR_NONE);
					spk_off();
					
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
		
		
		/* COUNTDOWN */
		
		if (dev_st == D_COUNTDOWN)
		{			
			uint32_t cdwn = minute_cnt * 1000UL * 30;
			uint8_t led_br = 0;		
			uint8_t led_rg = LED_BR_BREATHE_MAX - LED_BR_BREATHE_MIN; // brightness ranage		
			uint8_t led_ph = 1;			
			uint32_t sc = millis();						
			
			while(1)
			{
				// button check
				read_button();
				
				if (btn_st == B_RELEASED)
				{
					led(LED_BR_FULL);    	
					spk_on();
					
					_delay_ms(70);
				
					led(LED_BR_NONE);
					spk_off();
				
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
					led(LED_BR_BREATHE_MIN + (led_br * (1 + cos( M_PI + ((double)led_br / led_rg) * ( M_PI / 2 ) ))));
				}
				else
				{				
					led(LED_BR_BREATHE_MAX - (led_br * (cos( (3 * M_PI / 2) + ((double)led_br / led_rg) * ( M_PI / 2 ) ))));
				}				
					
			}			
		}
		
		
		/* ALARM */
		
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
					led(LED_BR_FULL);  	
					spk_on();
					
					_delay_ms(70);
				
					led(LED_BR_NONE);
					spk_off();
				
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


