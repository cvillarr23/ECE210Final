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
	uint8_t disableTime;
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

void updateHealth(uint8_t damage);

/******************************************************************************
 * Global Variables
 *****************************************************************************/
Weapon weaponList[11] = {
// NAME							 dis norm brc fire cd cst ecst sCnt acc
	{"Ion Cannon",			 8,	0,	 0,	50,	16,	20,	1, 1,	85},
	{"Ion Burst",				4,	0,	 0,	70,	20,	20,	2, 3,	85},
	{"Missile Launcher", 0,	3,	60,	20,	16,	20,	3, 1,	85},
	{"Flak Launcher",		0,	1,	50,	 0,	28,	20,	4, 7,	85},
	{"Beam Laser",			 0,	4,	 0,	70,	28,	20,	3, 1, 100},
	{"Burst Laser",			0,	2,	 0,	30,	16,	20,	1, 2,	85},
	{"Hull Bomb",				0,	2,	90,	20,	16,	20,	2, 1,	85},
	{"Scatter Laser",		0,	1,	 0,	15,	20,	20,	4, 6,	60},
	{"Chain Missile",		0,	2,	40,	10,	28,	20,	4, 3,	80},
	{"Ion Beam",				 16,	0,	 0,	80,	24,	20,	2, 1, 100},
	{"NULLWEAPON",			 0,	0,	 0,	 0,	0,	 0,	0, 0,	 0}
};

Ship myShip = {24, 4, 8, 1, 2, 6};
ShipStatus status = {100, 0, 0, 0, 2, 0};
bool STATUS_DEAD = false;
uint8_t buttons;
uint8_t LEDNum = 7;


uint8_t convertValuesHex[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
uint8_t convertValuesInt[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 99};

uint8_t greenLight[] = {0x06, 0x66, 0x00};
uint8_t redLight[] = {0xFF, 0x00, 0x00};
uint8_t yellowLight[] = {0xFF, 0xFF, 0x00};
uint8_t blueLight[] = {0x00, 0xFF, 0xFF};
uint8_t currColor[3] = {0xFF, 0xFF, 0x00};

uint8_t o2FireMultiplier = 1;
uint8_t o2BreachMultiplier = 2;
uint8_t fireDamagePeriod = 5;

bool wirelessOn = false;
//*****************************************************************************
//*****************************************************************************


// setFire()
// input: chance of fire
// output: if fire occured
bool setFire(uint8_t chance) {
	if(rand() % 100 <= chance) {
		status.fireCount += 1;
		ece210_lcd_add_msg("There was a fire on board!",TERMINAL_ALIGN_CENTER,LCD_COLOR_ORANGE);
		return true;
	} else { return false;}
}
// setBreach()
// input: chance of breach
// output: if breach occured
bool setBreach(uint8_t chance) {
	if(rand() % 100 <= chance) {
		status.breachCount += 1;
		ece210_lcd_add_msg("There was a hull breach!",TERMINAL_ALIGN_CENTER,LCD_COLOR_ORANGE);
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
				status.disableTime += weapon.damageDisable * 4;
				myShip.health -= weapon.damageNorm;
				updateHealth(weapon.damageNorm);
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
				ece210_lcd_add_msg("Hit Shield!" ,TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				status.disableTime += weapon.damageDisable;
				status.currShield -= weapon.damageDisable;
				status.currShield -= weapon.damageNorm;
				char printStr[32];
				sprintf(printStr, "Shield Status:  %d", status.currShield);
				ece210_lcd_add_msg(printStr,TERMINAL_ALIGN_CENTER,LCD_COLOR_WHITE);
				if(status.currShield > 2 || status.currShield < 0){	// make sure shield isn't negative
					status.currShield = 0;
					
				}
				if(status.currShield == 0) {
					ece210_tiva_rgb_write(0x00);
				}
			}
		}
	}
	return weapon.cooldown;
}

// tickOxygen()
// input:
// output:
// ticks Oxygen based on # of breaches and fires
void tickOxygen() {
	status.oxygen -= status.fireCount * o2FireMultiplier;
	status.oxygen -= status.breachCount * o2BreachMultiplier;
//	if(status.oxygen == 0){
//		ece210_wireless_send(99);
//		STATUS_DEAD = true;
//	}
}




// tickFireDamage()
// ticks fire based on # of fires
void tickFireDamage() {
	myShip.health -= status.fireCount;
}


// tickCheck()
// tick fire, oxygen, and shield recharge
bool tickCheck() {
	if(status.currShield < myShip.shield) {
		status.currShield += 1;
		char printStr[32];
		ece210_tiva_rgb_write(0x0F);
		
		sprintf(printStr, "Shield Recharged to:  %d", status.currShield);
		ece210_lcd_add_msg(printStr,TERMINAL_ALIGN_CENTER,LCD_COLOR_WHITE);
		
	}
	tickFireDamage();
	tickOxygen();

	return true;
}

