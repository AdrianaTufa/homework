#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "DataTypes.h"
#include "UTFT.h"
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/delay_basic.h>

void UTFT::LCD_Writ_Bus(char VH,char VL)
{   
	//_delay_us(10);
	PORTC = VH;
	//_delay_us(10);	
	PORTA = VL;
	//_delay_us(10);	
	pulse_low(P_WR, B_WR);
	//_delay_us(10);	
}

void UTFT::_fast_fill_16(int ch, int cl, long pix)
{
	long blocks;

	PORTC = ch;
	PORTA = cl;

	blocks = pix/16;
	for (int i=0; i<blocks; i++)
	{
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
	}
	if ((pix % 16) != 0)
		for (int i=0; i<(pix % 16)+1; i++)
		{
			pulse_low(P_WR, B_WR);
		}
}

UTFT::UTFT()
{
	disp_x_size = 239;
	disp_y_size = 319;
}


void UTFT::LCD_Write_COM(char VL)  
{   
	//_delay_us(10);
	cbi(P_RS, B_RS);
	//_delay_us(10);	
	LCD_Writ_Bus(0x00,VL);
    //_delay_us(10);	
}

void UTFT::LCD_Write_DATA(char VH,char VL)
{
	//_delay_us(10);
	sbi(P_RS, B_RS);
	//_delay_us(10);	
	LCD_Writ_Bus(VH,VL);
    //_delay_us(10);	
}

void UTFT::LCD_Write_DATA(char VL)
{
	//_delay_us(10);
 	sbi(P_RS, B_RS);
	//_delay_us(10); 	
 	LCD_Writ_Bus(0x00,VL);
    //_delay_us(10);	
}

void UTFT::LCD_Write_COM_DATA(char com1,int dat1)
{
	//_delay_us(10);
    LCD_Write_COM(com1);
	//_delay_us(10);    
    LCD_Write_DATA(dat1>>8,dat1);
    //_delay_us(10);
}

