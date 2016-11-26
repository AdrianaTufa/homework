
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "mpu6050/mpu6050.h"
#include "sdcard/pff.h"

#include "DataTypes.h"
#include "DefaultFonts.h"
#include "UTFT.h"


/* width of a cell of the snake */
#define WIDTH 5

#define LIMIT 0.2

#define BORDER_MIN_X 14
#define BORDER_MIN_Y 54
#define BORDER_MAX_X 310
#define BORDER_MAX_Y 230

#define MOVE_LEFT 1
#define MOVE_RIGHT 2
#define MOVE_UP 3
#define MOVE_DOWN 4

#define START_X 160
#define START_Y 90

#define MAX_LEVEL 10
#define MAX_SNAKE_SIZE 10
#define MAX_FOOD_TIME 100

#define DURATA_NOTA     100

using namespace std;

extern const unsigned char SmallFont[];

typedef struct {
	int x;
	int y;
} pixel;

pixel snake[100];
pixel food;
char dir = MOVE_RIGHT, prev_dir = MOVE_RIGHT;
int size = 1, level = 1, score = 0;
bool eat = false;
bool lose = false;

float frecventa_nota[8] = {
    261.63, 293.66, 329.63, 349.23, 392.0, 440.0, 493.88, 523.25
};

UTFT myGLCD;

FATFS fs;

/* check if a pixel is inside borders */
bool is_inside_borders(pixel p)
{
	if (p.x + WIDTH >= BORDER_MAX_X ||
		p.x - WIDTH <= BORDER_MIN_X ||
		p.y + WIDTH >= BORDER_MAX_Y ||
		p.y - WIDTH <= BORDER_MIN_Y) {

		return false;
	}

	return true;
}

/* move the snake according to direction */
void move_snake()
{
	pixel last_cell;
	last_cell.x = snake[size - 1].x;
	last_cell.y = snake[size - 1].y;

	for (int i = size - 1; i >= 1; i--) {
		snake[i].x = snake[i-1].x;
		snake[i].y = snake[i-1].y;
	}

	/* move the head of the snake according to the direction */
	switch (dir) {
		case MOVE_RIGHT: {
			if (prev_dir == MOVE_LEFT) { //if invalid direction, ignore
				snake[0].x -= WIDTH;
				dir = MOVE_LEFT;
			}
			else
				snake[0].x += WIDTH;

			break;
		}
		case MOVE_LEFT: {
			if (prev_dir == MOVE_RIGHT) { //if invalid direction, ignore
				snake[0].x += WIDTH;
				dir = MOVE_RIGHT;
			}
			else
				snake[0].x -= WIDTH;
			break;
		}
		case MOVE_UP: {
			if (prev_dir == MOVE_DOWN) { //if invalid direction, ignore
				snake[0].y += WIDTH;
				dir = MOVE_DOWN;
			}
			else
				snake[0].y -= WIDTH;
			break;
		}
		case MOVE_DOWN: {
			if (prev_dir == MOVE_UP) { //if invalid direction, ignore
				snake[0].y -= WIDTH;
				dir = MOVE_UP;
			}
			else
				snake[0].y += WIDTH;
			break;
		}
	}

	/* game over if the borders have been touched */
	if (!is_inside_borders(snake[0])) {
		lose = true;
	}

	/* if snake captured food */
	if (snake[0].x == food.x && snake[0].y == food.y) {
		size ++;

		snake[size - 1].x = last_cell.x;
		snake[size - 1].y = last_cell.y;

		eat = true;
		score ++;
	}
}

/* place food on a random position if previous food was eaten or time expired */
void place_food()
{

	myGLCD.setColor(0, 0, 0);
	myGLCD.fillRect(food.x, food.y, food.x + WIDTH, food.y + WIDTH);

    food.x = (BORDER_MIN_X + rand() % (BORDER_MAX_X - BORDER_MIN_X + 1)) / 5 * 5;
    food.y = (BORDER_MIN_Y + rand() % (BORDER_MAX_Y - BORDER_MIN_Y + 1)) / 5 * 5;

    myGLCD.setColor(VGA_PURPLE);

    myGLCD.fillRect(food.x, food.y, food.x + WIDTH, food.y + WIDTH);

}

/* clear previously drawn snake */
void clear_snake()
{
    myGLCD.setColor(0, 0, 0);

    for (int i = 0; i < size; i++) {
        myGLCD.fillRect(snake[i].x, snake[i].y, snake[i].x + WIDTH, snake[i].y + WIDTH);
    }
}


