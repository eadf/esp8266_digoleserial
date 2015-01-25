#include "ets_sys.h"
#include "osapi.h"
#include "digoleserial/uart.h"
#include "digoleserial/digoleserial.h"
#include "string.h"

#define UART1   1

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;

static void uart1_tx_string(uint8_t *buf);

/******************************************************************************
 * FunctionName : uart_config
 * Description  : Internal used function
 *                UART1 just used for debug output
 * Parameters   : uart_no, use UART0 or UART1 defined ahead
 * Returns      : NONE
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
digoleserial_uart1_config(void) {
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
  uart_div_modify(UART1, UART_CLK_FREQ / (UartDev.baut_rate));
  WRITE_PERI_REG(UART_CONF0(UART1),
      UartDev.exist_parity | UartDev.parity | (UartDev.stop_bits << UART_STOP_BIT_NUM_S) | (UartDev.data_bits << UART_BIT_NUM_S));
  os_delay_us(100000);
}

static STATUS ICACHE_FLASH_ATTR
uart1_tx_one_char(uint8_t TxChar) {
  while (true) {
    uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(UART1))
        & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);
    if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
      break;
    }
  }

  WRITE_PERI_REG(UART_FIFO(UART1), TxChar);
  return OK;
}

static void ICACHE_FLASH_ATTR
uart1_tx_string(uint8_t  *buf) {
  for (;*buf;buf++){
    uart1_tx_one_char(*buf);
  }
}

void ICACHE_FLASH_ATTR
digoleserial_lcdCharacter(uint8_t c) {
  if (c == '\n') {
    uart1_tx_one_char(0x0d);
    uart1_tx_string("TRT\n");
    uart1_tx_string("TT");
  } else if (c == '\r') {
  } else {
    uart1_tx_one_char(c);
  }
}

void ICACHE_FLASH_ATTR
digoleserial_lcdNString(uint8_t *buf, uint16_t len){
  uint16 i;
  uart1_tx_string("TT");
  for (i = 0; i < len; i++) {
    digoleserial_lcdCharacter(buf[i]);
  }
  uart1_tx_one_char(0x0d);
}

void ICACHE_FLASH_ATTR
digoleserial_gotoXY(uint8_t x, uint8_t y){
  uart1_tx_string("TP");
  uart1_tx_one_char(x);
  uart1_tx_one_char(y);
}

void ICACHE_FLASH_ATTR
digoleserial_lcdString(uint8_t *buf) {
  digoleserial_lcdNString(buf, strlen(buf));
}

void ICACHE_FLASH_ATTR
digoleserial_enableCursor(bool cursorOn){
  uart1_tx_string(cursorOn?"CS1":"CS0");
  uart1_tx_one_char(0x0);
}

void ICACHE_FLASH_ATTR
digoleserial_enableBacklight(bool backlightOn){
  uart1_tx_string(backlightOn?"BL1":"BL0");
  uart1_tx_one_char(0x0);
}

void ICACHE_FLASH_ATTR
digoleserial_lcdClear(void){
  uart1_tx_one_char(0x0);
  uart1_tx_string("CL");
  uart1_tx_one_char(0x0d);
  uart1_tx_one_char(0x0);
}

void ICACHE_FLASH_ATTR
digoleserial_setBaud(void){
  if (true) {
    uart1_tx_one_char(0x00);
    uart1_tx_string("SB115200\n");
    os_delay_us(50000);
    UartDev.baut_rate = 115200; //BIT_RATE_115200;
    digoleserial_uart1_config();
    os_delay_us(100000);
  }
}

void ICACHE_FLASH_ATTR
digoleserial_init(uint8_t col, uint8_t row) {

  UartDev.baut_rate = 9600;
  digoleserial_uart1_config();

  if (col>4 && row>=1) {
    uart1_tx_string("STCR");
    uart1_tx_one_char(col);
    uart1_tx_one_char(row);
    uart1_tx_one_char(0x80);
    uart1_tx_one_char(0xC0);
    uart1_tx_one_char(0x94);
    uart1_tx_one_char(0xD4);
    uart1_tx_one_char(0x0);
    os_delay_us(10000);
  }

  //digoleserial_setBaud();
  uart1_tx_one_char(0x0d);
  os_delay_us(100000);
  uart1_tx_one_char(0x0d);
  digoleserial_lcdClear();
  digoleserial_enableCursor(false);
  os_delay_us(10000);

  digoleserial_lcdClear();
  digoleserial_enableCursor(false);
}

