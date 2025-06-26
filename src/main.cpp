#include <M5StickC.h>
#undef min
#include <stdlib.h>
#include <stdint.h>
#include "EEPROM.h"
#include "classes.h"

#define MEM_SIZE 1024

// Function declarations.
void newGame();
void clearMemory();

void setup()
{
  M5.begin();
  M5.IMU.Init();
  EEPROM.begin(MEM_SIZE);
  clearMemory();
  Serial.begin(155200);
  Serial.flush();
  M5.Lcd.fillScreen(BLACK); // set the default background color
  // Change the screen orientation to horizontal.
  M5.Lcd.setRotation(1);
}

void loop()
{
  newGame();
  M5.update();
  delay(100);
}

// Function that contains the loop for an entire game.
void newGame()
{
  // Begin with clean black screen.
  M5.Lcd.fillScreen(BLACK);

  // Make a new grid object
  Grid newGrid;

  while (!newGrid.hasEnded())
  {
    delay(250); // slow the cursor down.
    M5.update();
    newGrid.moveCursor(); // Check for cursor move and update it accordingly.

    // Enter the menu screen.
    if (M5.BtnB.wasPressed())
    {
      // Make a new menu.
      Menu newMenu;
      newMenu.drawMenu();

      int loop = true;
      while (loop) // Infinite loop that displays the menu. Break out of it by selecting an option.
      {
        delay(100);
        M5.update();
        newMenu.drawMenu();
        if (M5.BtnB.wasPressed()) // Scroll down the menu.
        {
          newMenu.goDownMenu();
        }
        else if (M5.BtnA.wasPressed()) // Select an option.
        {
          int option = newMenu.selectedOption;
          switch (option)
          {
          case 0:         // option 1: return (do nothing)
            loop = false; // Break out of the loop.
            break;
          case 1: // option 2: we save the game.
            loop = false;
            newGrid.saveGame();
            break;
          case 2: // option 3: we load a previously saved game.
            loop = false;
            newGrid.loadGame();
            break;
          case 3: // option 4: begin a new game.
            loop = false;
            newGrid.setGameEnded(1); // Trigger end condition of the current game.
            break;
          }
        }
      }
      newGrid.drawGrid(); // Draw the grid when exiting the menu.
    }
    // Update the game.
    else if (M5.BtnA.wasPressed())
    {
      newGrid.deleteSameColorNeighbors();
    }
  }
}

void clearMemory()
{
  int address = 0;
  for (int i = 0; i < MEM_SIZE; i++)
  {
    u_int8_t emptyValue = 0;
    EEPROM.writeByte(address, emptyValue);
    address++;
  }
}