void draw_snake()
{
    myGLCD.setColor(VGA_AQUA);

    // draw head first
    myGLCD.fillRect(snake[0].x, snake[0].y, snake[0].x + WIDTH, snake[0].y + WIDTH);

    for (int i = 1; i < size; i++) {
        // check if snake eats himself
        if(snake[0].x == snake[i].x && snake[0].y == snake[i].y)
            lose = true;
        myGLCD.fillRect(snake[i].x, snake[i].y, snake[i].x + WIDTH, snake[i].y + WIDTH);
    }
}



uint8_t	buf[2][256];	// wave output buffers (double buffering)
const	 uint16_t	buf_size = 256;	// front and back buffer sizes
volatile uint8_t	buf_front = 0;	// front buffer index (current buffer used)
volatile uint8_t	buf_pos = 0;	// current buffer position
volatile uint8_t	buf_sync = 0;

#define BUF_FRONT	(buf[buf_front])
#define BUF_BACK	(buf[1 - buf_front])

#define FCC(c1, c2, c3, c4) \
	(((DWORD)(c4) << 24) + \
	 ((DWORD)(c3) << 16) + \
	 (( WORD)(c2) <<  8) + \
	 (( BYTE)(c1) <<  0))

ISR(TIMER0_COMPA_vect)
{
	OCR1A = BUF_FRONT[buf_pos++];

	// swap buffers when end is reached (end is 256 <=> overflow to 0)
	if(buf_pos == 0)
		buf_front = 1 - buf_front;
}

void timer0_start(void)
{
	// interrupt on compare A
	TIMSK0 |= (1 << OCIE0A);
	// CTC, top OCRA
	TCCR0B |= (0 << WGM02);
	TCCR0A |= (1 << WGM01) | (0 << WGM00);
	// prescaler 8
	TCCR0B |= (2 << CS00);
}

void timer0_stop(void)
{
	TCCR0B = 0;
	TCCR0A = 0;
	TIMSK0 = 0;
	OCR0A = 0;
	TCNT0 = 0;
}

void timer1_start(void)
{
	// 8-bit FastPWM
	TCCR1B |= (0 << WGM13) | (1 << WGM12);
	TCCR1A |= (0 << WGM11) | (1 << WGM10);
	// channel A inverted
	TCCR1A |= (3 << COM1A0);
	// prescaler 1
	TCCR1B |= (1 << CS10);
}

void timer1_stop(void)
{
	TCCR1B = 0;
	TCCR1A = 0;
	OCR1A = 0;
	TCNT1 = 0;
}


DWORD load_header(void)
{
	DWORD size;
	WORD ret;

	// citeste header-ul (12 octeti)
	if(pf_read(BUF_FRONT, 12, &ret))
		return 1;

	if(ret != 12 || LD_DWORD(BUF_FRONT + 8) != FCC('W','A','V','E'))
		return 0;

	for(;;)
	{
		// citeste chunk ID si size
		pf_read(BUF_FRONT, 8, &ret);
		if(ret != 8)
			return 0;

		size = LD_DWORD(&BUF_FRONT[4]);

		// verifica FCC
		switch(LD_DWORD(&BUF_FRONT[0]))
		{
			// 'fmt ' chunk
			case FCC('f','m','t',' '):
				// verifica size
				if(size > 100 || size < 16) return 0;

				// citeste continutul
				pf_read(BUF_FRONT, size, &ret);
				// verifica codificarea
				if(ret != size || BUF_FRONT[0] != 1) return 0;
				// verifica numarul de canale
				if(BUF_FRONT[2] != 1 && BUF_FRONT[2] != 2) return 0;
				// verifica rezolutia
				if(BUF_FRONT[14] != 8 && BUF_FRONT[14] != 16) return 0;

				// seteaza sampling rate-ul
				OCR0A = (BYTE)(F_CPU / 8 / LD_WORD(&BUF_FRONT[4])) - 1;
				break;

			// 'data' chunk => incepe redarea
			case FCC('d','a','t','a'):
				return size;

			// 'LIST' chunk => skip
			case FCC('L','I','S','T'):
			// 'fact' chunk => skip
			case FCC('f','a','c','t'):
				pf_lseek(fs.fptr + size);
				break;

			// chunk necunoscut => eroare
			default:
				return 0;
		}
	}

	return 0;
}

UINT play_song(char *name)
{
	FRESULT ret;

	if((ret = pf_open(name)) == FR_OK)
	{
		WORD bytes_read;

		myGLCD.setColor(VGA_WHITE);
		myGLCD.print("playing song", 40, 220);

		// incarca header-ul fisierului
		DWORD current_size = load_header();
		if(current_size < buf_size)
			return FR_NO_FILE;

		// align to sector boundary
		ret = pf_lseek((fs.fptr + 511) & ~511);
		if(ret != FR_OK)
			return ret;

		// fill front buffer
		ret = pf_read(BUF_FRONT, buf_size, &bytes_read);
		if(ret != FR_OK)
			return ret;
		if(bytes_read < buf_size)
			return ret;

		// reset front buffer index
		buf_pos = 0;

		// start output
		timer0_start();
		timer1_start();
		DDRD |= (1 << PD5);

		while(1)
		{
			uint8_t old_buf_front = buf_front;
			
			// fill back buffer
			ret = pf_read(BUF_BACK, buf_size, &bytes_read);
			if(ret != FR_OK)
				break;
			if(bytes_read < buf_size)
				break;

			// wait for buffer swap
			while(old_buf_front == buf_front);
		}

		// stop output
		DDRD &= ~(1 << PD5);
		timer1_stop();
		timer0_stop();
	}

	return ret;
}