void UTFT::InitLCD(byte orientation)
{
	orient=orientation;

	DDRC = 0xFF;
	DDRA = 0xFF;

	DDRB &= ~(0 << B_RD);
	DDRB |=  (1 << B_WR);
	sbi(P_RD, B_RD);
	sbi(P_WR, B_WR);

	DDRD |= (1 << B_RS);
	DDRD |= (1 << B_RST);
	DDRD |= (1 << B_CS);

	_delay_ms(10);
	sbi(P_RST, B_RST);
	_delay_ms(10);
	cbi(P_RST, B_RST);
	_delay_ms(10);
	sbi(P_RST, B_RST);
	_delay_ms(10);

	cbi( P_CS, B_CS );

	//LCD_Write_COM_DATA(0x00,0x0001);
	//LCD_Write_COM_DATA(0x03,0xA8A4);
	//LCD_Write_COM_DATA(0x0C,0x0000);
	//LCD_Write_COM_DATA(0x0D,0x080C);
	//LCD_Write_COM_DATA(0x0E,0x2B00);
	//LCD_Write_COM_DATA(0x1E,0x00B7);
	//LCD_Write_COM_DATA(0x01,0x2B3F);
	//LCD_Write_COM_DATA(0x02,0x0600);
	//LCD_Write_COM_DATA(0x10,0x0000);
	//LCD_Write_COM_DATA(0x11,0x6070);
	//LCD_Write_COM_DATA(0x05,0x0000);
	//LCD_Write_COM_DATA(0x06,0x0000);
	//LCD_Write_COM_DATA(0x16,0xEF1C);
	//LCD_Write_COM_DATA(0x17,0x0003);
	//LCD_Write_COM_DATA(0x07,0x0233);
	//LCD_Write_COM_DATA(0x0B,0x0000);
	//LCD_Write_COM_DATA(0x0F,0x0000);
	//LCD_Write_COM_DATA(0x41,0x0000);
	//LCD_Write_COM_DATA(0x42,0x0000);
	//LCD_Write_COM_DATA(0x48,0x0000);
	//LCD_Write_COM_DATA(0x49,0x013F);
	//LCD_Write_COM_DATA(0x4A,0x0000);
	//LCD_Write_COM_DATA(0x4B,0x0000);
	//LCD_Write_COM_DATA(0x44,0xEF00);
	//LCD_Write_COM_DATA(0x45,0x0000);
	//LCD_Write_COM_DATA(0x46,0x013F);
	//LCD_Write_COM_DATA(0x30,0x0707);
	//LCD_Write_COM_DATA(0x31,0x0204);
	//LCD_Write_COM_DATA(0x32,0x0204);
	//LCD_Write_COM_DATA(0x33,0x0502);
	//LCD_Write_COM_DATA(0x34,0x0507);
	//LCD_Write_COM_DATA(0x35,0x0204);
	//LCD_Write_COM_DATA(0x36,0x0204);
	//LCD_Write_COM_DATA(0x37,0x0502);
	//LCD_Write_COM_DATA(0x3A,0x0302);
	//LCD_Write_COM_DATA(0x3B,0x0302);
	//LCD_Write_COM_DATA(0x23,0x0000);
	//LCD_Write_COM_DATA(0x24,0x0000);
	//LCD_Write_COM_DATA(0x25,0x8000);
	//LCD_Write_COM_DATA(0x4f,0x0000);
	//LCD_Write_COM_DATA(0x4e,0x0000);
	//LCD_Write_COM(0x22);  
	
	
    LCD_Write_COM(0xCB);  
    LCD_Write_DATA(0x39); 
    LCD_Write_DATA(0x2C); 
    LCD_Write_DATA(0x00); 
    LCD_Write_DATA(0x34); 
    LCD_Write_DATA(0x02); 
 	_delay_ms(1);    

    LCD_Write_COM(0xCF);  
    LCD_Write_DATA(0x00); 
    LCD_Write_DATA(0XC1); 
    LCD_Write_DATA(0X30); 
 	_delay_ms(1);    

    LCD_Write_COM(0xE8);  
    LCD_Write_DATA(0x85); 
    LCD_Write_DATA(0x00); 
    LCD_Write_DATA(0x78); 
 	_delay_ms(1);    

    LCD_Write_COM(0xEA);  
    LCD_Write_DATA(0x00); 
    LCD_Write_DATA(0x00); 
 	_delay_ms(1);    
 
    LCD_Write_COM(0xED);  
    LCD_Write_DATA(0x64); 
    LCD_Write_DATA(0x03); 
    LCD_Write_DATA(0X12); 
    LCD_Write_DATA(0X81); 
 	_delay_ms(1);    

    LCD_Write_COM(0xF7);  
    LCD_Write_DATA(0x20); 
 	_delay_ms(1);    
  
    LCD_Write_COM(0xC0);    //Power control 
    LCD_Write_DATA(0x23);   //VRH[5:0] 
 	_delay_ms(1);
 
    LCD_Write_COM(0xC1);    //Power control 
    LCD_Write_DATA(0x10);   //SAP[2:0];BT[3:0] 
 	_delay_ms(1);

    LCD_Write_COM(0xC5);    //VCM control 
    LCD_Write_DATA(0x3e);   //Contrast
    LCD_Write_DATA(0x28); 
  	_delay_ms(1);
 
    LCD_Write_COM(0xC7);    //VCM control2 
    LCD_Write_DATA(0x86);   //--
  	_delay_ms(1);
 
    LCD_Write_COM(0x36);    // Memory Access Control 
    LCD_Write_DATA(0x48);   
 	_delay_ms(1);

    LCD_Write_COM(0x3A);    
    LCD_Write_DATA(0x55); 
 	_delay_ms(1);

    LCD_Write_COM(0xB1);    
    LCD_Write_DATA(0x00);  
    LCD_Write_DATA(0x18); 
  	_delay_ms(1);
 
    LCD_Write_COM(0xB6);    // Display Function Control 
    LCD_Write_DATA(0x08); 
    LCD_Write_DATA(0x82);
    LCD_Write_DATA(0x27);
 	_delay_ms(1);

	LCD_Write_COM(0x11);    //Exit Sleep 
    _delay_ms(10); 
				
    LCD_Write_COM(0x29);    //Display on 
 	_delay_ms(1);    
    LCD_Write_COM(0x2c); 
 	_delay_ms(1);

	sbi( P_CS, B_CS ); 

	setColor(255, 255, 255);
	setBackColor(0, 0, 0);
	cfont.font=0;
	_transparent = false;
}