void updateHealth(uint8_t damage) {
	
	
	for(uint8_t i = 0; i < damage; i++) {
		ece210_ws2812b_write(LEDNum, currColor[0], currColor[1], currColor[2]);
		if(LEDNum == 0) {
			LEDNum = 7;
			if(myShip.health > 16) {
				currColor[0] = yellowLight[0];
				currColor[1] = yellowLight[1];
				currColor[2] = yellowLight[2];
				
			} else if(myShip.health > 8) {
				currColor[0] = redLight[0];
				currColor[1] = redLight[1];
				currColor[2] = redLight[2];
				
			} else if(myShip.health > 0) {
				currColor[0] = 0x00;
				currColor[1] = 0x00;
				currColor[2] = 0x00;
				
			}
		} else {
			LEDNum -= 1;
		}
	}

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
					sprintf(printStr, "Disable Time:  %d", weaponList[weaponIndex].damageDisable / 4);
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
	
	return weaponIndex;
}






int
main(void)
{


	// Initialize Ship
	
	// Turn on lights

	char msg[80];
	uint32_t rx_data;
	uint32_t tx_data;
	uint8_t buttons;

	ece210_initialize_board();
	
		for(uint8_t i = 0; i < 8; i++) {	
		ece210_ws2812b_write(i, greenLight[0], greenLight[1], greenLight[2] );
	}
	ece210_tiva_rgb_write(0x0F);
	
	ece210_lcd_add_msg("Initializing....",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
//***********************************************************************
//****************	INITIALIZE COOLDOWN VARIABLES ***********************	
	uint8_t weapCnt1 = 0;			//	Cooldown counters, based of 500ms wait between loops
	uint8_t weapCnt2 = 0;
	uint8_t weapCnt3 = 0;
	


	
	uint8_t tickInterval = 20;	// 5 sec
	uint8_t tickCnt = 0;
	
	ece210_lcd_add_msg("CD Variable Init Complete",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	
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

	
	ece210_lcd_add_msg("Entered while loop",TERMINAL_ALIGN_CENTER,LCD_COLOR_BLUE);
	

	while(1) {
// ***********************************************************************************
//																RECIEVE DATA CYCLE
// ***********************************************************************************

		ece210_wait_mSec(250);
		if(ece210_wireless_data_avaiable()) {
			uint8_t catchData = ece210_wireless_get();
//***********************************************************************
//******************************ANALYZE DATA*****************************	
			if(catchData < 11) { // < 11 means it is an attack
					doAttack(weaponList[catchData]);

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
//																COOLDOWN CYCLE
// ***********************************************************************************
		weapCnt1 += 1;
		weapCnt2 += 1;
		weapCnt3 += 1;
		if(status.disableTime != 0) {
			status.disableTime -= 1;
		} else {
			tickCnt += 1;
		}
		
		if(tickCnt >= tickInterval) {
			tickCheck();
			tickCnt = 0;
		}
		
// ***********************************************************************************
//																ATTACK CYCLE
// ***********************************************************************************	
		if(AlertButtons) {
			AlertButtons = false;
			buttons = ece210_buttons_read();
			
			if(buttons == LEFT_BUTTON) {
				if(weapCnt1 >= weaponList[myShip.weapon1].cooldown){
					weapCnt1 = 0;
					ece210_lcd_add_msg("FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);

					// Insert weapon action code
					while(!ece210_wireless_send(myShip.weapon1)) {
						ece210_lcd_add_msg("NOT FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					}
				} else {
					ece210_lcd_add_msg("Weapon On Cooldown",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				}
			}
			if(buttons == UP_BUTTON) {
				if(weapCnt2 >= weaponList[myShip.weapon2].cooldown){
					weapCnt2 = 0;
					ece210_lcd_add_msg("FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					// Insert weapon action code
					while(!ece210_wireless_send(myShip.weapon2)) {
						ece210_lcd_add_msg("NOT FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					}
				} else {
					ece210_lcd_add_msg("Weapon On Cooldown",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				}
			}
				
			if(buttons == RIGHT_BUTTON) {

				if(weapCnt3 >= weaponList[myShip.weapon3].cooldown){
					weapCnt3 = 0;
					ece210_lcd_add_msg("FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					// Insert weapon action code
					while(!ece210_wireless_send(myShip.weapon3)) {
						ece210_lcd_add_msg("NOT FIRED",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
					}
				} else {
					ece210_lcd_add_msg("Weapon On Cooldown",TERMINAL_ALIGN_CENTER,LCD_COLOR_RED);
				}
			}
			
		}
	



	}
}
