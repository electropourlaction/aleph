/* main.c
   aleph-avr32
   
   calls hardware initialization
   processes event queue,
   sets default event handlers
 
 */

// asf
#include "compiler.h"
#include "board.h"
#include "conf_sd_mmc_spi.h"
#include "ctrl_access.h"
#include "delay.h"
#include "flashc.h"
#include "gpio.h"
#include "intc.h"
#include "pdca.h"
#include "power_clocks_lib.h"
#include "print_funcs.h"
#include "pm.h"
#include "sd_mmc_spi.h"
#include "smc.h"
#include "sysclk.h"
#include "usart.h"

//// aleph
// common
#include "fix.h"
#include "param_common.h"
#include "screen.h"
#include "simple_string.h"
#include "types.h"

// avr32
#include "aleph_board.h"
#include "adc.h"
#include "app.h"
#include "bfin.h"
#include "conf_tc_irq.h"
#include "encoders.h"
#include "events.h"
#include "filesystem.h"
#include "flash.h"
#include "font.h"
#include "ftdi.h"
#include "global.h"
#include "init.h"
#include "interrupts.h"
#include "memory.h"
#include "monome.h"
#include "switches.h"

//  block specific
#include "files.h"
#include "render.h"

//==================================================
//====  defines
// ad7923 control register
#define AD7923_CTL_WRITE  (1 << 11)
#define AD7923_CTL_SEQ1   (1 << 10)
#define AD7923_CTL_ADD1   (1 << 7)
#define AD7923_CTL_ADD0   (1 << 6)
#define AD7923_CTL_PM1    (1 << 5)
#define AD7923_CTL_PM0    (1 << 4)
#define AD7923_CTL_SEQ0   (1 << 3)
#define AD7923_CTL_RANGE  (1 << 1)
#define AD7923_CTL_CODING (1)

// command word to which we add the desired channel address for conversion
#define AD7923_CMD_BASE (AD7923_CTL_WRITE | AD7923_CTL_PM0 | AD7923_CTL_PM1 | AD7923_CTL_CODING)

#define SAMPLERATE 48000
#define BUFFERLEN (SAMPLERATE * 4)
#define BYTES (BUFFERLEN * 2)

//==================================================
//====  extern variables

//==================================================
//====  static variables
// flag for firstrun
static u8 firstrun = 0;
//  flag to wait for startup button press
static u8 launch = 0;
// flags for device connection events.
// need to re-send after app launch.
//static u8 ftdiConnect = 0;
static u8 monomeConnectMain = 0;
static u8 hidConnect = 0;
static u8 midiConnect = 0;

//=================================================
//==== static declarations

// initializations
static void init_avr32(void);
static void init_ctl(void);

// check startup status and possibly launch the application
static void check_startup(void);

// dummies
static void dummy_handler(s32 data) { ;; }
//static void dummy_render(void) { ;; }

// core event handlers

static void handler_FtdiConnect(s32 data) {
  ftdi_setup();
}

static void handler_FtdiDisconnect(s32 data) { 
  /// FIXME: assuming that FTDI == monome
  event_t e = { .type = kEventMonomeDisconnect };
  event_post(&e);
}

static void handler_MonomeConnect(s32 data) {
  // this just stores a flag to re-send connection event to app
  if(!launch) {
    // print_dbg("\r\n got monome device connection, saving flag for app launch");
    monomeConnectMain = 1;
  }
}
/*
static void handler_MonomePoll(s32 data) {
  monome_read_serial();
}

static void handler_MonomeRefresh(s32 data) {
  (*monome_refresh)();
  // FIXME: arc?
}
*/
static void handler_MidiConnect(s32 data) { 
  if(!launch) {
    midiConnect = 1;
  }
}
/*
static void handler_MidiRefresh(s32 data) {
  // TODO ??
}
*/

static void handler_HidConnect(s32 data) { 
  if(!launch) {
    hidConnect = 1;
  }
}
/*
// to be app defined
static void handler_Serial(s32 data) { 
    ;;
}
*/
 