void UTFT::setXY(word x1, word y1, word x2, word y2)
{
	if (orient==LANDSCAPE)
	{
		swap(word, x1, y1);
		swap(word, x2, y2)
		y1=disp_y_size-y1;
		y2=disp_y_size-y2;
		swap(word, y1, y2)
	}

	//LCD_Write_COM_DATA(0x44,(x2<<8)+x1);
	//LCD_Write_COM_DATA(0x45,y1);
	//LCD_Write_COM_DATA(0x46,y2);
	//LCD_Write_COM_DATA(0x4e,x1);
	//LCD_Write_COM_DATA(0x4f,y1);
	//LCD_Write_COM(0x22);
	
	LCD_Write_COM(0x2a); 
	LCD_Write_DATA(x1>>8);
	LCD_Write_DATA(x1);
	LCD_Write_DATA(x2>>8);
	LCD_Write_DATA(x2);
	LCD_Write_COM(0x2b); 
	LCD_Write_DATA(y1>>8);
	LCD_Write_DATA(y1);
	LCD_Write_DATA(y2>>8);
	LCD_Write_DATA(y2);
	LCD_Write_COM(0x2c);  
	
}   
    
void UTFT::clrXY()
{   
	if (orient==PORTRAIT)
		setXY(0,0,disp_x_size,disp_y_size);
	else
		setXY(0,0,disp_y_size,disp_x_size);
}   

void UTFT::drawRect(int x1, int y1, int x2, int y2)
{
	if (x1>x2)
	{
		swap(int, x1, x2);
	}
	if (y1>y2)
	{
		swap(int, y1, y2);
	}

	drawHLine(x1, y1, x2-x1);
	drawHLine(x1, y2, x2-x1);
	drawVLine(x1, y1, y2-y1);
	drawVLine(x2, y1, y2-y1);
}

void UTFT::drawRoundRect(int x1, int y1, int x2, int y2)
{
	if (x1>x2)
	{
		swap(int, x1, x2);
	}
	if (y1>y2)
	{
		swap(int, y1, y2);
	}
	if ((x2-x1)>4 && (y2-y1)>4)
	{
		drawPixel(x1+1,y1+1);
		drawPixel(x2-1,y1+1);
		drawPixel(x1+1,y2-1);
		drawPixel(x2-1,y2-1);
		drawHLine(x1+2, y1, x2-x1-4);
		drawHLine(x1+2, y2, x2-x1-4);
		drawVLine(x1, y1+2, y2-y1-4);
		drawVLine(x2, y1+2, y2-y1-4);
	}
}

void UTFT::fillRect(int x1, int y1, int x2, int y2)
{
	if (x1>x2)
	{
		swap(int, x1, x2);
	}
	if (y1>y2)
	{
		swap(int, y1, y2);
	}
	cbi(P_CS, B_CS);
	setXY(x1, y1, x2, y2);
	sbi(P_RS, B_RS);
	_fast_fill_16(fch,fcl,((long(x2-x1)+1)*(long(y2-y1)+1)));
	sbi(P_CS, B_CS);
}

void UTFT::fillRoundRect(int x1, int y1, int x2, int y2)
{
	if (x1>x2)
	{
		swap(int, x1, x2);
	}
	if (y1>y2)
	{
		swap(int, y1, y2);
	}

	if ((x2-x1)>4 && (y2-y1)>4)
	{
		for (int i=0; i<((y2-y1)/2)+1; i++)
		{
			switch(i)
			{
			case 0:
				drawHLine(x1+2, y1+i, x2-x1-4);
				drawHLine(x1+2, y2-i, x2-x1-4);
				break;
			case 1:
				drawHLine(x1+1, y1+i, x2-x1-2);
				drawHLine(x1+1, y2-i, x2-x1-2);
				break;
			default:
				drawHLine(x1, y1+i, x2-x1);
				drawHLine(x1, y2-i, x2-x1);
			}
		}
	}
}

