#include <xinu.h>
#include "titlescreen.h"
#include "playerImage.h"
#include "enemy.h"
#include "boss.h"
#include "gameover.h"
#include "shoot.h"

extern unsigned char tecla_actual;
typedef unsigned short u16;
#define RGB(r, g, b) (r | (g << 5) | (b << 10))
// #define REG_DISPCNT *(u16 *)0x4000000
#define extern videoBuffer
#define MODE3 3
#define BG2_ENABLE (1 << 10)
#define WHITE RGB(31, 31, 31)
#define BLACK RGB(0, 0, 0)

/*
#define BUTTON_A		(1<<0)
#define BUTTON_B		(1<<1)
#define BUTTON_SELECT	(1<<2)
#define BUTTON_START	(1<<3)
#define BUTTON_RIGHT	(1<<4)
#define BUTTON_LEFT		(1<<5)
#define BUTTON_UP		(1<<6)
#define BUTTON_DOWN		(1<<7)
#define BUTTON_R		(1<<8)
#define BUTTON_L		(1<<9)
#define KEY_DOWN_NOW(key)  (~(BUTTONS) & key)
*/
//#define BUTTONS *(volatile unsigned int *)0x4000130

#define BUTTON_A		0x24
#define BUTTON_B		0x25 
#define BUTTON_SELECT	0x03
#define BUTTON_START	0x2c
#define BUTTON_RIGHT	0x1f
#define BUTTON_LEFT		0x1e	
#define BUTTON_UP	'w'
#define BUTTON_DOWN 's'	
#define BUTTON_R	'1'
#define BUTTON_L	'2'
#define BUTTON_ESC		0x01
#define KEY_DOWN_NOW(key)  (tecla_actual == key)

//variable definitions
#define playerspeed 2
#define enemyspeed 1
#define fastXSpeed 3
#define fastYSpeed 2


void setPixel(int x, int y, u16 color);
void drawRect(int x, int y, int width, int height, u16 color);
void drawHollowRect(int x, int y, int width, int height, u16 color);
void drawHollowRectSize(int x, int y, int width, int height, u16 color, int size);
void drawImage3(int x, int y, int width, int height, const u16* image);
void delay_galaga();
void waitForVBlank();

//helpers
void initialize();
void drawEnemies();
void endGame();

struct obj_data {
	const u16* image;
	uint8 w, h, speed;
};

enum ObjectState { INACTIVE, ACTIVE }; 

enum ObjectType  { PLAYER, ENEMY_EASY, ENEMY_HARD, SHOOT } ;
struct obj_data obj_sheet[] = {  
	{ playerImage, PLAYER_WIDTH,	PLAYER_HEIGHT,	playerspeed },  
  	{ enemyImage,  ENEMY_WIDTH, 	ENEMY_HEIGHT,	enemyspeed  },
	{ enemyImage,  ENEMY_WIDTH, 	ENEMY_HEIGHT,	enemyspeed  },
	{ shootImage,  SHOOT_WIDTH,		SHOOT_HEIGHT,   2 }
};

//objects
struct Object_t {
	uint8 x, y, type, state;
};

struct Players {
	volatile u16 playerX;
	volatile u16 playerY;
};
struct Enemy {
	volatile u16 enemyX;
	volatile u16 enemyY;
};
struct FastEnemy {
	volatile u16 fastX;
	volatile u16 fastY;
};

