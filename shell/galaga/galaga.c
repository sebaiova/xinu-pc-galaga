#include <xinu.h>
#include "titlescreen.h"
#include "playerImage.h"
#include "easyImage.h"
#include "hardImage.h"
#include "bossImage.h"
#include "gameover.h"
#include "shoot.h"
#include "joypad.h"

#define COLOR(r,g,b) (b<<11) | (g<<5) | r
#define BLACK COLOR(0, 0, 0)
#define YELLOW COLOR(31, 63, 0)
#define WHITE COLOR(31, 63, 31)
#define BUTTON_A		0x24 /*J*/
#define BUTTON_B		0x25 /*K*/
#define BUTTON_SELECT	0x03 /*2*/
#define BUTTON_START	0x2c /*Z*/
#define BUTTON_RIGHT	0x20 /*D*/
#define BUTTON_LEFT		0x1e /*A*/	
#define BUTTON_UP		0x11 /*W*/
#define BUTTON_DOWN 	0x1f /*S*/
#define BUTTON_R	'1'
#define BUTTON_L	'2'
#define BUTTON_ESC		0x01 /*ESC*/

//variable definitions
#define PLAYER_SPEED 2
#define ENEMY_SPEED 1
#define SHOOT_SPEED 4
#define SHOOT_FREQUENCY 6
#define INMUNITY_TIME 30

#define N_SHOOTS 10
#define N_EASY 9
#define N_HARD 9
#define N_BOSS 3
#define N_ENEMIES N_EASY+N_HARD+N_BOSS
#define N_OBJECTS 1+N_SHOOTS+N_ENEMIES

void setPixel(int x, int y, uint16 color);
void drawRect(int x, int y, int width, int height, uint16 color);
void drawHollowRect(int x, int y, int width, int height, uint16 color);
void drawHollowRectSize(int x, int y, int width, int height, uint16 color, int size);
void drawImage3(int x, int y, int width, int height, const uint16* image);

//helpers
void endGame();

struct ObjectData_t {
	const uint16* image;
	uint8 w, h, speed;
};

enum ObjectState { INACTIVE, ACTIVE, INMUNE = INMUNITY_TIME }; 
enum ObjectType  { PLAYER, ENEMY_EASY, ENEMY_HARD, ENEMY_BOSS, SHOOT } ;
enum EventMsg	 { SCORE_UP, STRUCKT, RESET };

const struct ObjectData_t obj_sheet[] = {  
	/* Type				Sprite		   Width			Height		  Speed       */
	/*[PLAYER]*/	{ playerImage,	PLAYER_WIDTH,	PLAYER_HEIGHT,	PLAYER_SPEED },  
	/*[ENEMY_EASY]*/{ easyImage,  	EASY_WIDTH, 	EASY_HEIGHT,	ENEMY_SPEED  },
	/*[ENEMY_HARD]*/{ hardImage,  	HARD_WIDTH, 	HARD_HEIGHT,	ENEMY_SPEED  },
	/*[ENEMY_BOSS]*/{ bossImage,  	BOSS_WIDTH, 	BOSS_HEIGHT,	ENEMY_SPEED   },
	/*[SHOOT]*/		{ shootImage,  	SHOOT_WIDTH,	SHOOT_HEIGHT,   SHOOT_SPEED  }
};

struct Object_t {
	uint8 x, y, type, state;
};

pid32 pid_game;
pid32 pid_score;
pid32 pid_control;

int frame_time;
int running;
int score;
int lives;
int enemies_left;
int shoot_decay;



void struck(struct Object_t* player, struct Object_t* enemy)
{
	enemies_left--;
	destroy_obj(enemy);
	player->state = INMUNE;
	send(pid_score, STRUCKT);
}

void destroy_obj(struct Object_t* obj)
{
	drawRect(obj->x, obj->y, obj_sheet[obj->type].w, obj_sheet[obj->type].h, BLACK);
	obj->state = INACTIVE;
}

