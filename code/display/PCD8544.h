#define LOW 0
#define HIGH 1

#define BLACK 1
#define WHITE 0

#define LCDWIDTH 84
#define LCDHEIGHT 48

#define PCD8544_POWERDOWN 0x04
#define PCD8544_ENTRYMODE 0x02
#define PCD8544_EXTENDEDINSTRUCTION 0x01

#define PCD8544_DISPLAYBLANK 0x0
#define PCD8544_DISPLAYNORMAL 0x4
#define PCD8544_DISPLAYALLON 0x1
#define PCD8544_DISPLAYINVERTED 0x5

// H = 0
#define PCD8544_FUNCTIONSET 0x20
#define PCD8544_DISPLAYCONTROL 0x08
#define PCD8544_SETYADDR 0x40
#define PCD8544_SETXADDR 0x80

// H = 1
#define PCD8544_SETTEMP 0x04
#define PCD8544_SETBIAS 0x10
#define PCD8544_SETVOP 0x80

#define DIN1  LCD_MOSI
#define SCLK1 LCD_SCLK
#define DC1   LCD_DC
#define RST1  LCD_RST
#define CS1   LCD_CS

// keywords
#define LSBFIRST  0
#define MSBFIRST  1

#define CLKCONST_3  3
#define CLKCONST_2  100  // 400 is a good tested value for Raspberry Pi

static uint8_t cursor_x, cursor_y, textsize, textcolor;
static int8_t _din, _sclk, _dc, _rst, _cs;

uint8_t pcd8544_buffer[LCDWIDTH * LCDHEIGHT / 8] = {0,};

#define _BV(bit) (0x1 << (bit))

#define enablePartialUpdate 1


