# Using MSP430 with a Micro-SD Card

This code serves as a working demo of how to write data to a uSD card using an
MSP430FR5994 microcontroller. 

## Folder Structure: 

    ├── makefile    
    ├── msp430_dev          -> a small library of development functions
    │  └── msp430_dev.c
    ├── msp430_dev.h
    ├── README.md
    ├── sd_write_demo.c     -> 'main()' found here
    └── sdcard              -> the code contained in this directory is not my own,
       ├── diskio.c            other than sd_msp430fr5994_launchpad.h, which is
       ├── diskio.h            included in a few of Chan's files to enable functionality
       ├── ff.c                with my msp430 launchpad
       ├── ff.h
       ├── ffconf.h
       ├── integer.h
       ├── sd_controller.c
       ├── sd_controller.h
       └── sd_msp430fr5994_launchpad.h


The code in this repository uses a FAT file sd card library created by Chan, see: 
http://elm-chan.org/fsw/ff/00index_e.html

The only modifications I've made to the library are to include a custom header
file to direct the library to the appropriate pins for my board.
