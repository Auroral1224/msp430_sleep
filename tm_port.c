#include "tm_port.h"

volatile uint32_t timer_overflows = 0;

// Timer_A overflow interrupt service routine
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A_ISR(void)
{
    switch(__even_in_range(TA0IV, TA0IV_TAIFG))
    {
        case TA0IV_TAIFG:
            timer_overflows++;
            break;
        default:
            break;
    }
}
