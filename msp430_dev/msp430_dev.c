#include "../msp430_dev.h"
#include <stdint.h>
#include <msp430fr5994.h>

void dev_init_led(void)
{
    P1DIR |= BIT0 | BIT1;
    P1OUT &= ~(BIT0 + BIT1);
}

void dev_begin_countdown(void)
{
  __delay_cycles(1000);
  for (int i = 0; i < 6; i++) {
    P1OUT ^= BIT0;
    __delay_cycles(500000);
  }

  P1OUT ^= BIT1;
  __delay_cycles(500000);
  P1OUT ^= BIT1;
}

void dev_blink(int count)
{
  for (int i = 0; i < count * 2 /* on and off */; i++) {
    P1OUT ^= BIT0;
    __delay_cycles(500000);
  }
}

void dev_end(void)
{
  P1OUT ^= BIT0 | BIT1;
}

void dev_clear_memory(uint16_t *start, uint16_t *end)
{
  uint16_t *temp = start;
  while (temp <= end) {
    *temp = 0;
    temp++;
  }
}