void UTFT::drawCircle(int x, int y, int radius)
{
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x1 = 0;
	int y1 = radius;
 
	cbi(P_CS, B_CS);
	setXY(x, y + radius, x, y + radius);
	LCD_Write_DATA(fch,fcl);
	setXY(x, y - radius, x, y - radius);
	LCD_Write_DATA(fch,fcl);
	setXY(x + radius, y, x + radius, y);
	LCD_Write_DATA(fch,fcl);
	setXY(x - radius, y, x - radius, y);
	LCD_Write_DATA(fch,fcl);
 
	while(x1 < y1)
	{
		if(f >= 0) 
		{
			y1--;
			ddF_y += 2;
			f += ddF_y;
		}
		x1++;
		ddF_x += 2;
		f += ddF_x;    
		setXY(x + x1, y + y1, x + x1, y + y1);
		LCD_Write_DATA(fch,fcl);
		setXY(x - x1, y + y1, x - x1, y + y1);
		LCD_Write_DATA(fch,fcl);
		setXY(x + x1, y - y1, x + x1, y - y1);
		LCD_Write_DATA(fch,fcl);
		setXY(x - x1, y - y1, x - x1, y - y1);
		LCD_Write_DATA(fch,fcl);
		setXY(x + y1, y + x1, x + y1, y + x1);
		LCD_Write_DATA(fch,fcl);
		setXY(x - y1, y + x1, x - y1, y + x1);
		LCD_Write_DATA(fch,fcl);
		setXY(x + y1, y - x1, x + y1, y - x1);
		LCD_Write_DATA(fch,fcl);
		setXY(x - y1, y - x1, x - y1, y - x1);
		LCD_Write_DATA(fch,fcl);
	}
	sbi(P_CS, B_CS);
	clrXY();
}

void UTFT::fillCircle(int x, int y, int radius)
{
	for(int y1=-radius; y1<=0; y1++) 
		for(int x1=-radius; x1<=0; x1++)
			if(x1*x1+y1*y1 <= radius*radius) 
			{
				drawHLine(x+x1, y+y1, 2*(-x1));
				drawHLine(x+x1, y-y1, 2*(-x1));
				break;
			}
}

void UTFT::clrScr()
{
	cbi(P_CS, B_CS);
	clrXY();
	sbi(P_RS, B_RS);
	_fast_fill_16(0,0,((disp_x_size+1)*(disp_y_size+1)));
	sbi(P_CS, B_CS);
}

void UTFT::fillScr(byte r, byte g, byte b)
{
	word color = ((r&248)<<8 | (g&252)<<3 | (b&248)>>3);
	fillScr(color);
}

void UTFT::fillScr(word color)
{
	char ch, cl;
	
	ch=byte(color>>8);
	cl=byte(color & 0xFF);

	cbi(P_CS, B_CS);
	clrXY();
	sbi(P_RS, B_RS);
	_fast_fill_16(ch,cl,((disp_x_size+1)*(disp_y_size+1)));
	sbi(P_CS, B_CS);
}

void UTFT::setColor(byte r, byte g, byte b)
{
	fch=((r&248)|g>>5);
	fcl=((g&28)<<3|b>>3);
}

void UTFT::setColor(word color)
{
	fch=byte(color>>8);
	fcl=byte(color & 0xFF);
}

word UTFT::getColor()
{
	return (fch<<8) | fcl;
}

void UTFT::setBackColor(byte r, byte g, byte b)
{
	bch=((r&248)|g>>5);
	bcl=((g&28)<<3|b>>3);
	_transparent=false;
}

void UTFT::setBackColor(uint32_t color)
{
	if (color==VGA_TRANSPARENT)
		_transparent=true;
	else
	{
		bch=byte(color>>8);
		bcl=byte(color & 0xFF);
		_transparent=false;
	}
}

word UTFT::getBackColor()
{
	return (bch<<8) | bcl;
}

void UTFT::setPixel(word color)
{
	LCD_Write_DATA((color>>8),(color&0xFF));	// rrrrrggggggbbbbb
}

void UTFT::drawPixel(int x, int y)
{
	cbi(P_CS, B_CS);
	setXY(x, y, x, y);
	setPixel((fch<<8)|fcl);
	sbi(P_CS, B_CS);
	clrXY();
}

