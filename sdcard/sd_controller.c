#include <msp430fr5994.h>

void __attribute__((section(".upper.text"))) uSD_PWR_gate_on()
{
  // Set the power pin LOW to turn the mosfet on
  P3DIR |= BIT1;
  P3OUT &= ~(BIT1);
}

void __attribute__((section(".upper.text"))) uSD_PWR_gate_off()
{
  // Set the power pin HIGH to turn the mosfet off
  P3DIR |= BIT1;
  P3OUT |= BIT1;
}