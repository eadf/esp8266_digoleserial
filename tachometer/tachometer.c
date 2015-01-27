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

#define tachometer_micros  (0x7FFFFFFF & system_get_time())
#define TACHOMETER_PIN 0
#define TACHOMETER_POLL_TIME 250 // 250ms

static volatile uint32_t   tachometer_timeStamp = 0;
static volatile uint32_t   tachometer_pulses = 0;
static volatile uint32_t   tachometer_sample = 0;
static volatile os_timer_t tachometer_timer;

// forward declarations
static void tachometer_disableInterrupt(void);
static void tachometer_enableInterrupt(void);
static void tachometer_intr_handler(int8_t key);
static void tachometer_timerFunc(void);

static void
tachometer_disableInterrupt(void) {
  gpio_pin_intr_state_set(GPIO_ID_PIN(TACHOMETER_PIN), GPIO_PIN_INTR_DISABLE);
}

static void
tachometer_enableInterrupt(void) {
  gpio_pin_intr_state_set(GPIO_ID_PIN(TACHOMETER_PIN), GPIO_PIN_INTR_POSEDGE);
}

static void
tachometer_intr_handler(int8_t key) {
  uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(0));
  tachometer_pulses+=1;
}

uint32_t ICACHE_FLASH_ATTR
tachometer_getSample(void) {
   return tachometer_sample;
}

void ICACHE_FLASH_ATTR
tachometer_timerFunc(void) {
  static uint16 counter = 0;

  tachometer_disableInterrupt();
  uint32_t now = tachometer_micros;
  int32 period =  now - tachometer_timeStamp;
  uint32_t pulses = tachometer_pulses;
  tachometer_pulses = 0;
  tachometer_timeStamp = now;
  tachometer_enableInterrupt();
  bool aBit = GPIO_INPUT_GET(TACHOMETER_PIN);
  if (period>0){
    tachometer_sample = (1000000.0*(float)pulses)/(float)period;
  }
  if (counter%4 == 0) {
    // print this every 4:th iteration
    os_printf("RPM: pulses: %d period:%d us ", pulses, period);
    os_printf("pinValue:%c tachometer_sample=%d\n",aBit?'1':'0', tachometer_sample);
  }
  counter += 1;
  os_timer_disarm(&tachometer_timer);
  os_timer_arm(&tachometer_timer, TACHOMETER_POLL_TIME, 0);
}

void ICACHE_FLASH_ATTR
tachometer_init(void) {
  //Disarm timer
  os_timer_disarm(&tachometer_timer);
  os_timer_setfn(&tachometer_timer, (os_timer_func_t *) tachometer_timerFunc, NULL);

  //set gpio0 as gpio pin
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  //disable pull downs
  PIN_PULLDWN_DIS(PERIPHS_IO_MUX_GPIO0_U);
  //disable pull ups
  PIN_PULLUP_DIS(PERIPHS_IO_MUX_GPIO0_U);
  // disable output
  GPIO_DIS_OUTPUT(TACHOMETER_PIN);
  ETS_GPIO_INTR_ATTACH(tachometer_intr_handler,0);
  ETS_GPIO_INTR_DISABLE();

  // is this required??
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);
  gpio_output_set(0, 0, 0, GPIO_ID_PIN(TACHOMETER_PIN));


  gpio_register_set(GPIO_PIN_ADDR(0), GPIO_PIN_INT_TYPE_SET(GPIO_PIN_INTR_DISABLE)
      | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_DISABLE)
      | GPIO_PIN_SOURCE_SET(GPIO_AS_PIN_SOURCE));
  //clear gpio status
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(0));
  ETS_GPIO_INTR_ENABLE();
  os_timer_arm(&tachometer_timer, TACHOMETER_POLL_TIME, 0);
}