void UTFT::drawLine(int x1, int y1, int x2, int y2)
{
	if (y1==y2)
		drawHLine(x1, y1, x2-x1);
	else if (x1==x2)
		drawVLine(x1, y1, y2-y1);
	else
	{
		unsigned int	dx = (x2 > x1 ? x2 - x1 : x1 - x2);
		short			xstep =  x2 > x1 ? 1 : -1;
		unsigned int	dy = (y2 > y1 ? y2 - y1 : y1 - y2);
		short			ystep =  y2 > y1 ? 1 : -1;
		int				col = x1, row = y1;

		cbi(P_CS, B_CS);
		if (dx < dy)
		{
			int t = - (dy >> 1);
			while (true)
			{
				setXY (col, row, col, row);
				LCD_Write_DATA (fch, fcl);
				if (row == y2)
					return;
				row += ystep;
				t += dx;
				if (t >= 0)
				{
					col += xstep;
					t   -= dy;
				}
			} 
		}
		else
		{
			int t = - (dx >> 1);
			while (true)
			{
				setXY (col, row, col, row);
				LCD_Write_DATA (fch, fcl);
				if (col == x2)
					return;
				col += xstep;
				t += dy;
				if (t >= 0)
				{
					row += ystep;
					t   -= dx;
				}
			} 
		}
		sbi(P_CS, B_CS);
	}
	clrXY();
}

void UTFT::drawHLine(int x, int y, int l)
{
	if (l<0)
	{
		l = -l;
		x -= l;
	}
	cbi(P_CS, B_CS);
	setXY(x, y, x+l, y);
	sbi(P_RS, B_RS);
	_fast_fill_16(fch,fcl,l);
	sbi(P_CS, B_CS);
	clrXY();
}

void UTFT::drawVLine(int x, int y, int l)
{
	if (l<0)
	{
		l = -l;
		y -= l;
	}
	cbi(P_CS, B_CS);
	setXY(x, y, x, y+l);
	sbi(P_RS, B_RS);
	_fast_fill_16(fch,fcl,l);
	sbi(P_CS, B_CS);
	clrXY();
}

void UTFT::printChar(byte c, int x, int y)
{
	byte i,ch;
	word j;
	word temp; 

	cbi(P_CS, B_CS);
  
	if (!_transparent)
	{
		if (orient==PORTRAIT)
		{
			setXY(x,y,x+cfont.x_size-1,y+cfont.y_size-1);
	  
			temp=((c-cfont.offset)*((cfont.x_size>>3)*cfont.y_size))+4;
			for(j=0;j<((cfont.x_size>>3)*cfont.y_size);j++)
			{
				ch=pgm_read_byte(&cfont.font[temp]);
				for(i=0;i<8;i++)
				{   
					if((ch&(1<<(7-i)))!=0)   
					{
						setPixel((fch<<8)|fcl);
					} 
					else
					{
						setPixel((bch<<8)|bcl);
					}   
				}
				temp++;
			}
		}
		else
		{
			temp=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4;

			for(j=0;j<((cfont.x_size/8)*cfont.y_size);j+=(cfont.x_size/8))
			{
				setXY(x,y+(j/(cfont.x_size/8)),x+cfont.x_size-1,y+(j/(cfont.x_size/8)));
				for (int zz=(cfont.x_size/8)-1; zz>=0; zz--)
				{
					ch=pgm_read_byte(&cfont.font[temp+zz]);
					for(i=0;i<8;i++)
					{   
						if((ch&(1<<i))!=0)   
						{
							setPixel((fch<<8)|fcl);
						} 
						else
						{
							setPixel((bch<<8)|bcl);
						}   
					}
				}
				temp+=(cfont.x_size/8);
			}
		}
	}
	else
	{
		temp=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4;
		for(j=0;j<cfont.y_size;j++) 
		{
			for (int zz=0; zz<(cfont.x_size/8); zz++)
			{
				ch=pgm_read_byte(&cfont.font[temp+zz]); 
				for(i=0;i<8;i++)
				{   
				
					if((ch&(1<<(7-i)))!=0)   
					{
						setXY(x+i+(zz*8),y+j,x+i+(zz*8)+1,y+j+1);
						setPixel((fch<<8)|fcl);
					} 
				}
			}
			temp+=(cfont.x_size/8);
		}
	}

	sbi(P_CS, B_CS);
	clrXY();
}

