#ifndef UTFT_h
#define UTFT_h

#define UTFT_VERSION	281

#define LEFT 0
#define RIGHT 9999
#define CENTER 9998

#define PORTRAIT 0
#define LANDSCAPE 1

// *** Hardwarespecific defines ***
//#define cbi(reg, bitmask) reg &= ~bitmask
//#define sbi(reg, bitmask) reg |= bitmask

#define cbi(reg, pin) reg &= ~(1<< pin)
#define sbi(reg, pin) reg |=  (1<< pin)

#define pulse_high(reg, bitmask) sbi(reg, bitmask); cbi(reg, bitmask);
#define pulse_low(reg, bitmask)  cbi(reg, bitmask); sbi(reg, bitmask);

#define swap(type, i, j) {type t = i; i = j; j = t;}

#define fontbyte(x) pgm_read_byte(&cfont.font[x])  

#define P_WR PORTB
#define B_WR PB0

#define P_RD PORTB
#define B_RD PB1

#define P_RS PORTD
#define B_RS PD7

#define P_RST PORTD
#define B_RST PD6

#define P_CS PORTD
#define B_CS PD4


//*********************************
// COLORS
//*********************************
// VGA color palette
#define VGA_BLACK		0x0000
#define VGA_WHITE		0xFFFF
#define VGA_RED			0xF800
#define VGA_GREEN		0x0400
#define VGA_BLUE		0x001F
#define VGA_SILVER		0xC618
#define VGA_GRAY		0x8410
#define VGA_MAROON		0x8000
#define VGA_YELLOW		0xFFE0
#define VGA_OLIVE		0x8400
#define VGA_LIME		0x07E0
#define VGA_AQUA		0x07FF
#define VGA_TEAL		0x0410
#define VGA_NAVY		0x0010
#define VGA_FUCHSIA		0xF81F
#define VGA_PURPLE		0x8010
#define VGA_TRANSPARENT	0xFFFFFFFF

#include "DataTypes.h"

struct _current_font
{
	const unsigned char* font;
	uint8_t x_size;
	uint8_t y_size;
	uint8_t offset;
	uint8_t numchars;
};

class UTFT
{
	public:
		UTFT();
		void	InitLCD(byte orientation=LANDSCAPE);
		void	clrScr();
		void	drawPixel(int x, int y);
		void	drawLine(int x1, int y1, int x2, int y2);
		void	fillScr(byte r, byte g, byte b);
		void	fillScr(word color);
		void	drawRect(int x1, int y1, int x2, int y2);
		void	drawRoundRect(int x1, int y1, int x2, int y2);
		void	fillRect(int x1, int y1, int x2, int y2);
		void	fillRoundRect(int x1, int y1, int x2, int y2);
		void	drawCircle(int x, int y, int radius);
		void	fillCircle(int x, int y, int radius);
		void	setColor(byte r, byte g, byte b);
		void	setColor(word color);
		word	getColor();
		void	setBackColor(byte r, byte g, byte b);
		void	setBackColor(uint32_t color);
		word	getBackColor();
		void	print(char *st, int x, int y, int deg=0);
		void	printNumI(long num, int x, int y, int length=0, char filler=' ');
		void	printNumF(double num, byte dec, int x, int y, char divider='.', int length=0, char filler=' ');
		void	setFont(const unsigned char* font);
		const unsigned char* getFont();
		uint8_t	getFontXsize();
		uint8_t	getFontYsize();
		void	drawBitmap(int x, int y, int sx, int sy, bitmapdatatype data, int scale=1);
		void	drawBitmap(int x, int y, int sx, int sy, bitmapdatatype data, int deg, int rox, int roy);
		int		getDisplayXSize();
		int		getDisplayYSize();

		byte			fch, fcl, bch, bcl;
		byte			orient;
		long			disp_x_size, disp_y_size;
		_current_font	cfont;
		boolean			_transparent;

		void LCD_Writ_Bus(char VH,char VL);
		void LCD_Write_COM(char VL);
		void LCD_Write_DATA(char VH,char VL);
		void LCD_Write_DATA(char VL);
		void LCD_Write_COM_DATA(char com1,int dat1);
		void setPixel(word color);
		void drawHLine(int x, int y, int l);
		void drawVLine(int x, int y, int l);
		void printChar(byte c, int x, int y);
		void setXY(word x1, word y1, word x2, word y2);
		void clrXY();
		void rotateChar(byte c, int x, int y, int pos, int deg);
		void _fast_fill_16(int ch, int cl, long pix);
		void _convert_float(char *buf, double num, int width, byte prec);
};

#endif