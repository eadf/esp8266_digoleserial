#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "digoleserial/digoleserial.h"
#include "driver/stdout.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);
void user_init(void);

static char buffer[] = "                           ";

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events) {
  static int counter = 0;
  os_sprintf(buffer, "Hello uart0 %d\n", counter&3);
  os_printf(buffer);
  os_sprintf(buffer, "Hello uart1 %d   \n", counter);
  if (counter%4==0) digoleserial_lcdClear();
  digoleserial_gotoXY(0,counter&3);
  digoleserial_lcdNString(buffer,15);
  counter += 1;
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
  system_os_task(loop, user_procTaskPrio, user_procTaskQueue,
      user_procTaskQueueLen);

  system_os_post(user_procTaskPrio, 0, 0);
}