void UTFT::rotateChar(byte c, int x, int y, int pos, int deg)
{
	byte i,j,ch;
	word temp; 
	int newx,newy;
	double radian;
	radian=deg*0.0175;  

	cbi(P_CS, B_CS);

	temp=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4;
	for(j=0;j<cfont.y_size;j++) 
	{
		for (int zz=0; zz<(cfont.x_size/8); zz++)
		{
			ch=pgm_read_byte(&cfont.font[temp+zz]); 
			for(i=0;i<8;i++)
			{   
				newx=x+(((i+(zz*8)+(pos*cfont.x_size))*cos(radian))-((j)*sin(radian)));
				newy=y+(((j)*cos(radian))+((i+(zz*8)+(pos*cfont.x_size))*sin(radian)));

				setXY(newx,newy,newx+1,newy+1);
				
				if((ch&(1<<(7-i)))!=0)   
				{
					setPixel((fch<<8)|fcl);
				} 
				else  
				{
					if (!_transparent)
						setPixel((bch<<8)|bcl);
				}   
			}
		}
		temp+=(cfont.x_size/8);
	}
	sbi(P_CS, B_CS);
	clrXY();
}

void UTFT::print(char *st, int x, int y, int deg)
{
	int stl, i;

	stl = strlen(st);

	if (orient==PORTRAIT)
	{
	if (x==RIGHT)
		x=(disp_x_size+1)-(stl*cfont.x_size);
	if (x==CENTER)
		x=((disp_x_size+1)-(stl*cfont.x_size))/2;
	}
	else
	{
	if (x==RIGHT)
		x=(disp_y_size+1)-(stl*cfont.x_size);
	if (x==CENTER)
		x=((disp_y_size+1)-(stl*cfont.x_size))/2;
	}

	for (i=0; i<stl; i++)
		if (deg==0)
			printChar(*st++, x + (i*(cfont.x_size)), y);
		else
			rotateChar(*st++, x, y, i, deg);
}

void UTFT::printNumI(long num, int x, int y, int length, char filler)
{
	char buf[25];
	char st[27];
	boolean neg=false;
	int c=0, f=0;
  
	if (num==0)
	{
		if (length!=0)
		{
			for (c=0; c<(length-1); c++)
				st[c]=filler;
			st[c]=48;
			st[c+1]=0;
		}
		else
		{
			st[0]=48;
			st[1]=0;
		}
	}
	else
	{
		if (num<0)
		{
			neg=true;
			num=-num;
		}
	  
		while (num>0)
		{
			buf[c]=48+(num % 10);
			c++;
			num=(num-(num % 10))/10;
		}
		buf[c]=0;
	  
		if (neg)
		{
			st[0]=45;
		}
	  
		if (length>(c+neg))
		{
			for (int i=0; i<(length-c-neg); i++)
			{
				st[i+neg]=filler;
				f++;
			}
		}

		for (int i=0; i<c; i++)
		{
			st[i+neg+f]=buf[c-i-1];
		}
		st[c+neg+f]=0;

	}

	print(st,x,y);
}

void UTFT::printNumF(double num, byte dec, int x, int y, char divider, int length, char filler)
{
	char st[27];
	boolean neg=false;

	if (dec<1)
		dec=1;
	else if (dec>5)
		dec=5;

	if (num<0)
		neg = true;

	_convert_float(st, num, length, dec);

	if (divider != '.')
	{
		for( unsigned int i = 0; i<sizeof(st); i++ )
			if (st[i]=='.')
				st[i]=divider;
	}

	if (filler != ' ')
	{
		if (neg)
		{
			st[0]='-';
			for( unsigned int i = 1; i<sizeof(st); i++ )
				if ((st[i]==' ') || (st[i]=='-'))
					st[i]=filler;
		}
		else
		{
			for( unsigned int i = 0; i<sizeof(st); i++)
				if (st[i]==' ')
					st[i]=filler;
		}
	}

	print(st,x,y);
}

