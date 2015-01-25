/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: uart.c
 *
 * Description: Two UART mode configration and interrupt handler.
 *              Check your hardware connection while use this mode.
 *
 *******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
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
}

/******************************************************************************
 * FunctionName : uart1_tx_one_char
 * Description  : Internal used function
 *                Use uart1 interface to transfer one char
 * Parameters   : uint8_t TxChar - character to tx
 * Returns      : OK
 *******************************************************************************/
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

/******************************************************************************
 * FunctionName : uart1_tx_string
 * Description  : Internal used function
 *                Use uart1 interface to transfer a null terminated string
 *                without any filtering
 * Parameters   : uint8_t *buf - string to tx
 * Returns      : OK
 *******************************************************************************/
static void ICACHE_FLASH_ATTR
uart1_tx_string(uint8_t  *buf) {
  for (;*buf;buf++){
    uart1_tx_one_char(*buf);
  }
}

/******************************************************************************
 * FunctionName : digoleserial_lcdCharacter
 * Description  : Internal used function
 *                Do some special deal while tx char is '\r' or '\n'
 * Parameters   : char c - character to tx
 * Returns      : NONE
 *******************************************************************************/
void ICACHE_FLASH_ATTR
digoleserial_lcdCharacter(uint8_t c) {
  if (c == '\n') {
    uart1_tx_one_char(0x0d);
    uart1_tx_string("TRT\n");
    //uart1_tx_one_char(0x0d);
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
/******************************************************************************
 * FunctionName : digoleserial_lcdString
 * Description  : use uart1 to transfer buffer
 * Parameters   : uint8_t *buf - point to send buffer
 *                uint16 len - buffer len
 * Returns      :
 *******************************************************************************/
void ICACHE_FLASH_ATTR
digoleserial_lcdString(uint8_t *buf) {
  digoleserial_lcdNString(buf, strlen(buf));
}

void ICACHE_FLASH_ATTR
digoleserial_enableCursor(bool cursorOn){
  uart1_tx_string("CS");
  uart1_tx_one_char(cursorOn?'1':'0');
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
    UartDev.baut_rate = BIT_RATE_115200; //BIT_RATE_115200;
    digoleserial_uart1_config();
    os_delay_us(100000);
  }
}

/******************************************************************************
 * FunctionName : uart_init
 * Description  : user interface for init uart
 * Parameters   : UartBautRate uart1_br - uart1 bautrate
 * Returns      : NONE
 *******************************************************************************/
void ICACHE_FLASH_ATTR
digoleserial_init(uint8_t col, uint8_t row) {

  UartDev.baut_rate = BIT_RATE_9600;
  digoleserial_uart1_config();
  os_delay_us(100000);
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

  //ETS_UART_INTR_ENABLE();

  // install uart1 putc callback, no don't do that - then os_printf will use uart1 as well.
  //os_install_putc2((void *) uart1_write_char);
}