#define N_SHOOTS 10
int shoots[N_SHOOTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

pid32 pid_game;
pid32 pid_score;
pid32 pid_control;

int score;
int lives;
char buffer[32];

void score_up() 
{
	score++;
	send(pid_score, 0);
}

void input(struct Object_t* objects, uint32 player_index, uint32 shoots_index, uint32* curr_shoot)
{
	struct Object_t* player = &objects[player_index];

	if (KEY_DOWN_NOW(BUTTON_LEFT) && (player->x > 0)) {
		player->x -= playerspeed;
	}
	if (KEY_DOWN_NOW(BUTTON_RIGHT) && (player->x <= 216)) {
		player->x += playerspeed;
	}
	if (KEY_DOWN_NOW(BUTTON_UP) && (player->y > 25)) {
		player->y -= playerspeed;
	}
	if (KEY_DOWN_NOW(BUTTON_DOWN) && (player->y <= 136)) {
		player->y += playerspeed;
	}
	if (KEY_DOWN_NOW(BUTTON_A)) {
		struct Object_t* shoot = &objects[shoots_index+*curr_shoot]; 
		if (shoot->state == INACTIVE) {
			shoot->state = ACTIVE;
			shoot->x = player->x+9; /* 24 widht player */
			shoot->y = player->y; 
			(*curr_shoot)++;
			if (*curr_shoot >= N_SHOOTS)
				*curr_shoot = 0;
		};
	}
}

void drawObjects(struct Object_t* obj, uint32 size)
{
	for(int i=0; i<size; i++)
	{
		struct obj_data data = obj_sheet[obj[i].type]; 
		drawImage3(obj[i].x, obj[i].y, data.w, data.h, obj_sheet[obj[i].type].image);
		drawHollowRectSize(obj[i].x, obj[i].y, data.w, data.h, BLACK, data.speed);
	}	
}

#define spawn(x, y, type, state) obj[index++] = (struct Object_t){x, y, type, state};  
void initializeObjects(struct Object_t* obj, uint32 size)
{
	uint32 index = 0;
	spawn(120, 136, PLAYER, ACTIVE);  
	for(int i=0; i<9; i++)
		spawn(28*i, 40, ENEMY_EASY, ACTIVE);
	for(int i=0; i<10; i++)
		spawn(0, 0, SHOOT, INACTIVE);
}

int collision(struct Object_t obj1, struct Object_t obj2)
{
	struct obj_data data1 = obj_sheet[obj1.type];
	struct obj_data data2 = obj_sheet[obj2.type]; 

	if( obj1.x < obj2.x + data2.w && obj1.x + data1.w > obj2.x &&
		obj1.y < obj2.y + data2.h && obj1.y + data1.h > obj2.y )
		return 1;

	return 0;
}

void update(struct Object_t* obj, uint32 player_index, uint32 shoots_index, uint32 size)
{ 
	for(int i=player_index+1; i<shoots_index; i++) 
	{
		struct obj_data data = obj_sheet[obj[i].type]; 
		obj[i].y += data.speed;
		if(obj[i].y > 160)
			obj[i].y = 0; 
		if(collision(obj[i], obj[player_index]))
			endGame();
	}	

	for(int i=shoots_index; i<size; i++)
	{
		if(obj[i].state==INACTIVE)
			continue;
		
		struct obj_data data = obj_sheet[obj[i].type];
		obj[i].y -= data.speed;
		if(obj[i].y < 0)
			obj[i].state = INACTIVE;  
	}	
}


int galaga_game() {

	uint32 curr_shoot = 0;
	score = 0;
	lives = 3;

	struct Object_t objects[20]; 
	initializeObjects(objects, sizeof(objects)/sizeof(struct Object_t));
	
	//easy enemy wave set setup
	struct Enemy easyEnemies[9];
	for (int a = 0; a < 9; a++) {
		easyEnemies[a].enemyX = (28*a);
		easyEnemies[a].enemyY = 200;
	} 

	//difficult enemies setup
	struct Enemy hardEnemies[9];
	for (int a = 0; a < 9; a++) {
		hardEnemies[a].enemyX = (28*a);
		hardEnemies[a].enemyY = 160;
	} 
	hardEnemies[3].enemyX = 240;
	hardEnemies[6].enemyX = 240;
	//player setup
	struct Players player;
	player.playerX = 120;
	player.playerY = 136;
	//fast enemy "boss" setup
	struct FastEnemy fast;
	fast.fastX = 0;
	fast.fastY = 30;

	// REG_DISPCNT = MODE3 | BG2_ENABLE;
	//initalize title screen
	print_text_on_vga(10, 20, "GALAGA ");
	drawImage3(0, 0, 240, 160, titlescreen);

	while(1) {
		if (KEY_DOWN_NOW(BUTTON_START)) {
			break;
		}
		if (KEY_DOWN_NOW(BUTTON_ESC)){
			send(pid_control, BUTTON_ESC);
		} 
	}	
	//start black screen for drawing
	for (int i = 0; i < 240; i++) {
		for (int j = 0; j < 160; j++) {
			setPixel(i, j, BLACK);
		}
	}	
	while(1) {
		//go back to title screen if select button is pressed
		if (KEY_DOWN_NOW(BUTTON_SELECT)) {
			//initialize();
			galaga_game();
		}

		input(objects, 0, 10, &curr_shoot);
		update(objects, 0, 10, sizeof(objects)/sizeof(struct Object_t));

		//player shots 
	//	if (KEY_DOWN_NOW(BUTTON_A)) {
	//		if (shoots[curr_shot] == 0) {
	//			shoots[curr_shot] = 136*240 + player.playerX+9; /* 24 widht player */
	//			curr_shot++;
	//			if (curr_shot >= N_SHOOTS)
	//				curr_shot = 0;
	//		};
	//	}
		//player movement input
		//if (KEY_DOWN_NOW(BUTTON_LEFT) && (player.playerX > 0)) {
		//	player.playerX -= playerspeed;
		//}
		//if (KEY_DOWN_NOW(BUTTON_RIGHT) && (player.playerX <= 216)) {
		//	player.playerX += playerspeed;
		//}
		//if (KEY_DOWN_NOW(BUTTON_UP) && (player.playerY > 25)) {
		//	player.playerY -= playerspeed;
		//}
		//if (KEY_DOWN_NOW(BUTTON_DOWN) && (player.playerY <= 136)) {
		//	player.playerY += playerspeed;
		//}
		waitForVBlank();
		sleepms(50);
		//draw player
	//	drawImage3(player.playerX, player.playerY, 24, 24, playerImage);
	//	drawHollowRect(player.playerX - 1, player.playerY - 1, 26, 26, BLACK);
	//	drawHollowRect(player.playerX - 2, player.playerY - 2, 28, 28, BLACK);
		//draw easy enemies with downward movement
	//	for (int a = 0; a < 9; a++) {
	//		easyEnemies[a].enemyY += enemyspeed;
	//	//	drawImage3(easyEnemies[a].enemyX, easyEnemies[a].enemyY, 20, 20, enemyImage);
	//		if (collision(easyEnemies[a].enemyX, easyEnemies[a].enemyY, 20, 20, player.playerX, player.playerY)) {
	//			endGame();
	//		}	
	//		if (easyEnemies[a].enemyY >= 160) {
	//			easyEnemies[a].enemyY = 0;
	//		}		
	//	}

		//draw shots
	//	for (int i = 0; i < N_SHOOTS; i++) {
	//		if (shoots[i] != 0) {
	//			drawRect((shoots[i] % 240), (shoots[i] / 240)+4, 5, 5, BLACK);
	//			drawImage3((shoots[i] % 240), (shoots[i] / 240), 5, 5, shootImage);
	//			shoots[i] = shoots[i]-240*4;
	//			if (shoots[i] <=0)   shoots[i]=0;
	//		}

			// check hits of shoots
	//		for (int j = 0; j < 9; j++) {
	//			if (collision(easyEnemies[j].enemyX, easyEnemies[j].enemyY, 15, 15, shoots[i] % 240, shoots[i] / 240)) {
	//				drawRect(easyEnemies[j].enemyX, easyEnemies[j].enemyY,  20, 20, BLACK);
	//				drawRect((shoots[i] % 240), (shoots[i] / 240)+4, 5, 5, BLACK);
	//				easyEnemies[j].enemyY = 0;
	//				shoots[i] = 0;
	//				score_up();
	//			}
	//		}
	//	}
		
		//draw hard enemies
		for (int a = 0; a < 9; a++) {
			hardEnemies[a].enemyY += enemyspeed;
			drawImage3(hardEnemies[a].enemyX, hardEnemies[a].enemyY, 20, 20, enemyImage);
			drawRect(hardEnemies[a].enemyX, hardEnemies[a].enemyY, 20, 20, RGB(31, 0, 31));
	//		if (collision(hardEnemies[a].enemyX, hardEnemies[a].enemyY, 20, 20, player.playerX, player.playerY)) {
	//			endGame();
	//		}	
			if (hardEnemies[a].enemyY >= 228) {
				hardEnemies[a].enemyY = 0;
			}
			if ((hardEnemies[a].enemyY >= 200) && (easyEnemies[a].enemyY <=45)) {
				hardEnemies[a].enemyY = 160;
			}	
			//space enemies apart
			if ((hardEnemies[a].enemyY >= 200) && (easyEnemies[a].enemyY <=45)) {
				hardEnemies[a].enemyY = 160;
			}		
			if ((easyEnemies[a].enemyY >= 120) && (hardEnemies[a].enemyY >=170)) {
				hardEnemies[a].enemyY = 160;
			}							
		}	
		//draw fast enemy
		drawImage3(fast.fastX, fast.fastY, 15, 15, boss);
		drawHollowRect(fast.fastX - 1, fast.fastY - 1, 17, 17, BLACK);
		drawHollowRect(fast.fastX - 2, fast.fastY - 2, 19, 19, BLACK);
	//	if(collision(fast.fastX, fast.fastY, 15, 15, player.playerX, player.playerY)) {
	//		//endGame();
	//	}		
//RAFA		fast.fastX += fastXSpeed;
//RAFA		fast.fastY += fastYSpeed;
		if (fast.fastX >= 240) {
			fast.fastX = 0;
		}
		if (fast.fastY >= 200) {
			fast.fastY = player.playerY - 20;
		}

		drawObjects(objects, sizeof(objects)/sizeof(struct Object_t));
	}
	return 0;
}

//int collision(u16 enemyX, u16 enemyY, u16 enemyWidth, u16 enemyHeight, u16 playerX, u16 playerY) {
//	//check if bottom right corner of enemy is within player
//	if (((enemyX + enemyWidth) > playerX) && ( (enemyY + enemyHeight) 
//		> playerY ) &&  ((enemyX + enemyWidth) < (playerX + 24)) 
//		&& ((enemyY + enemyWidth) < (playerY + 24))) {
//		return 1;
//	} 
//	//check bottom left corner of enemy
//	if ( (enemyX < (playerX + 24)) 
//		&& (enemyX > playerX)
//		&& ((enemyY + enemyHeight) > playerY)
//		&& ((enemyY + enemyHeight) < (playerY + 24))
//		) {
//		return 1;
//	}
//	//check top left corner
//	if ( (enemyX < (playerX + 24)) 
//		&& (enemyX > playerX)
//		&& (enemyY > playerY)
//		&& (enemyY < (playerY + 24))
//		) {
//		return 1;
//	}	
//	//check top right corner
//	if ( ((enemyX + enemyWidth) < (playerX + 24)) 
//		&& ((enemyX + enemyWidth) > playerX)
//		&& (enemyY > playerY)
//		&& (enemyY < (playerY + 24))
//		) {
//		return 1;
//	}	
//	return 0;
//}

void endGame() {
	//start Game Over State
	drawImage3(0, 0, 240, 160, gameover);
	drawHollowRect(0, 0, 240, 160, WHITE);
	while(1) {
		if (KEY_DOWN_NOW(BUTTON_SELECT)) {
			galaga_game();
		}
		if (KEY_DOWN_NOW(BUTTON_START))	{
			galaga_game();
		}
	}
}

int galaga_score() {
	while(1){
		receive();
 		sprintf(buffer, "Vidas: %d    Score: %d", lives, score);
		print_text_on_vga(4, 164, buffer);
	}
}

int galaga(){

	pid_game = create(galaga_game, 1024, 20, "Galaga Game", 0);
	pid_score = create(galaga_score, 1024, 20, "Galaga Score", 0);
	pid_control = currpid;
	resume(pid_game);
	resume(pid_score);
	while(receive()!=BUTTON_ESC);

	kill(pid_game);
	kill(pid_score);
}