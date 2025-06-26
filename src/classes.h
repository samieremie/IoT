#pragma once

#include <M5StickC.h>
#include <map>
#include <vector>
#include <optional>
#include <queue>

// Constants
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 80
#define BLOCK_WIDTH 10
#define BLOCK_HEIGHT 10

// Tilt constant used for moving the cursor
#define MIN_TILT 0.15

// Grid class forward declaration for Cursor.
class Grid;

// Block Class Declaration
class Block
{
private:
    int width = BLOCK_WIDTH;
    int height = BLOCK_HEIGHT;
    int blockType; // Block type determines the color.

public:
    Block();

    // Accessors
    int getBlockType();

    // Mutators
    void setBlockType(int newBlockType);

    // Method to draw a block
    void drawBlock(int col, int row, int color, int topSpace);
};

// Cursor class
class Cursor
{
private:
    int x_coord;
    int y_coord;
    int width = BLOCK_WIDTH;
    int height = BLOCK_HEIGHT;

public:
    // Initialize the cursor position to be in the bottom left corner.
    Cursor(int x = 0, int y = 70);

    // Accessors
    int getX();
    int getY();

    // Mutators
    void setX(int newX);
    void setY(int newY);

    // Method to draw the cursor on the screen.
    void drawCursor();
};

// Grid Class Declaration
class Grid
{
private:
    int width;
    int height;
    int numDifferentBlocks;
    int numBlocks;
    int topSpace;
    int score = 0;
    int bestScore = 0;
    int gameEnded = 0; // variable that is 1 if the game has ended.

public:
    std::map<int, int> blockColors = {
        {0, RED},
        {1, BLUE},
        {2, GREEN},
        {3, YELLOW},
        {4, PURPLE},
    }; // Map of block types to colors

    Cursor cursor;
    int rowCursor;
    int colCursor;
    std::vector<std::vector<std::optional<Block>>> matrix; // 2D matrix of blocks

    Grid();

    void initializeGrid();

    // Accessors
    int getWidth();
    int getHeight();
    int getNumDifferentBlocks();
    int getBlockColor(int blockType);
    int getNumBlocks();
    int getTopSpace();

    // Mutators
    void setWidth(int newWidth);
    void setHeight(int newHeight);
    void setNumBlocks(int newAmount);
    void setGameEnded(int value);

    // Method to draw the grid
    void drawGrid();

    // Methods to move the cursor
    void moveCursor();
    void eraseCursor(int col, int row);
    int updateCursorPosition();

    // Methods to delete blocks of same color at cursor location.
    void deleteSameColorNeighbors();
    void updateBlocksPositions(int mostLeftCol, int mostDownRow);
    void updateRows(int mostLeftCol, int mostDownRow);
    void updateColumns(int mostLeftCol, int mostDownRow);

    // Methods to save and load the game.
    void saveGame();
    void loadGame();

    // Methods to save and load best score.
    void saveScore();
    void loadScore();

    // Method to check for the different end conditions.
    void checkEndCondition();
    int anyPossibilityLeft();

    // Method to check if the gameEnded variable indicates 1.
    bool hasEnded();
};

class Menu
{
public:
    int selectedOption;
    int numOptions;

    Menu(int selectedOpt = 0, int numOpts = 4);

    // Method to scroll down the menu.
    void goDownMenu();

    // Method to draw the menu on the screen.
    void drawMenu();
};