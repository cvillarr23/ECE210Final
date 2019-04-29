//*****************************************************************************
// main.c
// Author: jkrachey@wisc.edu
//*****************************************************************************
#include "lab_buttons.h"
#include "time.h"
#include <stdlib.h>
#include <stdio.h>



#define MOVE_PIXELS	 1

// Turn Debuggin off
#undef DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x) do {} while (0)
#endif

#define UP_BUTTON 0x01
#define DOWN_BUTTON 0x02
#define LEFT_BUTTON 0x04
#define RIGHT_BUTTON 0x08

#define lightOFF 0x00

#define LOCAL_ID 0x63
#define REMOTE_ID 0x62
	

/******************************************************************************
 * Structs
 *****************************************************************************/
typedef struct Weapon {
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
} Weapon;

typedef struct ShipStatus {
	uint8_t oxygen;
	uint8_t breachCount;
	uint8_t disable;
	uint8_t fireCount;
	uint8_t currShield;
	clock_t disableTime;
	//clock_t disableWeapon;
} ShipStatus;


typedef struct Ship {
	uint8_t health;
	uint8_t shield;
	uint8_t energy;
	uint8_t weapon1;
	uint8_t weapon2;
	uint8_t weapon3;
} Ship;
//*****************************************************************************
//*****************************************************************************



/******************************************************************************
 * Global Variables
 *****************************************************************************/
Weapon weaponList[11] = {
// NAME							 dis norm brc fire cd cst ecst sCnt acc
	{"Ion Cannon",			 2,	0,	 0,	50,	4,	20,	1, 1,	85},
	{"Ion Burst",				1,	0,	 0,	70,	5,	20,	2, 3,	85},
	{"Missile Launcher", 0,	3,	60,	20,	4,	20,	3, 1,	85},
	{"Flak Launcher",		0,	1,	50,	 0,	7,	20,	4, 7,	85},
	{"Beam Laser",			 0,	4,	 0,	70,	4,	20,	3, 1, 100},
	{"Burst Laser",			0,	2,	 0,	30,	4,	20,	1, 2,	85},
	{"Hull Bomb",				0,	2,	90,	20,	4,	20,	2, 1,	85},
	{"Scatter Laser",		0,	1,	 0,	15,	5,	20,	4, 6,	60},
	{"Chain Missile",		0,	2,	40,	10,	7,	20,	4, 3,	80},
	{"Ion Beam",				 4,	0,	 0,	80,	6,	20,	2, 1, 100},
	{"NULLWEAPON",			 0,	0,	 0,	 0,	0,	 0,	0, 0,	 0}
};

Ship myShip = {24, 2, 8, 1, 2, 6};
ShipStatus status = {100, 0, 0, 0, 2, 0};
bool STATUS_DEAD = false;
uint8_t buttons;


