/*****************************************************************//**
 * @file main_video_test.cpp
 *
 * @brief Basic test of 4 basic i/o cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

//#define _DEBUG
#include "chu_init.h"
#include "gpio_cores.h"
#include "vga_core.h"
#include "sseg_core.h"
#include "ps2_core.h"
#include "xadc_core.h"

#define BLOCK_SIZE 8
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define RIGHT 0
#define DOWN 1
#define LEFT 2
#define UP 3

int snake_color(double voltage) {
   double duty_red, duty_green, duty_blue;

   int i = voltage * 1000;
	   // 0 to 0.167 volt interval
	   if (i <= 170){
	   	   duty_red = 1.0;
	   	   duty_green = i / 170.0; // green light slope
	   	   duty_blue = 0;
	   }
	   // 0.167 to 0.334 volt interval
	   else if (i > 170 && i <= 330){
		   duty_red = (330 - i) / 159.0; // red light slope
		   duty_green = 1.0;
		   duty_blue = 0;
	   }
	   // 0.334 to 0.5 volt interval
	   else if (i > 330 && i <= 500){
		   duty_red = 0;
		   duty_green = 1.0;
		   duty_blue = (i - 331) / 169.0; // blue light slope
	   }
	   // 0.5 to 0.668 volt interval
	   else if (i > 500 && i <= 670){
		   duty_red = 0;
		   duty_green = (670 - i) / 169.0; // green light slope
		   duty_blue = 1;
	   }
	   // 0.668 to 0.835 volt interval
	   else if (i > 670 && i <= 830){
		   duty_red = (i - 671) / 159.0; // red light slope
		   duty_green = 0;
		   duty_blue = 1.0;
	   }
	   // 0.835 to 1 volt interval
	   else {
		   duty_red = 1.0;
		   duty_green = 0;
		   duty_blue = (1000 - i) / 169.0;	// blue light slope
	   }

	   duty_red *= 7;
	   duty_green *= 7;
	   duty_blue *= 7;

	   int result = (int)duty_red + ((int)duty_green << 3) + ((int)duty_blue << 6);

	   return result;
}

void draw_block(FrameCore *frame_p, int x, int y, int color)
{
	for(int i = 0; i < BLOCK_SIZE; i++)
		for(int j = 0; j < BLOCK_SIZE; j++)
			frame_p->wr_pix(x + j, y + i, color);
}

//void clear_screen(FrameCore *frame_p, int (&arr)[60][80])
void clear_snake_screen(FrameCore *frame_p)
{
	for(int i = 0; i < SCREEN_HEIGHT; i+=BLOCK_SIZE)
		for(int j = 0; j < SCREEN_WIDTH; j+=BLOCK_SIZE)
		{
			draw_block(frame_p, j, i, 0);
		}
}

void draw_top_border(FrameCore *frame_p, OsdCore *osd_p, int score) {
	for (int i = 0; i < 80; i+=BLOCK_SIZE)
		for (int j = 0; j < 640; j+=BLOCK_SIZE) {
			draw_block(frame_p, j, i, 511);
		}
	osd_p->set_color(0x0f0, 0x001); // dark gray/green
	osd_p->bypass(0);
	osd_p->clr_screen();
	osd_p->wr_char(50, 3, 83);	// Write 'S'
	osd_p->wr_char(51, 3, 99);	// Write 'c'
	osd_p->wr_char(52, 3, 111);	// Write 'o'
	osd_p->wr_char(53, 3, 114);	// Write 'r'
	osd_p->wr_char(54, 3, 101);	// Write 'e'
	osd_p->wr_char(55, 3, 58);	// Write ':'
	osd_p->wr_char(56, 3, 48);	// Write 'hundred place of score'
	osd_p->wr_char(57, 3, 48);	// Write 'tens place of score'
	osd_p->wr_char(58, 3, 48);	// Write 'ones place of score'
}

void update_score(OsdCore *osd_p, int score) {

	int score_hundreds = score / 100;
	int score_tens = (score / 10) % 10;
	int score_ones = (score % 10);

	osd_p->wr_char(50, 3, 83);	// Write 'S'
	osd_p->wr_char(51, 3, 99);	// Write 'c'
	osd_p->wr_char(52, 3, 111);	// Write 'o'
	osd_p->wr_char(53, 3, 114);	// Write 'r'
	osd_p->wr_char(54, 3, 101);	// Write 'e'
	osd_p->wr_char(55, 3, 58);	// Write ':'
	osd_p->wr_char(56, 3, 48 + score_hundreds );	// Write 'hundred place of score'
	osd_p->wr_char(57, 3, 48 + score_tens);	// Write 'tens place of score'
	osd_p->wr_char(58, 3, 48 + score_ones);	// Write 'ones place of score'
}

void game_over_text(OsdCore *osd_p) {
	osd_p->clr_screen();
	osd_p->wr_char(35, 18, 71);		// Write 'G'
	osd_p->wr_char(36, 18, 97);		// Write 'a'
	osd_p->wr_char(37, 18, 109);	// Write 'm'
	osd_p->wr_char(38, 18, 101);	// Write 'e'
	osd_p->wr_char(40, 18, 79);		// Write 'O'
	osd_p->wr_char(41, 18, 118);	// Write 'v'
	osd_p->wr_char(42, 18, 101);	// Write 'e'
	osd_p->wr_char(43, 18, 114);	// Write 'r'
	osd_p->wr_char(44, 18, 19);		// Write '!'

}


// external core instantiation
GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
FrameCore frame(FRAME_BASE);
GpvCore bar(get_sprite_addr(BRIDGE_BASE, V7_BAR));
GpvCore gray(get_sprite_addr(BRIDGE_BASE, V6_GRAY));
SpriteCore ghost(get_sprite_addr(BRIDGE_BASE, V3_GHOST), 1024);
SpriteCore mouse(get_sprite_addr(BRIDGE_BASE, V1_MOUSE), 1024);
OsdCore osd(get_sprite_addr(BRIDGE_BASE, V2_OSD));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));

struct snake{
	int x;
	int y;
	int color;
	int direction;
};

struct apple{
	int x;
	int y;
	int color;
};

int main() {
	double reading;
	int scolor;
	struct snake snake_body[100];	// initialize snake size to be 100
	snake_body[0].x = 24;			// initialize snake to be of size 3 [0], [1], [2]
	snake_body[0].y = 30;
	snake_body[0].direction = RIGHT;
	snake_body[0].color = 255;
	snake_body[1].x = 23;
	snake_body[1].y = 30;
	snake_body[1].direction = RIGHT;
	snake_body[1].color = 255;
	snake_body[2].x = 22;
	snake_body[2].y = 30;
	snake_body[2].direction = RIGHT;
	snake_body[2].color = 255;
	snake_body[3].x = 21;
	snake_body[3].y = 30;			// tail of snake is black to erase trail
	snake_body[3].direction = RIGHT;
	snake_body[3].color = 0;
	struct apple red_apple;			// initialize apple
	red_apple.x = 50;
	red_apple.y = 40;
	red_apple.color = 256;
	// bypass all cores
	int head_direction = RIGHT;		// initial snake direction = right
	frame.bypass(0);
	bar.bypass(1);
	gray.bypass(1);
	ghost.bypass(1);
	osd.bypass(1);
	mouse.bypass(1);
	int length = 3;
	int score = 0;
	char ch;
	bool game_over = false;
	clear_snake_screen(&frame);
	draw_top_border(&frame, &osd, score);
    while (1) {
    	// while snake doesn't collide with wall of itself
       while(snake_body[0].x < 80 && snake_body[0].x > 0 && snake_body[0].y < 60 && snake_body[0].y > 10 && !game_over)
	   {
    	   reading = adc.read_adc_in(0);	// read adc to determine snake color
    	   scolor = snake_color(reading);
		   for (int i = 0; i < length; i++)	// draw the snake
			   draw_block(&frame, (snake_body[i].x)*BLOCK_SIZE, (snake_body[i].y)*BLOCK_SIZE, scolor);
		   draw_block(&frame, (snake_body[length].x)*BLOCK_SIZE, (snake_body[length].y)*BLOCK_SIZE, 0);
		   draw_block(&frame, (red_apple.x)*BLOCK_SIZE, (red_apple.y)*BLOCK_SIZE, red_apple.color);	// draw apple
		   sleep_ms(50);
		   for (int i = length; i >= 0; i--) {		// update direction of snake
			   if (i == 0){
				   if (snake_body[i].direction == RIGHT) {		// if snake is moving right, increment x direction
					   (snake_body[i].x)++;
				   }
				   else if (snake_body[i].direction == DOWN) {	// if snake is moving down, increment y direction
					   (snake_body[i].y)++;
				   }
				   else if (snake_body[i].direction == LEFT) {	// if snake is moving left, decrement x direction
					   (snake_body[i].x)--;
				   }
				   else if (snake_body[i].direction == UP) {	// if snake is moving up, decrement y direction
					   (snake_body[i].y)--;
				   }
				   else
					   (snake_body[i].x)++;
			   }
			   else {											// shift data of head snake to body snake, 1 by 1
				   snake_body[i].x = snake_body[i - 1].x;
				   snake_body[i].y = snake_body[i - 1].y;
				   snake_body[i].direction = snake_body[i - 1].direction;
			   }
		   }
		   if (ps2.get_kb_ch(&ch)) {	// if a key is pressed
			   if ((int)ch == 52)
			   {
				   if (head_direction != RIGHT)	// if left key is pressed and snake isn't moving right, direction = left
					   head_direction = LEFT;
			   }
			   else if ((int)ch == 50)			// if down key is pressed and snake isn't moving up, direction = up
			   {
				   if (head_direction != UP)
					   head_direction = DOWN;
			   }
			   else if ((int)ch == 54)			// if right key is pressed and snake isn't moving left, direction = right
			   {
				   if (head_direction != LEFT)
					   head_direction = RIGHT;
			   }
			   else if ((int)ch == 56)			// if up key is pressed and snake isn't moving down, direction = up
			   {
				   if (head_direction != DOWN)
					   head_direction = UP;
			   }
			   else if ((int)ch == 27)
			   {
				   while((int)ch != 13)
					   ps2.get_kb_ch(&ch);
			   }
			   else
				   head_direction = head_direction;
		   }
		   snake_body[0].direction = head_direction;	// update direction of head snake
		   if (snake_body[0].x == red_apple.x && snake_body[0].y == red_apple.y) {	// if snake eats apple
			   bool check = true;
			   do																		// make sure new apple doesn't spawn on snake body
			   {
				   check = true;
				   red_apple.x = 25 + rand() % (55 - 25 + 1); // range of x between 55 and 25
				   red_apple.y = 25 + rand() % (55 - 25 + 1); // range of y between 55 and 25
				   for(int i = 0; i < length; i++)
				   {
					   if(snake_body[i].x == red_apple.x || snake_body[i].y == red_apple.y)
						   check = false;
				   }
			   }
			   while(!check);
			   draw_block(&frame, (red_apple.x)*BLOCK_SIZE, (red_apple.y)*BLOCK_SIZE, red_apple.color);
			   length++;												// increment length of snake
			   score++;													// increment score
			   update_score(&osd, score);								// update score text
			   snake_body[length-1].color = snake_body[length-2].color;	// add body to snake
			   snake_body[length].color = 0;							// update new tail to have color black
			   snake_body[length].direction = snake_body[length-1].direction;	// update direction of tail
			   if (snake_body[length-1].direction == UP){				// if snake tail moving up
				   snake_body[length].x = snake_body[length-1].x;		// spawn new tail below old tail
				   snake_body[length].y = snake_body[length-1].y + 1;
			   }
			   else if (snake_body[length-1].direction == RIGHT) {		// if snake tail is moving right
				   snake_body[length].x = snake_body[length-1].x - 1;	// spawn new tail to the left of old tail
				   snake_body[length].y = snake_body[length-1].y;
			   }
			   else if (snake_body[length-1].direction == DOWN){		// if snake it moving down
				   snake_body[length].x = snake_body[length-1].x;		// spawn new tail above old tail
				   snake_body[length].y = snake_body[length-1].y - 1;
			   }
			   else if (snake_body[length-1].direction == LEFT){		// if snake is moving left
				   snake_body[length].x = snake_body[length-1].x + 1;	// spawn new tail to the right of old tail
				   snake_body[length].y = snake_body[length-1].y;
			   }
		   }
		   for (int i = 1; i < length; i++) {		// if the snake collides with itself, game over = true;
			   if (snake_body[0].x == snake_body[i].x && snake_body[0].y == snake_body[i].y)
				   game_over = true;
		   }
	   }
	   clear_snake_screen(&frame);	// clear the screen
	   game_over_text(&osd);		// game over screen
	   sleep_ms(10000);

   } // while
} //main