/// explicitly assign default event handlers.
/// this way the order of the event types enum doesn't matter.
static inline void assign_main_event_handlers(void) {
  app_event_handlers[ kEventAdc0 ]      = &dummy_handler ;
  app_event_handlers[ kEventAdc1 ]      = &dummy_handler ;
  app_event_handlers[ kEventAdc2 ]      = &dummy_handler ;
  app_event_handlers[ kEventAdc3 ]      = &dummy_handler ;
  app_event_handlers[ kEventEncoder0 ]	= &dummy_handler ;
  app_event_handlers[ kEventEncoder1 ]	= &dummy_handler ;
  app_event_handlers[ kEventEncoder2 ]	= &dummy_handler ;
  app_event_handlers[ kEventEncoder3 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch0 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch1 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch2 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch3 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch4 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch5 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch6 ]	= &dummy_handler ;
  app_event_handlers[ kEventSwitch7 ]	= &dummy_handler ;
  app_event_handlers[ kEventTwi ]       = &dummy_handler ;
  app_event_handlers[ kEventFtdiConnect ]	= &handler_FtdiConnect ;
  app_event_handlers[ kEventFtdiDisconnect ]    = &handler_FtdiDisconnect ;
  app_event_handlers[ kEventMonomeConnect ]	= &handler_MonomeConnect ;
  app_event_handlers[ kEventMonomeDisconnect ]	= &dummy_handler ;
  app_event_handlers[ kEventMonomeGridKey ]	= &dummy_handler ;
  app_event_handlers[ kEventMonomeRingEnc ]	= &dummy_handler ;
  app_event_handlers[ kEventMidiConnect ]	= &handler_MidiConnect ;
  app_event_handlers[ kEventMidiDisconnect ]	= &dummy_handler ;
  app_event_handlers[ kEventMidiPacket ]	= &dummy_handler ;
  app_event_handlers[ kEventHidConnect ]	= &handler_HidConnect ;
  app_event_handlers[ kEventHidDisconnect ]	= &dummy_handler ;
}

//=================================================
//==== definitons

// low-level init called by startup_uc3.S assembly.  So we can init
// the static mem ctlr before C runtime starts up and use external SDRAM for the heap.
int _init_startup(void);
int _init_startup(void) {
  // clocks
  // setup clocks
  sysclk_init();

  // not sure why but when need to explictly enable clock for static mem ctlr
  sysclk_enable_pbb_module(SYSCLK_SMC_REGS);
  flashc_set_bus_freq(FCPU_HZ);
  flashc_set_wait_state(1);

  /// interrupts
  irq_initialize_vectors();

  // serial usb
  init_ftdi_usart();

  print_dbg(" ++++++++++++++++ Initializing SMC / SDRAM\r\n ");

  // external sram
  smc_init(FHSB_HZ);

  // init will continue below after app main() is called from the C runtime.
  return 0;
}


// top-level peripheral init
static void init_avr32(void) {
  
  // fixme: test malloc for SDRAM paranoia
  test_malloc();

  // disable all interrupts for now
  cpu_irq_disable();

  // initialize spi1: OLED, ADC, SD/MMC
  init_spi1();
  // initialize PDCA controller
  init_local_pdca();
  // initialize blackfin resources
  init_bfin_resources();
  // initialize application timer
  init_tc();
  // initialize other GPIO
  init_gpio();
  // init twi
//  init_twi();
  // register interrupts
  register_interrupts();
  // initialize the OLED screen
  init_oled();
  // enable interrupts
  cpu_irq_enable();
  // usb host controller
  init_usb_host();
  // initialize usb class drivers
  init_monome();

  print_dbg("\r\n ++++++++++++++++ avr32 init done ");
}

// init timer-related stuff.
static void init_ctl(void) {
  // disable interrupts
  cpu_irq_disable();

  // intialize the event queue
  init_events();
  print_dbg("\r\n init_events");

  // intialize encoders
  init_encoders();
  print_dbg("\r\n init_encoders");

  // send ADC config
  init_adc();
  print_dbg("\r\n init_adc");
  
  // enable interrupts
  cpu_irq_enable();
  print_dbg("\r\n enabled interrupts");
}

// launch application
void check_startup(void) {
  event_t e = { .data = 0 };

  if(!launch) {
    //// haven't launched yet
    // wipe out the event queue
//    init_events();
    // clear the power sw interrupt? wtf?? ok
    gpio_clear_pin_interrupt_flag(SW_POWER_PIN);
    // return 1 if app completed firstrun tasks
    launch = app_launch(firstrun);

    delay_ms(10);

    if(launch) {

      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // you must do this, or the aleph will powercycle forever !!
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // pull up power control pin, enabling soft-powerdown
      gpio_set_gpio_pin(POWER_CTL_PIN);

      if(firstrun) {
	// successfully launched on firstrun, so write magic number to flash
	flash_write_firstrun();
      }
      if(hidConnect) {
	e.type = kEventHidConnect;
	event_post(&e);
      }
    } else {      
      if( firstrun) {
	// firstrun, but app launch failed, so clear magic number to try again
	flash_clear_firstrun();
      } 
    }
  }
}

