
#include <msp430.h>

#include "bsp/bsp_watchdog_timer.h"
#include "jetson_watchdog.h"

// Called every 30ms
void __attribute__((interrupt(TIMER1_B1_VECTOR))) isr_T1B1(void)
{
	if( TB1IV & TBIV__TBIFG ) {
		// led_toggle( LED1 );
		bsp_watchdog_touch();

		jetson_watchdog_service();

		// Clear LPM0 bits on exit (Wake up!)
		__bic_SR_register_on_exit(LPM0_bits);
	}
}
