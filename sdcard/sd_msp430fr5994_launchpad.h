#ifndef _MSP430FR5994_LAUNCHPAD_H
#define _MSP430FR5994_LAUNCHPAD_H

//For MSP430FR5994 LaunchPad SD SPI pins
#define CS_b        BIT0 //P4.0
                    #define CS_OUT_p        P4OUT
                    #define CS_DIR_p        P4DIR
                    #define CS_REN_p        P4REN
                    #define CS_SEL1_p       P4SEL1
                    #define CS_SEL0_p       P4SEL0 
#define MISO_b      BIT7 //P1.7
                    #define MISO_OUT_p      P1OUT
                    #define MISO_DIR_p      P1DIR
                    #define MISO_REN_p      P1REN
                    #define MISO_SEL1_p     P1SEL1
                    #define MISO_SEL0_p     P1SEL0 
#define MOSI_b      BIT6 //P1.6
                    #define MOSI_OUT_p      P1OUT
                    #define MOSI_DIR_p      P1DIR
                    #define MOSI_REN_p      P1REN
                    #define MOSI_SEL1_p     P1SEL1
                    #define MOSI_SEL0_p     P1SEL0 
#define SCLK_b      BIT2 //P2.2
                    #define SCLK_OUT_p      P2OUT
                    #define SCLK_DIR_p      P2DIR
                    #define SCLK_REN_p      P2REN
                    #define SCLK_SEL1_p     P2SEL1
                    #define SCLK_SEL0_p     P2SEL0 
#define DETECT_b    BIT2 //P7.2
                    #define DETECT_OUT_p    P7OUT
                    #define DETECT_DIR_p    P7DIR
                    #define DETECT_REN_p    P7REN
                    #define DETECT_SEL1_p   P7SEL1
                    #define DETECT_SEL0_p   P7SEL0 
// For MOSFET on SD_ENABLE line  
// There is no assoicated MOSFET on the launchpad.. assigned to an LED for testing purposes...
#define PWR_b    BIT1 //P1.1
                    #define PWR_OUT_p       P1OUT
                    #define PWR_DIR_p       P1DIR
                    #define PWR_REN_p       P1REN
                    #define PWR_SEL1_p      P1SEL1
                    #define PWR_SEL0_p      P1SEL0 
#define MODULE       UCB0

//#defines for the diskio.c files
#define UCxxIFG UCB0IFG
#define UCxxTXBUF UCB0TXBUF
#define UCxxSTATW UCB0STATW
#define UCxxRXBUF UCB0RXBUF
#define UCxxCTLW0 UCB0CTLW0
#define UCxxBR0 UCB0BR0
#define UCxxBR1 UCB0BR1

#endif