// !!!!!!!!!!!!!
// main function
int main (void) {

  // set up avr32 hardware and peripherals
  init_avr32();
    
  // show the startup screen
  screen_startup();  

  //memory manager
  init_mem();  
  print_dbg("\r\n init_mem");

  // intialize the FAT filesystem
  fat_init();
  print_dbg("\r\n init filesystem");

  // setup control logic
  init_ctl();
  print_dbg("\r\n init ctl");

  // initialize the application
  app_init();
  print_dbg("\r\n init app");

  // initialize flash
  firstrun = init_flash();

  // check sw2 and force firstrun if held
  if(gpio_get_pin_value(SW2_PIN)) {
    firstrun = 1;
    print_dbg("r\n sw2 down -> force firstrun ");
  }

  if(gpio_get_pin_value(SW0_PIN)) {
    firstrun = 1;
    print_dbg("r\n sw0 down -> force firstrun ");
  }

  // assign default event handlers
  assign_main_event_handlers();
  print_dbg("\r\n assigned default/dummy event handlers");

  print_dbg("\r\n starting event loop.\r\n");

  check_startup();

    //  allocate buffer
    s16 *buf = (s16*)alloc_mem(BUFFERLEN * sizeof(s16));

    //  print menu
    render_boot("sw 1 : new");
    render_boot("sw 2 : record");
    render_boot("sw 3 : view");
    render_boot("sw 4 : save");
    
  while(1)
  {
      int unsigned n;
      volatile unsigned int i;
      
      static u8 sw = 4;

      //  detect switch action
      if (gpio_get_pin_value(SW0_PIN)) sw = 0;
      else if (gpio_get_pin_value(SW1_PIN)) sw = 1;
      else if (gpio_get_pin_value(SW2_PIN)) sw = 2;
      else if (gpio_get_pin_value(SW3_PIN)) sw = 3;
      else if (!gpio_get_pin_value(SW_POWER_PIN)) sw = 5;
      else ;
      
      switch(sw)
      {
          case 0 :
              //  led on
              gpio_clr_gpio_pin(LED_MODE_PIN);

              //  clear buffer
              for (n=0; n<BUFFERLEN; ++n)
              {
                  buf[n] = 0;
              }

              //  create and open new file
              files_new_sample();
              
              //  led off
              gpio_set_gpio_pin(LED_MODE_PIN);
              
              //  wait..
              while(gpio_get_pin_value(SW0_PIN));;
              sw = 4;
              break;

          case 1 :
              //  disable interrupts
              cpu_irq_disable();
              
              delay_ms(10);
              
              //  initiate bfin test tone
              bfin_wait();
              spi_selectChip(BFIN_SPI, BFIN_SPI_NPCS);
              
              //  initiate adc input
              spi_selectChip(ADC_SPI, ADC_SPI_NPCS);
              
              //  record to buffer
              for (n=0; n<BUFFERLEN; ++n)
              {
                  const u16 cmd = (AD7923_CMD_BASE) << 4;
                  uint16_t *data = 0;
                  
                  //  fine tune timing to 48khz
                  i = 0;
                  while (i<70)
                  {
                      ++i;
                  }
                  nop();

                  //  write command to ad7923
                  ADC_SPI->tdr = cmd << AVR32_SPI_TDR_TD_OFFSET;
                  while (!(ADC_SPI->sr & AVR32_SPI_SR_TXEMPTY_MASK)) { ;; };

                  //  read cv value
                  while (!(ADC_SPI->sr & (AVR32_SPI_SR_RDRF_MASK | AVR32_SPI_SR_TXEMPTY_MASK)));
                  *data = ADC_SPI->rdr >> AVR32_SPI_RDR_RD_OFFSET;

                  //  write to buffer
                  buf[n] = *data & 0xfff;

                  //  spi transfer to bfin (generates a pulse tone at 24kHz)
                  BFIN_SPI->tdr = MSG_PULSE_COM << AVR32_SPI_TDR_TD_OFFSET;
                  while (!(BFIN_SPI->sr & AVR32_SPI_SR_TXEMPTY_MASK)) { ;; };
              }
              
              //  end adc input
              spi_unselectChip(ADC_SPI, ADC_SPI_NPCS);
              
              //  end bfin test tone
              bfin_wait();
              spi_unselectChip(BFIN_SPI, BFIN_SPI_NPCS);
              
              //  enable interrupts
              cpu_irq_enable();

              //  wait..
              while(gpio_get_pin_value(SW1_PIN));;
              sw = 4;
              break;

          case 2 :
              //  render wave to screen
              
              //  wait..
              while(!(gpio_get_pin_value(SW2_PIN)));;
              sw = 4;
              break;

          case 3 :
              //  led on
              gpio_clr_gpio_pin(LED_MODE_PIN);

              //  save sample to file and close it
              files_save_sample(buf, BYTES);
              
              //  led off
              gpio_set_gpio_pin(LED_MODE_PIN);
              
              //  wait..
              while(gpio_get_pin_value(SW3_PIN));;
              sw = 4;
              break;
              
          case 4 :
              //  wait
              delay_ms(50);
              break;
              
          case 5 :
              //  power down
              gpio_clr_gpio_pin(POWER_CTL_PIN);
              break;
              
              
          default :
              break;
      }
  }
}