void play( void )
{
	double axg = 0;
    double ayg = 0;
    double azg = 0;
    double gxds, gyds, gzds;
	int i;
	char itmp[10];
	int food_time = 0;


	for (i = 0; i < MAX_LEVEL; i++) {
		
		/* print level */
		myGLCD.setColor(VGA_YELLOW);
    	dtostrf( level, 3, 0, itmp );
    	myGLCD.print(itmp, 200, 30);

    	_delay_ms(300);

    	clear_snake();
		size = 1;
		snake[0].x = START_X;
	  	snake[0].y = START_Y;
	  	food_time = 0;

	  	place_food();

		while(!lose && size != MAX_SNAKE_SIZE) {
			mpu6050_getConvData(&axg, &ayg, &azg, &gxds, &gyds, &gzds);

			clear_snake();

			if(axg >  LIMIT)
	            dir = MOVE_DOWN;
			else if(axg < -LIMIT)
	            dir = MOVE_UP;
			if(ayg >  LIMIT)
	            dir = MOVE_RIGHT;
			else if(ayg < -LIMIT)
	            dir = MOVE_LEFT;

	        move_snake();

	        draw_snake();

	        /* print score */
	        myGLCD.setColor(VGA_YELLOW);
        	dtostrf( score, 3, 0, itmp );
        	myGLCD.print(itmp, 60, 30);

        	//place_food();

        	if (eat || food_time == MAX_FOOD_TIME) {
	        	place_food();
	        	eat = false;
	        	food_time = 0;
        	}

	        prev_dir = dir;

	        food_time++;

			_delay_ms(150);
		}

		if (lose) {
			myGLCD.setColor(255, 0, 0);
			myGLCD.print("GAME OVER! :(", 130, START_Y);
			play_song("gameover.wav");
			break;
		}

		level ++;
	}

	if (!lose) {
		myGLCD.setColor(255, 0, 0);
		myGLCD.print("GOOD GAME, SNAKE MASTER!", 80, START_Y);
		play_song("triumph.wav");
		
	}
}


int main( void )
{
    srand(0);

    mpu6050_init();
    myGLCD.InitLCD();
  	myGLCD.setFont(SmallFont);

    // mount sd card file sistem
    for(;;)
	{
		if(pf_mount(&fs) != FR_OK) // mount filesystem
		{
			_delay_ms(1000); // wait a while and retry
			continue;
		} else break;
	}

	// unsigned int buff[64];
	// WORD nr_bytes;

 //  	FRESULT ret;

	// if((ret = pf_open("logo.raw")) == FR_OK)
	// {
	// 	int y = 0;

	// 	while (y < 64) {
	// 		ret = pf_read(buff, 128, &nr_bytes);
	// 		if(ret != FR_OK) {
	// 			myGLCD.setColor(VGA_GREEN);
	//     		myGLCD.print("not ok", 10, 0);
	// 			break;
	// 		}
	// 		if(nr_bytes < 128) {
	// 			myGLCD.setColor(VGA_GREEN);
	// 	    	myGLCD.print("n-am citit cat tb", 10, 0);
	// 			break;
	// 		}

	// 		memset(buff, 100, 128);

	// 		myGLCD.drawBitmap(0, y, 64, 1, buff);

	// 		y++;
	// 	}

	// 	_delay_ms(5000);
	// }

	sei();


	while (1) {
		myGLCD.clrScr();

	  	myGLCD.fillScr(VGA_BLACK);

	  	myGLCD.setColor(VGA_NAVY);
	  	myGLCD.drawRect(BORDER_MIN_X, BORDER_MIN_Y, BORDER_MAX_X + 1, BORDER_MAX_Y + 1);

	    myGLCD.setColor(VGA_GREEN);

	    myGLCD.print("Accelero-Snake", 10, 0);
	    myGLCD.print("Score: ", 14, 30);
	    myGLCD.print("Level: ", 154, 30);

		play();

		_delay_ms(5000);
		lose = false;
		eat = false;
		level = 1;
		score = 0;
		size = 1;
		dir = MOVE_RIGHT;
		prev_dir = MOVE_RIGHT;
	}
	
	return 0;
 }