uint8_t convertValuesHex[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
uint8_t convertValuesInt[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 99};

uint8_t greenLight[] = {0x04, 0xFF, 0x00};
uint8_t redLight[] = {0xFF, 0x00, 0x00};
uint8_t yellowLight[] = {0xFF, 0xFF, 0x00};
uint8_t blueLight[] = {0x00, 0xFF, 0xFF};

uint8_t o2FireMultiplier = 1;
uint8_t o2BreachMultiplier = 2;
uint8_t fireDamagePeriod = 5;

bool wirelessOn = false;
//*****************************************************************************
//*****************************************************************************

// checkCooldown()
// input: start of CD
// input: weapon CD
// output: if the weapon is off CD
bool checkCooldown(clock_t endTime){
	
		if(clock() >= endTime ){
			ece210_lcd_add_msg("NOT ON COOLDOWN",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
			return true;
		} else { 
			ece210_lcd_add_msg("ON COOLDOWN",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
			return false;
		}
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
		status.breachCount += 1;
		return true;
	} else { return false;}
}
// didHit()
// input: chance of breach
// output: if breach occured
bool didHit(uint8_t chance) {
	if(rand() % 100 <= chance) {
		return true;
	}	else { 
		ece210_lcd_add_msg("MISSED!",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
	return false;
	}
}

// doAttack()
// input: weapon from weaponlist
// output: cooldown, not used currently
// makes changes on your ship from attack
uint8_t doAttack(Weapon weapon ) {
	for(uint8_t i = 0; i < weapon.shotCount; i++) {	// for each shot calculate effects
		if(didHit(weapon.accuracy)) {
			if(status.currShield == 0){
				// TODO: Flash lights for damage
				status.disableTime += weapon.damageDisable * CLOCKS_PER_SEC;
				myShip.health -= weapon.damageNorm;
				char printStr[32];
				sprintf(printStr, "Took %d damage", weapon.damageNorm); 
				ece210_lcd_add_msg(printStr,TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				sprintf(printStr, "%d Health Remaining", myShip.health);
				ece210_lcd_add_msg(printStr,TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				setBreach(weapon.breachChance);
				setFire(weapon.fireChance);
				if(myShip.health == 0 || myShip.health > 24) {
					STATUS_DEAD = true;
				}
			} else {
				// TODO: Flash lights for shield
				status.disableTime += weapon.damageDisable * CLOCKS_PER_SEC; // cooldown timer for recharge
				status.currShield -= weapon.damageDisable;
				status.currShield -= weapon.damageNorm;
				if(status.currShield > 2){	// make sure shield isn't negative
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
void updateHealthBar() {
	
	uint8_t health = myShip.health;
	uint8_t shield = status.currShield;
	for(uint8_t i = 0; i < 8; i++) {
		ece210_lcd_add_msg("Iterating health bar",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
		
		if(health >= 3) {
			health -=3;
			ece210_ws2812b_write(i, greenLight[0], greenLight[1], greenLight[2] );
		} else if(health == 2) {
			health = 0;
			ece210_ws2812b_write(i, yellowLight[0], yellowLight[1], yellowLight[2]);
		} else if(health == 1) {
			health = 0;
			ece210_ws2812b_write(i, redLight[0], redLight[1], redLight[2]);
		} else if(health == 0 || health > 24) {
			ece210_ws2812b_write(i, lightOFF, lightOFF, lightOFF);
		}
	}

	
}


// tickOxygen()
// input:
// output:
// ticks Oxygen based on # of breaches and fires
void tickOxygen() {
	status.oxygen -= status.fireCount * o2FireMultiplier;
	status.oxygen -= status.breachCount * o2BreachMultiplier;

}




// tickFireDamage()
// input:
// output:
// ticks fire based on # of fires
void tickFireDamage() {
	myShip.health -= status.fireCount;
}

bool tickCheck(clock_t rechargeTime, clock_t tickTimer) {
	if(status.currShield < myShip.shield && checkCooldown(status.disableTime)) {
		status.disableTime = clock() + rechargeTime;
		status.currShield += 1;
	}
//	if(checkCooldown(tickTimer)) {
//		tickFireDamage();
//		tickOxygen();
//		return true;
//	}
	return false;
}

uint8_t chooseWeapon(uint8_t slot) {
	uint8_t weaponIndex = 0;
	bool weaponChosen = false;
	char printStr[32];
	sprintf(printStr, "Press Up to choose weapon for slot %d", slot);
	ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
	while(!weaponChosen) {
		if(AlertButtons){
			AlertButtons = false;
			buttons = ece210_buttons_read();
				if(buttons == RIGHT_BUTTON) {
					if(weaponIndex < 9) {
						weaponIndex += 1;
					} else {
						weaponIndex = 0;
					}
					ece210_lcd_add_msg(weaponList[weaponIndex].name, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					sprintf(printStr, "Damage:  %d", weaponList[weaponIndex].damageNorm);
					ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					sprintf(printStr, "Accuracy:  %d", weaponList[weaponIndex].accuracy);
					ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					sprintf(printStr, "Shot Count:  %d", weaponList[weaponIndex].shotCount);
					ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
				} else if(buttons == LEFT_BUTTON) {
					if(weaponIndex > 0) {
						weaponIndex -= 1;
					} else {
						weaponIndex = 9;
					}
					ece210_lcd_add_msg(weaponList[weaponIndex].name, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					sprintf(printStr, "Damage:  %d", weaponList[weaponIndex].damageNorm);
					ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					sprintf(printStr, "Accuracy:  %d", weaponList[weaponIndex].accuracy);
					ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					sprintf(printStr, "Shot Count:  %d", weaponList[weaponIndex].shotCount);
					ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
					ece210_lcd_add_msg("", TERMINAL_ALIGN_LEFT, LCD_COLOR_WHITE);
				} else if(buttons == UP_BUTTON) {
					weaponChosen = true;
					ece210_lcd_add_msg("Weapon Chosen!", TERMINAL_ALIGN_CENTER, LCD_COLOR_WHITE);
				}
		}
	}
	
	return weaponChosen;
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
	
	ece210_lcd_add_msg("Initializing....",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
//***********************************************************************
//****************	INITIALIZE COOLDOWN VARIABLES ***********************
	clock_t weaponCD1 = weaponList[myShip.weapon2].cooldown * CLOCKS_PER_SEC;	// Cooldowns
	clock_t weaponCD2 = weaponList[myShip.weapon2].cooldown * CLOCKS_PER_SEC;
	clock_t weaponCD3 = weaponList[myShip.weapon3].cooldown * CLOCKS_PER_SEC;
	ece210_lcd_add_msg("Weapon CD Init Complete",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	
	clock_t shieldRecharge = 5 * CLOCKS_PER_SEC;
	clock_t endTime1;														// When weapons can be used again
	clock_t endTime2;
	clock_t endTime3;
	clock_t tickInterval = 10 * CLOCKS_PER_SEC; // every 10 sec tick ox + fire
	clock_t tickTimer = clock() + tickInterval;	// Used to indicate when to tick everything
		
	ece210_lcd_add_msg("Start CD complete",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	
	myShip.weapon1 = chooseWeapon(1);
	myShip.weapon2 = chooseWeapon(2);
	myShip.weapon3 = chooseWeapon(3);
//***********************************************************************
//****************	INITIALIZE WIRELESS SETTINGS ***********************
	ece210_lcd_add_msg("Press Right for Player 1, Left for Player 2",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
//	ece210_wireless_init(LOCAL_ID, REMOTE_ID);
	do {
		if(btn_right_pressed()) {
			ece210_wireless_init(REMOTE_ID, LOCAL_ID);
			wirelessOn = true;
			ece210_lcd_add_msg("PLAYER 1",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
		} else if(btn_left_pressed()) {
			ece210_wireless_init(LOCAL_ID, REMOTE_ID);
			wirelessOn = true;
			ece210_lcd_add_msg("PLAYER 2",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
		}
	} while(!wirelessOn);
	ece210_lcd_add_msg("Wireless Init Complete",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);

	clock_t tickStart = clock();
	ece210_lcd_add_msg("Entered while loop",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	

	while(1) {
// ***********************************************************************************
//																RECIEVE DATA CYCLE
// ***********************************************************************************
		ece210_lcd_add_msg("Looping",TERMINAL_ALIGN_CENTER,LCD_COLOR_GREEN);
		//updateHealthBar();
		ece210_wait_mSec(300);
		if(ece210_wireless_data_avaiable()) {
			uint8_t catchData = ece210_wireless_get();
			//ece210_lcd_add_msg("Catching Data...",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
			//char printStr[16];
			//sprintf(printStr, "Value is %d", catchData); 
			//ece210_lcd_add_msg(printStr, TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
//***********************************************************************
//******************************ANALYZE DATA*****************************	


			if(catchData < 11) { // < 11 means it is an attack
					doAttack(weaponList[catchData]);
					//updateHealthBar();
					ece210_lcd_add_msg(weaponList[catchData].name, TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);

			} else if(catchData == 99) {	//99 indicates game over
				ece210_lcd_add_msg("YOU WIN", TERMINAL_ALIGN_CENTER, LCD_COLOR_GREEN);
				break;
			} else {
				ece210_lcd_add_msg("RECIEVED DATA, ERROR",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
			} 
			if(STATUS_DEAD == true) {
				ece210_lcd_add_msg("YOU LOSE", TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
				ece210_wireless_send(99); // 99 indicates game over
				break;
			}
		
		}
		
// ***********************************************************************************
//																SHIELD CYCLE
// ***********************************************************************************
//		if(tickCheck(shieldRecharge, tickTimer)) {
//			tickTimer = clock() + tickInterval;
//		}
	
		
// ***********************************************************************************
//																ATTACK CYCLE
// ***********************************************************************************	
		if(AlertButtons) {
			AlertButtons = false;
			buttons = ece210_buttons_read();
			
			if(buttons == LEFT_BUTTON) {
				if(checkCooldown(endTime1)){
					ece210_lcd_add_msg("FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					endTime1 = clock() + weaponCD1;
					// Insert weapon action code
					if(!ece210_wireless_send(myShip.weapon1)) {
						ece210_lcd_add_msg("NOT FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					}
				} else {
					ece210_lcd_add_msg("Weapon On Cooldown",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				}
			}
			if(buttons == UP_BUTTON) {
				if(checkCooldown(endTime2)){
					ece210_lcd_add_msg("FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					endTime2 = clock() + weaponCD2;
					// Insert weapon action code
					if(!ece210_wireless_send(myShip.weapon2)) {
						ece210_lcd_add_msg("NOT FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					}
				} else {
					ece210_lcd_add_msg("Weapon On Cooldown",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				}
			}
				
			if(buttons == RIGHT_BUTTON) {

				if(checkCooldown(endTime3)){
					ece210_lcd_add_msg("FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					endTime3 = clock() + weaponCD3;
					// Insert weapon action code
					if(!ece210_wireless_send(myShip.weapon3)) {
						ece210_lcd_add_msg("NOT FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					}
				} else {
					ece210_lcd_add_msg("Weapon On Cooldown",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				}
			}
			
		}
	



	}
}