void UTFT::setFont(const unsigned char* font)
{
	cfont.font=font;
	cfont.x_size=fontbyte(0);
	cfont.y_size=fontbyte(1);
	cfont.offset=fontbyte(2);
	cfont.numchars=fontbyte(3);
}

const unsigned char* UTFT::getFont()
{
	return cfont.font;
}

uint8_t UTFT::getFontXsize()
{
	return cfont.x_size;
}

uint8_t UTFT::getFontYsize()
{
	return cfont.y_size;
}

void UTFT::drawBitmap(int x, int y, int sx, int sy, bitmapdatatype data, int scale)
{
	unsigned int col;
	int tx, ty, tc, tsx, tsy;

	if (scale==1)
	{
		if (orient==PORTRAIT)
		{
			cbi(P_CS, B_CS);
			setXY(x, y, x+sx-1, y+sy-1);
			for (tc=0; tc<(sx*sy); tc++)
			{
				col=pgm_read_word(&data[tc]);
				LCD_Write_DATA(col>>8,col & 0xff);
			}
			sbi(P_CS, B_CS);
		}
		else
		{
			cbi(P_CS, B_CS);
			for (ty=0; ty<sy; ty++)
			{
				setXY(x, y+ty, x+sx-1, y+ty);
				for (tx=sx-1; tx>=0; tx--)
				{
					col=pgm_read_word(&data[(ty*sx)+tx]);
					LCD_Write_DATA(col>>8,col & 0xff);
				}
			}
			sbi(P_CS, B_CS);
		}
	}
	else
	{
		if (orient==PORTRAIT)
		{
			cbi(P_CS, B_CS);
			for (ty=0; ty<sy; ty++)
			{
				setXY(x, y+(ty*scale), x+((sx*scale)-1), y+(ty*scale)+scale);
				for (tsy=0; tsy<scale; tsy++)
					for (tx=0; tx<sx; tx++)
					{
						col=pgm_read_word(&data[(ty*sx)+tx]);
						for (tsx=0; tsx<scale; tsx++)
							LCD_Write_DATA(col>>8,col & 0xff);
					}
			}
			sbi(P_CS, B_CS);
		}
		else
		{
			cbi(P_CS, B_CS);
			for (ty=0; ty<sy; ty++)
			{
				for (tsy=0; tsy<scale; tsy++)
				{
					setXY(x, y+(ty*scale)+tsy, x+((sx*scale)-1), y+(ty*scale)+tsy);
					for (tx=sx-1; tx>=0; tx--)
					{
						col=pgm_read_word(&data[(ty*sx)+tx]);
						for (tsx=0; tsx<scale; tsx++)
							LCD_Write_DATA(col>>8,col & 0xff);
					}
				}
			}
			sbi(P_CS, B_CS);
		}
	}
	clrXY();
}

void UTFT::drawBitmap(int x, int y, int sx, int sy, bitmapdatatype data, int deg, int rox, int roy)
{
	unsigned int col;
	int tx, ty, newx, newy;
	double radian;
	radian=deg*0.0175;  

	if (deg==0)
		drawBitmap(x, y, sx, sy, data);
	else
	{
		cbi(P_CS, B_CS);
		for (ty=0; ty<sy; ty++)
			for (tx=0; tx<sx; tx++)
			{
				col=pgm_read_word(&data[(ty*sx)+tx]);

				newx=x+rox+(((tx-rox)*cos(radian))-((ty-roy)*sin(radian)));
				newy=y+roy+(((ty-roy)*cos(radian))+((tx-rox)*sin(radian)));

				setXY(newx, newy, newx, newy);
				LCD_Write_DATA(col>>8,col & 0xff);
			}
		sbi(P_CS, B_CS);
	}
	clrXY();
}



int UTFT::getDisplayXSize()
{
	if (orient==PORTRAIT)
		return disp_x_size+1;
	else
		return disp_y_size+1;
}

int UTFT::getDisplayYSize()
{
	if (orient==PORTRAIT)
		return disp_y_size+1;
	else
		return disp_x_size+1;
}


void UTFT::_convert_float(char *buf, double num, int width, byte prec)
{
	dtostrf(num, width, prec, buf);
}