void input(struct Object_t* objects, uint32 player_index, uint32 shoots_index, uint32* curr_shoot)
{
	struct Object_t* player = &objects[player_index];

	if (joypad_check(BUTTON_LEFT) && (player->x > 0)) {
		player->x -= PLAYER_SPEED;
	}
	if (joypad_check(BUTTON_RIGHT) && (player->x <= 216)) {
		player->x += PLAYER_SPEED;
	}
	if (joypad_check(BUTTON_UP) && (player->y > 25)) {
		player->y -= PLAYER_SPEED;
	}
	if (joypad_check(BUTTON_DOWN) && (player->y <= 136)) {
		player->y += PLAYER_SPEED;
	}
	if (joypad_check(BUTTON_A) && shoot_decay==0) {
		struct Object_t* shoot = &objects[shoots_index+*curr_shoot]; 
		if (shoot->state == INACTIVE) 
		{
			shoot_decay = SHOOT_FREQUENCY;
			shoot->state = ACTIVE;
			shoot->x = player->x+(PLAYER_WIDTH/2)-(SHOOT_WIDTH/2); 
			shoot->y = player->y-SHOOT_HEIGHT; 
			(*curr_shoot)++;
			if (*curr_shoot >= N_SHOOTS)
				*curr_shoot = 0;
		};
	}
	if (joypad_check(BUTTON_SELECT)) {
		running=FALSE;
	}
}

void drawObjects(struct Object_t* obj, uint32 size)
{
	for(int i=0; i<size; i++)
	{
		struct ObjectData_t data = obj_sheet[obj[i].type]; 
		if(obj[i].state>=ACTIVE)
		{
			if((obj[i].state%2)==0)
				drawRect(obj[i].x, obj[i].y, data.w, data.h, BLACK);	
			else 
				drawImage3(obj[i].x, obj[i].y, data.w, data.h, obj_sheet[obj[i].type].image);
			drawHollowRectSize(obj[i].x, obj[i].y, data.w, data.h, BLACK, data.speed);
		}
	}
}

#define spawn(x, y, type, state) obj[index++] = (struct Object_t){x, y, type, state};  
void initializeObjects(struct Object_t* obj, uint32 size)
{
	uint32 index = 0;
	spawn(120, 136, PLAYER, ACTIVE);  
	
	for(int i=0; i<N_EASY; i++)
		spawn(28*i, 80,  ENEMY_EASY, ACTIVE);
	
	for(int i=0; i<N_HARD; i++)
		spawn(28*i, 40, ENEMY_HARD, ACTIVE);

	for(int i=0; i<N_BOSS; i++)
		spawn(i*80, 0,  ENEMY_BOSS, ACTIVE);

	for(int i=0; i<N_SHOOTS; i++)
		spawn(0, 0, SHOOT, INACTIVE);
}

int collision(struct Object_t obj1, struct Object_t obj2)
{
	struct ObjectData_t data1 = obj_sheet[obj1.type];
	struct ObjectData_t data2 = obj_sheet[obj2.type]; 

	if( obj1.x < obj2.x + data2.w && obj1.x + data1.w > obj2.x &&
		obj1.y < obj2.y + data2.h && obj1.y + data1.h > obj2.y )
		return 1;

	return 0;
}

#define forEach_enemyIndex(i) for(int i=player_index+1; i<shoots_index; i++)
#define forEach_shootIndex(i) for(int i=shoots_index; i<size; i++)
void update(struct Object_t* obj, uint32 player_index, uint32 shoots_index, uint32 size)
{ 
	struct Object_t* player = &obj[player_index];

	if(shoot_decay>0)
		shoot_decay--;

	// INMUNITY DECAY
	if(player->state>ACTIVE)	
		obj->state--;

	forEach_enemyIndex(i) 
	{
		struct ObjectData_t data = obj_sheet[obj[i].type]; 
		obj[i].y += data.speed;
		if(obj[i].y > 160)
			obj[i].y = 0;
		if(obj[i].type==ENEMY_BOSS)
		{
			obj[i].x += data.speed;
			if(obj[i].x > 240)
				obj[i].x = 0;
		}
	}	
	if(player->state==ACTIVE)
	{	
		forEach_enemyIndex(i)
		{
			if(obj[i].state==ACTIVE)
			{	
				if(collision(obj[i], obj[player_index]))
				{	
					struck(&obj[player_index], &obj[i]);
					break;
				}	
			}		
		}	
	}
	forEach_shootIndex(i)
	{
		if(obj[i].state==ACTIVE)
		{
			forEach_enemyIndex(j)
			{
				if(obj[j].state==ACTIVE && collision(obj[j], obj[i]))
				{
					if(obj[j].type == ENEMY_HARD)
						obj[j].type = ENEMY_EASY;
					else 
					{
						destroy_obj(&obj[j]);
						enemies_left--;
					}
					destroy_obj(&obj[i]);
					send(pid_score, SCORE_UP);
				}
			}
			struct ObjectData_t data = obj_sheet[obj[i].type];
			obj[i].y -= data.speed;
			if(obj[i].y > 160)
			{
				obj[i].y = 3;
				destroy_obj(&obj[i]);
			}
		}	
	}	
}

