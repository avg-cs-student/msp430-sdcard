/*
 * msp430_dev.h: A collection of functions often used in development only.
 * Author: Justin Cromer
 */
#ifndef _AUR_DEV_H
#define _AUR_DEV_H

#include <stdint.h>

/**
 * dev_init_led(): Sets up MSP430 launchpad LED pins for I/O
 */
void dev_init_led(void);

/**
 * dev_begin_countdown(): Provides a visual cue to indicate beginning of execution.
 *
 * Uses LED's on MSP430 launchpad to give a Mario Kart-like indcation of the
 * start of program execution: 3x red followed by 1x green.
 */
void dev_begin_countdown(void);

/**
 * dev_blink(): Provides a visual cue to indicate points within execution.
 * @count:  number of blinks to execute
 *
 * Uses LED on MSP430 launchpad to give a visual indication of when a breakpoint
 * has been crossed.
 */
void dev_blink(int count);

/**
 * dev_end(): Provides a visual cue when an endpoint is reached
 *
 * Uses both red and green LED's on MSP430 launchpad to indicate that the end of
 * a code block has been executed. This function toggles the LED's to a permanent
 * 'on' state.
 */
void dev_end(void);

/**
 * dev_clear_memory(): Replaces all bytes in range with 0x0
 * @start:  Inclusive beginning address of memory to clear
 * @end:    Inclusive ending address of memory to clear
 *
 * Used for clearing a specific block of memory prior to an experiment to ensure
 * data integrity.
 */
void dev_clear_memory(uint16_t *start, uint16_t *end);

#endif
