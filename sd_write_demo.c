#include <msp430fr5994.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "msp430_dev.h"
#include "sdcard/diskio.h"
#include "sdcard/ff.h"
#include "sdcard/ffconf.h"
#include "sdcard/integer.h"

#define ONE_BYTE 8
#define TEST_BUFF_SIZE 150
#define END_TEST 100
#define HIFRAM_START (uint16_t*) 0x158F3 /* sd card library uses 0x10000 thru 0x158F2 */
#define SD_FILENAME "TEST01"
volatile uint16_t *hifram;

// Objects for sd card library usage
FIL file;
FATFS fatfs;
DIR dir;
FRESULT errCode;

unsigned int bytes_written;  /* sd card function clears this number prior to use */
int test_data[TEST_BUFF_SIZE] = {0};

int main(void)
{
  //--Disable watchdog timer
  WDTCTL = WDTPW | WDTHOLD;
  PM5CTL0 &= ~LOCKLPM5;

  //--Clock system setup
  CSCTL0_H = CSKEY >> ONE_BYTE;
  CSCTL1 = DCORSEL | DCOFSEL_0;
  CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;
  CSCTL3 = DIVA__1 | DIVS_5 | DIVM__1;
  CSCTL0_H = 0;

  //--Initialize test_data array
  for (int i = 0; i < TEST_BUFF_SIZE; i++)
      test_data[i] = i;

  //--Signal user of program beginning
  dev_init_led();
  dev_begin_countdown();

  //--Initialize fat file system
  errCode = (FRESULT)-1;    /* cast '-1' as an FRESULT type. -1 is an invalid
                                error code just to let us know it was the 
                                initialization */

  while (errCode != FR_OK) {
    errCode = f_mount(&fatfs, "", 0);
    errCode = f_opendir(&dir, "/");
    errCode = f_open(&file, SD_FILENAME, FA_CREATE_ALWAYS | FA_WRITE);
  }

  int test_loops = 0;
  while (test_loops < 1) {
    // TEST_BUFF_SIZE is multiplied by two because each int takes up 2 bytes of
    // space.
    f_write(&file, &test_data, (TEST_BUFF_SIZE * 2), &bytes_written); 
    f_sync(&file);
    test_loops++;
  }

  //--Signal end of execution
  dev_end();
}