int galaga_game() 
{
	struct Object_t objects[N_OBJECTS];
	while(1)
	{
		frame_time = 50;
		drawImage3(0, 0, 240, 160, titlescreen);
		while(1) 
		{
			if (joypad_check(BUTTON_START)) {
				break;
			}
			if (joypad_check(BUTTON_ESC)){
				send(pid_control, BUTTON_ESC);
			} 
		}

		shoot_decay = 0;
		running = TRUE;

		start_level:

		initializeObjects(objects, sizeof(objects)/sizeof(struct Object_t));
		enemies_left = N_ENEMIES;
		uint32 curr_shoot = 0;

		//start black screen for drawing
		for (int i = 0; i < 240; i++) {
			for (int j = 0; j < 160; j++) {
				setPixel(i, j, BLACK);
			}
		}

		while(running==TRUE) {
			//go back to title screen if select button is pressed

			input(objects, 0, N_ENEMIES+1, &curr_shoot);
			update(objects, 0, N_ENEMIES+1, sizeof(objects)/sizeof(struct Object_t));
			drawObjects(objects, sizeof(objects)/sizeof(struct Object_t));

			if(lives==0)
			{
				endGame();
			}
			else if(enemies_left==0)
			{
				frame_time -= frame_time/10;
				goto start_level;
			}

			sleepms(frame_time);
		}
		send(pid_score, RESET);
	}
	return 0;
}

void endGame()
{
	//start Game Over State
	drawImage3(0, 0, 240, 160, gameover);
	drawHollowRect(0, 0, 240, 160, WHITE);

	while(running==TRUE)
		if(joypad_check(BUTTON_START) || joypad_check(BUTTON_SELECT) ) 
			running = FALSE;
	
	sleep(1);
	send(pid_score, RESET);
}


int galaga_score()
{
	char buffer[32];
	score = 0;
	lives = 3;
	while(1)
	{
		switch(receive())
		{
			case STRUCKT: lives--;	/* FALLTHROUGH */
			case SCORE_UP: score++; if(score==100) { lives++; score++; }; break;
			case RESET: score = 0; lives = 3; break;
			default: break;
		}
 		sprintf(buffer, "Vidas: %d    Score: %d        ", lives, score);
		print_text_on_vga(4, 164, buffer);
	}
}

int galaga()
{
	print_text_on_vga(260, 16, "GALAGA ");
	print_text_on_vga(260, 32, "Quit: <ESC> at title");
	print_text_on_vga(260, 32, "Left <A> - Right <D> - Up <W> - Down <S>");
	print_text_on_vga(260, 48, "Start <Z> - Select <2> - A <J> - B <K>");

	pid_game = create(galaga_game, 1024, 20, "Galaga Game", 0);
	pid_score = create(galaga_score, 1024, 20, "Galaga Score", 0);

	joypad_run();

	pid_control = currpid;
	resume(pid_game);
	resume(pid_score);

	while(receive()!=BUTTON_ESC);

	joypad_stop();
	kill(pid_game);
	kill(pid_score);

	drawRect(0, 0, 240, 160, YELLOW);
	return 0;
}