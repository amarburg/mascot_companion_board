
#include "msp430.h"

#include "pwm.h"
#include "bsp_timer.h"



void bsp_timer_init( void )
{
// • Write 1 to the TBCLR bit (TBCLR = 1) to clear TBxR, clock divider state, and the counter direction.
// • If necessary, write initial counter value to TBxR.
// • Initialize TBxCCRn.
// • Apply desired configuration to TBxIV, TBIDEX, and TBxCCTLn.
// • Apply desired configuration to TBxCTL including to MC bits.

	// Stop and reset timer
	bsp_timer_stop();
	TB1CTL |= TBCLR;

	// Timer runs from SMCLK (1MHz)
	// divided by 8 (8us per bit)
	// with a 12-bit counter (32.768 ms/rollover = 30Hz)
	TB1CTL = TBSSEL__SMCLK | ID__8 | CNTL__12;

	TB1CCR1 = 0x00;
	TB1CCR2 = 0x00;

	// Reset/set mode
	TB1CCTL1 = OUTMOD_7;
	TB1CCTL2 = OUTMOD_7;

	// Enable to overflow interrupt
	TB1CTL |= TBIE_1;

	pwm_set( TIMER1, 0 );
	pwm_set( TIMER2, 0 );

	// Start with the TB1.1 and TB1.2 dio pins disconnected from the timer block

	P2SEL0 &= ~(BIT0 | BIT1);
	P2SEL1 &= ~(BIT0 | BIT1);

	P2DIR |= BIT0 | BIT1;
	P2OUT &= ~(BIT0 | BIT1);


	bsp_timer_start();
}

void bsp_timer_start( void )
{
	TB1CTL |= (MC__CONTINUOUS | TBCLR);
}

void bsp_pwm_set( uint8_t which, uint16_t scalar )
{
	uint8_t bit = (which == TIMER1) ? BIT0 : BIT1;

	if( scalar == 0 ) {
		//scalar = 0 means pull the pin to ground
		P2OUT  &= ~bit;
		P2SEL0 &= ~bit;

	} else {
		P2SEL0 |=  bit;
		// P2SEL1 &= ~bit;

		if( which == TIMER1 ){
			TB1CCR1 = scalar;
		} else if( which == TIMER2 ) {
			TB1CCR2 = scalar;
		}
	}
}

uint16_t bsp_pwm_get( uint8_t which )
{
	if( which == TIMER1 )
		return TB1CCR1;
	else if( which == TIMER2 )
		return TB1CCR2;

	return 0;
}

void bsp_timer_stop( void )
{
	// Set stop mode
	TB1CTL &= ~(MC__CONTINUOUS | MC__UP);
}
