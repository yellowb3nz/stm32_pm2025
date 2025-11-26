#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SSD1306_WIDTH  128
#define SSD1306_HEIGHT 64

void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_Update(uint8_t startPage, uint8_t endPage, uint8_t startCol, uint8_t endCol);
void SSD1306_DrawPixel(uint8_t x, uint8_t y, uint8_t color);
void SSD1306_DrawChessBoard(void);

#endif