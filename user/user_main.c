#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "digoleserial/digoleserial.h"
#include "bigint.h"
#include "driver/stdout.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t user_procTaskQueue[user_procTaskQueueLen];
static void systemTask(os_event_t *events);
void user_init(void);
void loop(uint32_t iterations);


static char buffer[] = "                           ";

void ICACHE_FLASH_ATTR
loop(uint32_t iterations) {
  //os_sprintf(buffer, "Hello uart0 %d\n", iterations&3);
  //os_printf(buffer);
  //os_sprintf(buffer, "Hello ua\nrt1 %d   \n", iterations);
  //if (iterations%3==0) {
  digoleserial_lcdClear();
  bigint_print5Digits(1,(iterations%100)+5200);
  os_printf("Hello uart0 %d\n", iterations%100);
  os_sprintf(buffer, "Uart1 %d", iterations%100);
  //digoleserial_gotoXY(0,3);
  //digoleserial_lcdNString(buffer,13);
  //}

  //digoleserial_gotoXY(0,iterations&3);
  //digoleserial_lcdNString(buffer,15);
  //digoleserial_enableBacklight(iterations&2);
}

//Main code function
static void ICACHE_FLASH_ATTR
systemTask(os_event_t *events) {
  static uint32_t iterations = 0;
  if (iterations==0) {
    digoleserial_lcdClear();
    digoleserial_enableCursor(false);
    bigint_init();
    //digoleserial_setBaud();
  } else {
    loop(iterations);
  }
  iterations += 1;
  os_delay_us(1500000);
  system_os_post(user_procTaskPrio, 0, 0);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init(void) {
  stdoutInit();
  digoleserial_init(20,4);

  //Set station mode
  wifi_set_opmode( NULL_MODE );

  //Start os task
  system_os_task(systemTask, user_procTaskPrio, user_procTaskQueue,
      user_procTaskQueueLen);

  system_os_post(user_procTaskPrio, 0, 0);
}
