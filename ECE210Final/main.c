//*****************************************************************************
// main.c
// Author: jkrachey@wisc.edu
//*****************************************************************************
#include "lab_buttons.h"

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
 * Global Variables
 *****************************************************************************/


//*****************************************************************************
//*****************************************************************************
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
  
  while(1)
  {
    if(ece210_wireless_data_avaiable())
    {
      rx_data = ece210_wireless_get();
      if( rx_data == UP_BUTTON)
      {
        ece210_lcd_add_msg("Remote UP BUTTON Pressed", TERMINAL_ALIGN_CENTER, LCD_COLOR_RED);
      }
    }
    
    if(AlertButtons)
    {
      AlertButtons = false;
      
      // Transmit data
      buttons = ece210_buttons_read();
      if(buttons == UP_BUTTON)
      {
        ece210_lcd_add_msg("UP BUTTON Pressed", TERMINAL_ALIGN_CENTER, LCD_COLOR_GREEN);
        ece210_wireless_send(UP_BUTTON);
      }
      

    }
    
  }
}


