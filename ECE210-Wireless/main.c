//*****************************************************************************
// main.c
// Author: jkrachey@wisc.edu
//*****************************************************************************
#include "lab_buttons.h"
#include "time.h"
#include <stdlib.h>

#define MOVE_PIXELS   1

// Turn Debuggin off
#undef DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

#define UP_BUTTON 0x01

#define LOCAL_ID     0x63
#define REMOTE_ID    0x62
/******************************************************************************
 * Structs
 *****************************************************************************/
struct weapon {
	char *name;
	uint8_t damageDisable;
	uint8_t damageNorm;
	uint8_t breachChance;
	uint8_t fireChance;
	uint8_t cooldown;
	uint8_t cost;
	uint8_t energyCost;
	uint8_t shotCount;
	uint8_t accuracy;
};

struct shipStatus {
	uint8_t oxygen;
	bool breach;
	uint8_t breachCount;
	uint8_t disable;
	bool fire;
	uint8_t fireCount;
	uint8_t currShield;
	clock_t disableTime;
	//clock_t disableWeapon;
};


struct ship {
	uint8_t health;
	uint8_t shield;
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
	{"Ion Burst",        1,  0,   0,  70,  5,  20,  2, 3,  85},
	{"Missile Launcher", 0,  3,  60,  20,  4,  20,  3, 1,  85},
	{"Flak Launcher",    0,  1,  50,   0,  7,  20,  4, 7,  85},
	{"Beam Laser",       0,  4,   0,  70,  4,  20,  3, 1, 100},
	{"Burst Laser",      0,  2,   0,  30,  4,  20,  1, 2,  85},
	{"Hull Bomb",        0,  2,  90,  20,  4,  20,  2, 1,  85},
	{"Scatter Laser",    0,  1,   0,  15,  5,  20,  4, 6,  60},
	{"Chain Missile",    0,  2,  40,  10,  7,  20,  4, 3,  80},
	{"Ion Beam",         4,  0,   0,  80,  6,  20,  2, 1, 100},
	{"NULLWEAPON",       0,  0,   0,   0,  0,   0,  0, 0,   0}
};

struct ship myShip = {20, 2, 8, 10, 10, 10};
struct shipStatus status = {100, false, 0, 0, false, 0, 2};
bool STATUS_DEAD = false;


uint16_t greenLight[] = {0x04, 0xFF, 0x00};
uint16_t redLight[] = {0xFF, 0x00, 0x00};
uint16_t yellowLight[] = {0xFF, 0xFF, 0x00};
uint16_t blueLight[] = {0x00, 0xFF, 0xFF};


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


// setFire()
// input: chance of fire
// output: if fire occured
bool setFire(uint8_t chance) {
	if(rand() % 100 <= chance) {
		status.fireCount += 1;
		return true;
	} else { return false;}
}
// setBreach()
// input: chance of breach
// output: if breach occured
bool setBreach(uint8_t chance) {
	if(rand() % 100 <= chance) {
		status.breach += 1;
		return true;
	} else { return false;}

}
// didHit()
// input: chance of breach
// output: if breach occured
bool didHit(uint8_t chance) {
	if(rand() % 100 <= chance) {
		return true;
	}	else { return false;}
}

// doAttack()
// input: weapon from weaponlist
// output: cooldown, not used currently
// makes changes on your ship from attack
uint8_t doAttack(struct weapon weapon ) {
	for(uint8_t i = 0; i < weapon.shotCount; i++) {	// for each shot calculate effects
		if(didHit(weapon.accuracy)) {
			if(status.currShield == 0){
				// TODO: Flash lights for damage
				status.disableTime = weapon.damageDisable;
				myShip.health -= weapon.damageNorm;
				status.breach = setBreach(weapon.breachChance);
				status.fire = setFire(weapon.fireChance);
				if(myShip.health == 0 || myShip.health > 24) {
					STATUS_DEAD = true;
				}
			} else {
				// TODO: Flash lights for shield
				status.disableTime += weapon.damageDisable; // cooldown timer for recharge
				status.currShield -= weapon.damageDisable;
				status.currShield -= weapon.damageNorm;
				if(status.currShield < 0){	// make sure shield isn't negative
					status.currShield = 0;
				}
			}
		}
	}
	return weapon.cooldown;
}

// updateHealthBar()
// input:
// output:
// updates top 8 lights for health
void updateHealthBar(uint8_t damage) {
	myShip.health -= damage;
	uint8_t health = myShip.health;
	for(uint8_t i = 0; i < 8; i++) {
		if(health > 3) {
			health -=3;
			ece210_ws2812b_write(i, greenLight[0], greenLight[1], greenLight[2] );
		} else if(health == 2) {
			health = 0;
			ece210_ws2812b_write(i, yellowLight[0], yellowLight[1], yellowLight[2]);
		} else if(health == 1) {
			health = 0;
			ece210_ws2812b_write(i, redLight[0], redLight[1], redLight[2]);
		} else if(health == 0) {
			ece210_ws2812b_write(i, 0x00, 0x00, 0x00);
		}
	}

	for(uint8_t i = 0; i < status.shield; i++) {
		ece210_ws2812b_write(i, blueLight[0], blueLight[1], blueLight[2]);
	}
}


int
main(void)
{


	// Initialize Ship


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

	clock_t shieldRecharge = 5 * CLOCKS_PER_SEC;

	clock_t startCD1;
	clock_t startCD2;
	clock_t startCD3;
	clock_t rechargeCD;

	bool canFire1 = true;
	bool canFire2 = true;
	bool canFire3 = true;
	bool canRecharge = true;
//***********************************************************************





  while(1)
  {

    // weapon 1 cooldown
	if(ece210_wireless_data_avaiable()) {
		uint32_t catchData = ece210_wireless_get();

		if(catchData < 11) { // < 11 means it is an attack
			doAttack(weaponList[catchData]);
			ece210_lcd_add_msg(weaponList[catchData].name, TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);

		} else if(catchData == 99) {	//99 indicates game over

		}
	if(STATUS_DEAD == true) {
		ece210_lcd_add_msg("YOU LOSE", TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
		ece210_wireless_send(99); // 99 indicates game over
		break;
	}

	}
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
	// shield cooldown
	if(!canRecharge) {
		canRecharge = checkCooldown(rechargeCD, status.disableTime);
		if(canRecharge) { //reset timer
			status.disableTime = 0;
		}
	}
	// TODO: Oxygen Ticks
	// TODO: Recieve Attacks-------- Done?
	// TODO: Implement Repair
	// TODO:
	bool didRecieve;
	if(btn_down_pressed()) {
		ece210_wireless_send(myShip.weapon1);
	}




	}
	if(btn_left_pressed()) {
		if(canFire1){
			startCD1 = clock();
			ece210_wireless_send(myShip.weapon1);
			canFire1 = false;
		}
	}
	if(btn_up_pressed()) {
		if(canFire2){
			startCD2 = clock();
			// Insert weapon action code
			ece210_wireless_send(myShip.weapon2);
			canFire2 = false;
		}
	}
	if(btn_right_pressed()) {
		if(canFire3){
			startCD3 = clock();
			// Insert weapon action code
			ece210_wireless_send(myShip.weapon3);
			canFire3 = false;
		}
	}

	if(status.disableTime > 0) { //disable time set
		canRecharge = false;
		rechargeCD = clock();
	}


  }
