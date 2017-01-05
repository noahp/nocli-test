
#include "MKL26Z4.h"
#include "systick.h"
#include <string.h> // memset
#include "usb_main.h"
#include <stdlib.h> // strtoul
#include <stdbool.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "nocli.h"

//// need to define _sbrk for sprintf (malloc)
//extern int __end__;
//extern caddr_t _sbrk(int incr);
//extern caddr_t _sbrk(int incr)
//{
//    static unsigned char *heap = NULL;
//    unsigned char *prev_heap;
//
//    if(heap == NULL){
//        heap = (unsigned char *)&__end__;
//    }
//    prev_heap = heap;
//
//    heap += incr;
//
//    return (caddr_t)prev_heap;
//}

// makes heap go
caddr_t
_sbrk (int incr)
{
  extern char   end __asm ("end");
  extern char   heap_limit __asm ("__HeapLimit");
  static char * heap_end;
  char *        prev_heap_end;

  if (heap_end == NULL)
    heap_end = & end;

  prev_heap_end = heap_end;

  if (heap_end + incr > &heap_limit)
    {
#ifdef NIO_ENOMEM   //TODO: Update NIO error code for MQX
        errno = NIO_ENOMEM;
#else
        errno = ENOMEM;
#endif
      return (caddr_t) -1;
    }

  heap_end += incr;

  return (caddr_t) prev_heap_end;
}

static void main_init_io(void)
{
  // init ports
  // disable COP
  SIM_COPC = 0;

  // enable clocks for PORTB
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

  // set B0 to GPIO
  PORTB_PCR0 = PORT_PCR_MUX(1);

  // set output B0 low (LED on initially)
  GPIOB_PCOR = (1 << 0);

  // set B0 DDR to output
  GPIOB_PDDR |= (1 << 0);
}

static void delay_ms(uint32_t ms)
{
   uint32_t start = systick_getms();
   while(systick_getms() - start < ms);
}

static void main_init_uart(void)
{
  uint32_t baud;

  // init uart0
  // enable clocks for port a and uart0
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM_SOPT2 |= SIM_SOPT2_UART0SRC(1); // select MCGFLLCLK clock

  // configure io pins for uart0- alt 2
  PORTA_PCR1 = PORT_PCR_MUX(2);
  PORTA_PCR2 = PORT_PCR_MUX(2);

  // set output mode for TX, pa12
  GPIOA_PDDR |= (1 << 2);

  // set oversampling ratio to 25
  UART0_C4 = UART0_C4_OSR(25 - 1);

  // set baud rate to 9600
  baud      = (48000000 / 9600) / 25;            // <- baud = CLOCK /
                                                 // ((OSR+1)*BR)
  UART0_BDH = UART0_BDH_SBR((baud >> 8) & 0x1F); // upper 5 bits
  UART0_BDL = UART0_BDL_SBR(baud & 0xFF);        // lower 8 bits

  // turn on tx + rx
  UART0_C2 = UART0_C2_RE_MASK | UART0_C2_TE_MASK;

  //    // send a char?
  //    UART0_D = 'N';
}

#define DEFAULT_TIME_STR ("1200")
static char* main_get_time(void)
{
  static char ascii_time[sizeof(DEFAULT_TIME_STR)] = DEFAULT_TIME_STR;

  sprintf(ascii_time, "%2lu%02lu", (systick_getms() / 10 / 60 / 60) % 12,
          (systick_getms() / 10 / 60) % 60);

  return ascii_time;
}

static void main_led(void)
{
  static uint32_t blinkTime = 0;

  // blink every 250ms
  if(systick_getms() - blinkTime > 250){
    blinkTime = systick_getms();

    // toggle
    GPIOB_PTOR = (1 << 0);
  }
}

static int main_poll_uart(uint8_t *pChr)
{
  if (UART0_S1 & UART0_S1_RDRF_MASK) {
    *pChr = UART0_D;
    return 1;
  }

  return -1;
}

static void output(char *data, size_t length)
{
  for(size_t i=0; i<length; i++){
    delay_ms(2);
    usb_send_data((uint8_t*)&data[i], 1);
  }
}

size_t _write(int fd, const unsigned char *buf, size_t count){
  (void)fd;
  output((char*)buf, count);
  return count;
}

static void function1(int argc, char **argv){
    (void)argv;
    printf("[Executing function1] %d\r\n", argc);
    for(int i=0; i<argc; i++){
        printf("%s ", argv[i]);
    }
    printf("\r\n");
}

static void function2(int argc, char **argv){
    (void)argv;
    printf("[Executing function2] %d\r\n", argc);
}

int main(void)
{
  uint8_t   rxData;
  int       gotCharacter = -1;

  // initialize the necessary
  main_init_io();
  main_init_uart();

  // usb init
  usb_main_init();

  // setup cli
  // setup context
  static struct NocliCommand cmdlist[] ={
    {
      .name = "function1",
      .function = function1,
    },
    {
      .name = "function2",
      .function = function2,
    },
  };
  static struct Nocli nocli_ctx = {
    .output_stream = output,
    .command_table = cmdlist,
    .command_table_length = sizeof(cmdlist)/sizeof(cmdlist[0]),
    .prefix_string = "nocli$ ",
    .error_string = "error, command not found",
    .echo_on = true,
  };
  (void)Nocli_Init(&nocli_ctx);

  while (1) {
    // led task
    main_led();

    // usb task
    gotCharacter = usb_main_mainfunction(&rxData);

    // check for incoming uart characters if nothing received on usb
    if(gotCharacter == -1){
      gotCharacter = main_poll_uart(&rxData);
    }

    if(gotCharacter > 0){
      Nocli_Feed(&nocli_ctx, (char*)&rxData, 1);
    }
  }

  return 0;
}
