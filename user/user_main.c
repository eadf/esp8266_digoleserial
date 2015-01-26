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
  //digoleserial_lcdClear();
  bigint_print5Digits(0,(iterations%100)+5200);
}

//Main code function
static void ICACHE_FLASH_ATTR
systemTask(os_event_t *events) {
  static uint32_t iterations = 0;
  if (iterations<2) {
    os_printf("noop\n");
  } else if (iterations==2) {
    digoleserial_init(20,4);
    digoleserial_lcdClear();
    digoleserial_enableCursor(false);
    bigint_init();
  } else if (iterations>=3 && iterations<=7 ) {
    digoleserial_lcdClear();
    digoleserial_gotoXY(0,0);
    digoleserial_lcdString("Digole serial driver");
    digoleserial_gotoXY(0,1);
    digoleserial_lcdString("    for esp8266");
    digoleserial_gotoXY(0,2);
    digoleserial_lcdString("  github.com/eadf/  ");
    digoleserial_gotoXY(0,3);
    digoleserial_lcdString("esp8266_digoleserial");
  } else if (iterations==8){
    digoleserial_lcdClear();
    loop(iterations);
  } else if (iterations>8){
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

  //Set station mode
  wifi_set_opmode( NULL_MODE );

  //Start os task
  system_os_task(systemTask, user_procTaskPrio, user_procTaskQueue,
      user_procTaskQueueLen);

  system_os_post(user_procTaskPrio, 0, 0);
}
