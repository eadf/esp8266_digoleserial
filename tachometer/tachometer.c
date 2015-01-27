/*
 * tachometer.c
 *
 *  Created on: Jan 26, 2015
 *      Author: eadf
 */
#include "osapi.h"
#include "ets_sys.h"
#include "gpio.h"
#include "mem.h"
#include "tachometer/tachometer.h"
#include "os_type.h"

#define tachometer_micros (0x7FFFFFFF & system_get_time())

#define TACHOMETER_POLL_TIME 500 // 500ms

static volatile uint32_t   tachometer_timeStamp = 0;
static volatile uint32_t   tachometer_pulses = 0;
static volatile uint32_t   tachometer_sample = 0;
static volatile os_timer_t tachometer_timer;
static uint8_t tachometer_pin = 0;

// forward declarations
static void tachometer_disableInterrupt(void);
static void tachometer_enableInterrupt(void);
static void tachometer_intr_handler(void);
static void tachometer_timerFunc(void);
static bool tachometer_setupInterrupt(uint8 gpio_pin, void (*interruptHandler)(void));

static void
tachometer_disableInterrupt(void) {
  gpio_pin_intr_state_set(GPIO_ID_PIN(tachometer_pin), GPIO_PIN_INTR_DISABLE);
}

static void
tachometer_enableInterrupt(void) {
  gpio_pin_intr_state_set(GPIO_ID_PIN(tachometer_pin), GPIO_PIN_INTR_POSEDGE);
}

static void
tachometer_intr_handler(void) {
  uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  //clear interrupt status
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(tachometer_pin));
  tachometer_pulses+=1;
}

uint32_t ICACHE_FLASH_ATTR
tachometer_getSample(void) {
   return tachometer_sample;
}

void ICACHE_FLASH_ATTR
tachometer_timerFunc(void) {
  static uint16 counter = 0;

  // save the state as 'atomic' as possible
  //tachometer_disableInterrupt();
  uint32_t now = tachometer_micros;
  uint32_t prevTimeStamp = tachometer_timeStamp;
  uint32_t pulses = tachometer_pulses;
  tachometer_timeStamp = now;
  tachometer_pulses = 0;
  //tachometer_enableInterrupt();

  int32 period =  now - prevTimeStamp;
  bool aBit = GPIO_INPUT_GET(tachometer_pin);
  if (period>0){
    tachometer_sample = (1000000.0*(float)pulses)/(float)period;
    if (counter%3 == 0) {
      // print this every 4:th iteration
      os_printf("Tachometer: pulses: %d period:%d us ", pulses, period);
      os_printf("pinValue:%c tachometer_sample=%d\n",aBit?'1':'0', tachometer_sample);
    }
  }

  counter += 1;
}

/**
 * Sets the 'gpio_pin' pin as a GPIO and sets the interrupt to trigger on that pin
 */
static bool ICACHE_FLASH_ATTR
tachometer_setupInterrupt(uint8 gpio_pin, void (*interruptHandler)(void)) {
  uint8 gpio_func;
  uint32 gpio_name;

  if (gpio_pin == 6 || gpio_pin == 7 || gpio_pin == 8 || gpio_pin == 11 || gpio_pin >= 17) {
    os_printf("tachometer_setupInterrupt Error: There is no GPIO%d, check your code\n", gpio_pin);
    return false;
  }
  if (gpio_pin == 16) {
    os_printf("tachometer_setupInterrupt Error: GPIO16 does not have interrupts\n");
    return false;
  }

  if (gpio_pin == 0) {
    gpio_func = FUNC_GPIO0;
    gpio_name = PERIPHS_IO_MUX_GPIO0_U;
  }
  if (gpio_pin == 1) {
    gpio_func = FUNC_GPIO1;
    gpio_name = PERIPHS_IO_MUX_U0TXD_U;
  }
  if (gpio_pin == 2) {
    gpio_func = FUNC_GPIO2;
    gpio_name = PERIPHS_IO_MUX_GPIO2_U;
  }
  if (gpio_pin == 3) {
    gpio_func = FUNC_GPIO3;
    gpio_name = PERIPHS_IO_MUX_U0RXD_U;
  }
  if (gpio_pin == 4) {
    gpio_func = FUNC_GPIO4;
    gpio_name = PERIPHS_IO_MUX_GPIO4_U;
  }
  if (gpio_pin == 5) {
    gpio_func = FUNC_GPIO5;
    gpio_name = PERIPHS_IO_MUX_GPIO5_U;
  }
  if (gpio_pin == 9) {
    gpio_func = FUNC_GPIO9;
    gpio_name = PERIPHS_IO_MUX_SD_DATA2_U;
  }
  if (gpio_pin == 10) {
    gpio_func = FUNC_GPIO10;
    gpio_name = PERIPHS_IO_MUX_SD_DATA3_U;
  }
  if (gpio_pin == 12) {
    gpio_func = FUNC_GPIO12;
    gpio_name = PERIPHS_IO_MUX_MTDI_U;
  }
  if (gpio_pin == 13) {
    gpio_func = FUNC_GPIO13;
    gpio_name = PERIPHS_IO_MUX_MTCK_U;
  }
  if (gpio_pin == 14) {
    gpio_func = FUNC_GPIO14;
    gpio_name = PERIPHS_IO_MUX_MTMS_U;
  }
  if (gpio_pin == 15) {
    gpio_func = FUNC_GPIO15;
    gpio_name = PERIPHS_IO_MUX_MTDO_U;
  }

  ETS_GPIO_INTR_ATTACH(interruptHandler, NULL);
  ETS_GPIO_INTR_DISABLE();

  PIN_FUNC_SELECT(gpio_name, gpio_func);
  //disable pull downs
  PIN_PULLDWN_DIS(gpio_name);
  //disable pull ups
  PIN_PULLUP_DIS(gpio_name);
  // disable output
  GPIO_DIS_OUTPUT(gpio_pin);

  gpio_register_set(GPIO_PIN_ADDR(gpio_pin), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
                    | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
                    | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));

  //clear gpio14 status
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(gpio_pin));
  ETS_GPIO_INTR_ENABLE();

  return true;
}

void ICACHE_FLASH_ATTR
tachometer_init(uint8_t ioPin) {
  tachometer_pin = ioPin;

  //Disarm timer
  os_timer_disarm(&tachometer_timer);
  os_timer_setfn(&tachometer_timer, (os_timer_func_t *) tachometer_timerFunc, NULL);

  if (tachometer_setupInterrupt(tachometer_pin, tachometer_intr_handler)) {
    // start the poll/sample timer
    os_timer_arm(&tachometer_timer, TACHOMETER_POLL_TIME, 1);
    tachometer_enableInterrupt();
  }
}
