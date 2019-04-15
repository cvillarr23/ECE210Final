//*****************************************************************************
// main.c
// Author: jkrachey@wisc.edu
//*****************************************************************************
#include "lab_buttons.h"
#include "time.h"

#define MOVE_PIXELS   1

// Turn Debuggin off
#undef DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

#define UP_BUTTON 0x01

#define LOCAL_ID     0x01
#define REMOTE_ID    0x00
/******************************************************************************
 * Structs
 *****************************************************************************/
struct weapon {
	char *name;-
	short damageDisable;
	short damageNorm;
	short breachChance;
	short fireChance;
	short cooldown;
	uint8_t cost;
	uint8_t energyCost;
	uint8_t shotCount;
	uint8_t accuracy;
};

struct ship {
	uint8_t health;
	uint8_t energy;
	uint8_t weapon1;
	uint8_t weapon2;
	uint8_t weapon3;
};
//*****************************************************************************
//*****************************************************************************

	
	
/******************************************************************************
 * Global Variables
 *****************************************************************************/
struct weapon weaponList[11] = {
// NAME               dis norm brc fire cd cst ecst sCnt acc
	{"Ion Cannon",       2,  0,   0,  50,  4,  20,  1, 1,  85},
	{"Ion Burst",        1,  0,   0,  70,  4,  20,  2, 3,  85},
	{"Missile Launcher", 0,  3,  60,  20,  4,  20,  2, 1,  85},
	{"Flak Launcher",    0,  1,  50,   0,  7,  20,  4, 7,  85},
	{"Beam Laser",       0,  4,   0,  70,  4,  20,  3, 1, 100},
	{"Burst Laser",      0,  2,   0,  30,  4,  20,  1, 2,  85},
	{"Hull Bomb",        0,  2,  90,  20,  4,  20,  2, 1,  85},
	{"Scatter Laser",    0,  1,   0,  15,  5,  20,  4, 5,  75},
	{"Chain Missile",    0,  2,  40,  10,  7,  20,  4, 3,  80},
	{"Ion Beam",         4,  0,   0,  80,  4,  20,  2, 1, 100},
	{"NULLWEAPON",       0,  0,   0,   0,  0,   0,  0, 0,   0}
};

struct ship myShip = {20, 8, 11, 11, 11};

//*****************************************************************************
//*****************************************************************************

// checkCooldown()
// input: start of CD
// input: weapon CD
// output: if the weapon is off CD
bool checkCooldown(clock_t startTime, clock_t weaponCD){
	clock_t elapsedCD = clock() - startTime;
			if(elapsedCD >= weaponCD ){
				return true;
			} else { return false;}
}


int 
main(void)
{
	char msg[80];
  uint32_t rx_data;
  uint32_t tx_data;
  uint8_t buttons;
  
  ece210_initialize_board();
  ece210_lcd_add_msg("Wireless TEST CODE\n\r",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
  sprintf(msg,"Local ID %d",LOCAL_ID);
  ece210_lcd_add_msg(msg,TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
  sprintf(msg,"Remote ID %d",REMOTE_ID);
  ece210_lcd_add_msg(msg,TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
  ece210_wireless_init(LOCAL_ID,REMOTE_ID);
	
//***********************************************************************
//****************  INITIALIZE COOLDOWN VARIABLES ***********************
  clock_t weaponCD1 = weaponList[myShip.weapon2].cooldown * CLOCKS_PER_SEC;
	clock_t weaponCD2 = weaponList[myShip.weapon2].cooldown * CLOCKS_PER_SEC;
	clock_t weaponCD3 = weaponList[myShip.weapon3].cooldown * CLOCKS_PER_SEC;
	
	clock_t startCD1;
	clock_t startCD2;
	clock_t startCD3;
	
	bool canFire1 = true;
	bool canFire2 = true;
	bool canFire3 = true;
//***********************************************************************		

  while(1)
  {
    // weapon 1 cooldown
		if(!canFire1) {
			canFire1 = checkCooldown(startCD1, weaponCD1);
		}
		// weapon 2 cooldown
		if(!canFire2) {
			canFire2 = checkCooldown(startCD2, weaponCD2);
		}
		// weapon 3 cooldown
		if(!canFire3) {
			canFire3 = checkCooldown(startCD3, weaponCD3);
		}
		
		
		
  }
	

	
}


