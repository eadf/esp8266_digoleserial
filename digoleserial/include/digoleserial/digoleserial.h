#ifndef DIGOLESERIAL_INCLUDE_DIGOLESERIAL_DIGOLESERIAL_H_
#define UART_APP_H

/**
 * clear lcd
 */
void digoleserial_lcdClear(void);

/**
 * set the mode of cursor
 */
void digoleserial_enableCursor(bool cursorOn);

/**
 * set the mode of cursor
 */
void digoleserial_enableBacklight(bool backlightOn);

/**
 * Writes a single character
 */
void digoleserial_lcdCharacter(uint8_t character);

/**
 * moves the cursor to x=col, y=row
 */
void digoleserial_gotoXY(uint8_t x, uint8_t y);

/**
 * set baud to max speed: BIT_RATE_115200
 */
void digoleserial_setBaud(void);

/**
 * Writes an optionally null terminated string
 */
void digoleserial_lcdNString(uint8_t *buf, uint16_t len);

/**
 * Writes a null terminated string
 */
void digoleserial_lcdString(uint8_t *characters);

/**
 * initiates the lcd
 */
void digoleserial_init(uint8_t col, uint8_t row);
#endif

