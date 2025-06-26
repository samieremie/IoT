#include <M5StickC.h>
#include <stdint.h>
#include <ctime>
#include "EEPROM.h"
#include "classes.h"

uint32_t black_color = M5.Lcd.color565(0, 0, 0);
uint32_t white_color = M5.Lcd.color565(255, 255, 255);

// Constructor of the Grid class
Grid::Grid()
{
    // Change the seed each time you generate a new random grid.
    std::srand(std::time(0));

    /** Because of the small screen too many blocks, becomes unplayable because you dont see them.
     * (you need to make the blocks smaller so that they all fit inside the screen).
     * A small amount is also not fun to play, so I made the randomness be restricted within a range.
     * the range for the width is [10, 16]
     * the range for the height is [4, 6]
     * the range for the number of different colors is [3, 5] */
    width = 16; // 10 + (rand() % 7);
    height = 6; // 4 + (rand() % 3);
    numDifferentBlocks = 3 + (rand() % 3);
    numBlocks = width * height;
    topSpace = SCREEN_HEIGHT - (height * BLOCK_HEIGHT);

    Cursor newCursor;
    cursor = newCursor;
    initializeGrid();
}

/** Method that gets called at construction of a grid.
 * It initializes the matrix and the cursor. */
void Grid::initializeGrid()
{
    // Resize the matrix to match the grid dimensions
    matrix.resize(width); // Resize the outer vector to match the number of cols
    for (int col = 0; col < width; col++)
    {
        matrix[col].resize(height); // Resize each col to match the number of rows.
    }

    // Initialize the matrix containing the rectangles.
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            // Make the new block at the specified (x, y).
            Block newBlock;
            newBlock.setBlockType(rand() % numDifferentBlocks);
            matrix[col][row] = std::make_optional(newBlock);
        }
    }

    // Initialize the total number of blocks.
    numBlocks = width * height;
    // Initialize the cursor position to be in the left bottom corner of the grid.
    colCursor = 0;
    rowCursor = height - 1;

    loadScore(); // Load the best score stored (if any).

    // Draw the initial grid on the screen with the cursor.
    drawGrid();
}

// Accessors for the private variables.
int Grid::getWidth()
{
    return width;
}

int Grid::getHeight()
{
    return height;
}

int Grid::getNumDifferentBlocks()
{
    return numDifferentBlocks;
}

int Grid::getNumBlocks()
{
    return numBlocks;
}

int Grid::getTopSpace()
{
    return topSpace;
}

int Grid::getBlockColor(int blockType)
{
    return blockColors[blockType];
}

// These are the mutators of the class.
void Grid::setWidth(int newWidth)
{
    width = newWidth;
}

void Grid::setHeight(int newHeight)
{
    height = newHeight;
}

void Grid::setNumBlocks(int newAmount)
{
    numBlocks = newAmount;
}

void Grid::setGameEnded(int value)
{
    gameEnded = value;
}

// Method to draw the grid
void Grid::drawGrid()
{
    // Reset everything by drawing the background again.
    M5.Lcd.fillScreen(black_color);

    // Draw the score and best score.
    M5.Lcd.setCursor(5, 2);
    M5.Lcd.printf("Score: %d", score);

    M5.Lcd.setCursor(100, 2);
    M5.Lcd.printf("Best: %d", bestScore);

    // Draw the matrix
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            std::optional<Block> cell = matrix[col][row];
            if (cell.has_value()) // Check if the block pointer is not the same as the empty block.
            {
                // Get the current block.
                Block curBlock = cell.value();

                // Get the color of the block.
                int curBlockType = curBlock.getBlockType();
                int curBlockColor = blockColors[curBlockType];

                // Draw the block.
                curBlock.drawBlock(col, row, curBlockColor, getTopSpace());
            }
            else // No more block at this location.
            {
                int x = col * BLOCK_WIDTH;
                int y = (row * BLOCK_HEIGHT) + getTopSpace();
                M5.Lcd.fillRect(x, y, BLOCK_WIDTH, BLOCK_HEIGHT, black_color);
            }
        }
    }

    // Draw the cursor on top of everything at the end.
    cursor.drawCursor();
}

// Main method that moves the cursor only when needed.
void Grid::moveCursor()
{
    // Get the current col and row of the cursor.
    int oldCursorRow = rowCursor;
    int oldCursorCol = colCursor;

    // Update the cursor position.
    if (updateCursorPosition() == 1) // It means the cursor has changed of position.
    {
        eraseCursor(oldCursorCol, oldCursorRow);
        cursor.drawCursor(); // Redraw the cursor at the new location.
    }
}

// Help method for moveCursor. Erases cursor (graphically) from old position.
void Grid::eraseCursor(int col, int row)
{
    // Check if there is a block at the cursor location and get the block color.
    std::optional<Block> cell = matrix[col][row];
    if (cell.has_value())
    {
        Block cell = matrix[col][row].value();
        int blockType = cell.getBlockType();
        uint32_t blockColor = blockColors[blockType];
        // Draw the block at old cursor location again (to remove the cursor)
        cell.drawBlock(col, row, blockColor, getTopSpace());
    }
    else // No more block at this location. Draw a black square.
    {
        int x = col * BLOCK_WIDTH;
        int y = row * BLOCK_HEIGHT + getTopSpace();
        M5.Lcd.fillRect(x, y, BLOCK_WIDTH, BLOCK_HEIGHT, BLACK);
    }
}

/** Help method for moveCursor. Changes the cursor position based on IMU.
 * Returns 1 if an update was made, 0 otherwise.*/
int Grid::updateCursorPosition()
{
    // Get the accelerator data.
    float acc_x = 0, acc_y = 0, acc_z = 0;
    M5.IMU.getAccelData(&acc_y, &acc_x, &acc_z);

    // Get the current grid width, and top space.
    int gridWidth = getWidth() * BLOCK_WIDTH;
    int gridTopSpace = getTopSpace();

    // The move logic.
    int cur_x = cursor.getX();
    int cur_y = cursor.getY();

    // Value that gets returned at the end. If 0, no updates were made to the current cursor position.
    int changed = 0;

    if (acc_x > MIN_TILT)
    {
        if ((cur_x + BLOCK_WIDTH) <= (gridWidth - BLOCK_WIDTH))
        {
            cur_x += BLOCK_WIDTH;
            cursor.setX(cur_x);
            colCursor += 1;
            changed = 1;
        }
    }
    else if (acc_x < -MIN_TILT)
    {
        if ((cur_x - BLOCK_WIDTH) >= 0)
        {
            cur_x -= BLOCK_WIDTH;
            cursor.setX(cur_x);
            colCursor -= 1;
            changed = 1;
        }
    }

    if (acc_y > MIN_TILT)
    {
        if ((cur_y + BLOCK_HEIGHT) < SCREEN_HEIGHT)
        {
            cur_y += BLOCK_HEIGHT;
            cursor.setY(cur_y);
            rowCursor += 1;
            changed = 1;
        }
    }
    else if (acc_y < -MIN_TILT)
    {
        if ((cur_y - BLOCK_HEIGHT) >= (0 + gridTopSpace))
        {
            cur_y -= BLOCK_HEIGHT;
            cursor.setY(cur_y);
            rowCursor -= 1;
            changed = 1;
        }
    }
    return changed; // 0 if cursor position didn't change, 1 if it changed.
}

/** Get the same color neighbors of the block at the current position of the cursor.
 * It deletes the block and the neighbors if 2 or more exist.
 * Function gets called when A button is pressed.
 * A BFS algorithm that returns a list of positions of the neighbors with the same color.*/
void Grid::deleteSameColorNeighbors()
{
    // Start with current position of the cursor.
    int startCol = colCursor;
    int startRow = rowCursor;
    // Get the color of the start position.
    std::optional<Block> start = matrix[startCol][startRow];

    // Check if there is a block at current position.
    if (!start.has_value())
    {
        return;
    }

    int startType = start.value().getBlockType(); // Used for comparison later on.

    // Make a matrix to keep track of visited elements so far.
    std::vector<std::vector<bool>> visited;
    visited.resize(width); // Resize the outer vector to match the number of cols
    for (int col = 0; col < width; col++)
    {
        visited[col].resize(height, false); // Resize each col to match the number of rows.
    }

    // The queue where we push the blocks positions to visit.
    std::queue<std::pair<int, int>> blocksQueue;
    blocksQueue.push(std::make_pair(startCol, startRow)); // Push the start position.
    visited[startCol][startRow] = true;                   // Mark it as visited

    // Vector where we build our result.
    std::vector<std::pair<int, int>> result;
    result.push_back(std::make_pair(startCol, startRow)); // Push the start position.

    // Offsets used to compute the neighbors.
    int dCol[] = {1, -1, 0, 0};
    int dRow[] = {0, 0, 1, -1};

    // The actual BFS
    while (!blocksQueue.empty())
    {
        // Pop the first element from the queue.
        std::pair<int, int> curElement = blocksQueue.front();
        blocksQueue.pop();

        int curCol = curElement.first;
        int curRow = curElement.second;

        // Compute the neighbors of curElement.
        for (int i = 0; i < 4; i++)
        {
            int neighborCol = curCol + dCol[i];
            int neighborRow = curRow + dRow[i];

            // Check if the neighbor is within boundaries.
            if (neighborRow >= 0 && neighborRow < height &&
                neighborCol >= 0 && neighborCol < width)
            {
                // Check if the neighbor is already visited.
                if (!visited[neighborCol][neighborRow])
                {
                    // Mark the neighbor as visited.
                    visited[neighborCol][neighborRow] = true;

                    // Check if the neighbor block exists in our grid.
                    if (matrix[neighborCol][neighborRow].has_value())
                    {
                        // Check if it matches the startColor.
                        Block curBlock = matrix[neighborCol][neighborRow].value();
                        int curBlockType = curBlock.getBlockType();

                        // If the types match, then only we add the block position to the queue.
                        if (startType == curBlockType)
                        {
                            // Add the neighbor to the queue.
                            blocksQueue.push(std::make_pair(neighborCol, neighborRow));
                            // Add current neighbor to the result.
                            result.push_back(std::make_pair(neighborCol, neighborRow));
                        }
                    }
                }
            }
        }
    }

    int mostLeftCol = width - 1;
    int mostDownRow = 0;
    if (result.size() >= 2)
    {
        // Delete the elements from the positions in result.
        for (int i = 0; i < result.size(); i++)
        {
            int col = result[i].first;
            int row = result[i].second;
            matrix[col][row] = std::nullopt; // Remove the block.
            numBlocks -= 1;                  // Update the number of blocks left.
            score += 1;                      // Add one to the current game score.

            // Update the mostLeftCol and mostDownRow.
            if (mostLeftCol > col)
            {
                mostLeftCol = col;
            }
            if (mostDownRow < row)
            {
                mostDownRow = row;
            }
        }
        updateBlocksPositions(mostLeftCol, mostDownRow);
    }
}

// Method that updates the blocks positions in the given rectangle range.
void Grid::updateBlocksPositions(int mostLeftCol, int mostDownRow)
{
    // Make the blocks from each column "fall" down.
    updateRows(mostLeftCol, mostDownRow);
    // Put the empty columns to the back.
    updateColumns(mostLeftCol, mostDownRow);

    // Check for the end conditions.
    checkEndCondition();

    // Redraw the matrix at the end.
    drawGrid();
}

// Help method for updateBlocksPositions.
void Grid::updateRows(int mostLeftCol, int mostDownRow)
{
    // Make the blocks from each column "fall" down.
    if (mostDownRow > 0) // Do only if mostDownRow is not 0.
    {
        for (int col = mostLeftCol; col < width; col++)
        {
            // Pointer that holds the new position of the next block in the column.
            int newRow = mostDownRow;
            // Pointer that goes through all elements from the column (starting at mostDownRow)
            // If there is a block at the position in the column changes his place to newPosition pointer.
            int curRow = mostDownRow;

            while (curRow >= 0) // While curElement pointer has not reached the top of the column.
            {
                if (matrix[col][curRow].has_value()) // If there is a block at curElement.
                {
                    // Swap curRow with newRow if the pointers are not on the same element.
                    if (newRow != curRow)
                    {
                        Block curBlock = matrix[col][curRow].value(); // Change the block's coordinates.
                        std::swap(matrix[col][newRow], matrix[col][curRow]);
                    }

                    // Go to next row.
                    newRow -= 1;
                    curRow -= 1;
                }
                else // There is no block at current position.
                {
                    // Skip the empty position by updating only curRow pointer.
                    curRow -= 1;
                }
            }
        }
    }
}

// Help method for updateBlockPositions
void Grid::updateColumns(int mostLeftCol, int mostDownRow)
{
    /** Move the empty columns at the right side of the matrix.
     * Only do this if the mostDownRow is the deepest row.
     * If the passed mostDownRow is not the deepest,
     * then we didn't get an empty column this time, so we skip this step.
     * */
    if (mostDownRow == (height - 1))
    {
        // Almost same algorithm as before just going through one row instead of a column.
        int newColumn = mostLeftCol;
        int curColumn = mostLeftCol;

        while (curColumn <= width - 1) // While curColumn has not reached the last column in the row.
        {
            // If current column is not empty.
            if (matrix[curColumn][mostDownRow].has_value())
            {
                if (curColumn != newColumn)
                {
                    std::swap(matrix[curColumn], matrix[newColumn]);
                }
                // Go to next column.
                curColumn += 1;
                newColumn += 1;
            }
            else // Case where column is empty.
            {
                // Skip the column.
                curColumn += 1;
            }
        }
    }
}

// Method to save a game.
void Grid::saveGame()
{
    int address = 0;

    // Make the first byte 1 to indicate there is a save.
    EEPROM.writeByte(address, (u_int8_t)1);
    address++;

    // First save current score.
    EEPROM.writeInt(address, score);
    address += sizeof(int);

    // Then skip the best score variables in memory.
    address++;              // Skip hasBestScore.
    address += sizeof(int); // Skip best Score.

    // Save the grid dimensions.
    EEPROM.writeInt(address, width);
    address += sizeof(int);
    EEPROM.writeInt(address, height);
    address += sizeof(int);

    // Save the game matrix. Save the block types. 5 is equal to no-block.
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            std::optional<Block> cell = matrix[col][row];
            if (cell.has_value()) // If there is a block.
            {
                Block curBlock = cell.value();
                int typeBlock = curBlock.getBlockType();
                EEPROM.writeInt(address, typeBlock);
                address += sizeof(int);
            }
            else // If there is no block.
            {
                int empty = 5;
                EEPROM.writeInt(address, empty);
                address += sizeof(int);
            }
        }
    }

    // Save the numBlocks left in the current game.
    EEPROM.writeInt(address, numBlocks);
    address += sizeof(int);

    // Save the number of different blocks.
    EEPROM.writeInt(address, numDifferentBlocks);
    address += sizeof(int);

    EEPROM.commit();
}

// Method to load a saved game only if one exists.
void Grid::loadGame()
{
    int address = 0;
    // Check if there is any save.
    if (EEPROM.readByte(address) == (uint8_t)0)
    {
        return; // There is no save yet.
    }

    // Load the score first and print it.
    address++;
    // Load the score in score of the grid class.
    score = EEPROM.readInt(address);
    address += sizeof(int);

    // Load best score and print it.
    address++;
    bestScore = EEPROM.readInt(address);
    address += sizeof(int);
    M5.Lcd.setCursor(100, 2);
    M5.Lcd.printf("Best: %d", bestScore);

    // Load the grid dimensions.
    width = EEPROM.readInt(address);
    address += sizeof(int);
    height = EEPROM.readInt(address);
    address += sizeof(int);

    // Change the top space according to new loaded height.
    topSpace = SCREEN_HEIGHT - (height * BLOCK_HEIGHT);

    // Load the game matrix. Load the block types. 5 is equal to no-block.
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            std::uint8_t blockType = EEPROM.readByte(address);
            address++;

            if (blockType != 5) // If there is a block.
            {
                Block curBlock = matrix[col][row].value();
                int typeBlockInt = static_cast<int>(blockType);
                curBlock.setBlockType(typeBlockInt);
            }
            else // If there is no block.
            {
                matrix[col][row] = std::nullopt;
            }
        }
    }

    // Change the top space according to new loaded height.
    topSpace = SCREEN_HEIGHT - (height * BLOCK_HEIGHT);

    // Load the number of blocks remaining.
    numBlocks = EEPROM.readInt(address);
    address += sizeof(int);

    // Load the number of different blocks.
    numDifferentBlocks = EEPROM.readInt(address);
    address += sizeof(int);

    // Redraw the game.
    drawGrid();
}

// Method to load the best score that is stored in memory at the beginning of the game.
void Grid::loadScore()
{
    int address = 0;

    address++;              // Skip byte that tells if there is a save.
    address += sizeof(int); // Skip int that holds score.

    // Check if there is a best score saved yet.
    if (EEPROM.readByte(address) == (uint8_t)0) // There is no best score.
    {
        bestScore = 0;
    }
    else // There is a best score.
    {
        address++;
        bestScore = EEPROM.readInt(address); // Load the best score so far.
    }
}

// Method to save the current score as the best score.
void Grid::saveScore()
{
    int address = 0;
    address += sizeof(int); // address of score.

    // Make the byte hasBestScore 1 to indicate there is a best score saved.
    std::uint8_t hasBestScore = 1;
    EEPROM.writeByte(address, hasBestScore);
    address++;
    // Write the new best score.
    EEPROM.writeInt(address, score);
}

// Method to check if any of the end conditions has been met.
void Grid::checkEndCondition()
{
    if (anyPossibilityLeft() == 0)
    {
        gameEnded = 1; // End the game if no possibility left.
        M5.Lcd.fillScreen(black_color);
        M5.Lcd.setCursor(30, 35, 4);
        M5.Lcd.printf("You Lost");
        M5.Lcd.setCursor(5, 2, 1); // Set the cursor back to normal size.
        delay(5000);               // Wait 5 seconds before continuing execution.
    }
    if (numBlocks == 0)
    {
        gameEnded = 1;
        M5.Lcd.fillScreen(black_color);
        M5.Lcd.setCursor(30, 35, 4);
        M5.Lcd.printf("You Won");
        M5.Lcd.setCursor(5, 2, 1); // Set the cursor back to normal size.
        delay(5000);               // Wait 5 seconds before continuing execution.
    }
}

// Method to check for a pair of the same block type next to each other.
int Grid::anyPossibilityLeft()
{
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            if (matrix[col][row].has_value())
            {
                Block curBlock = matrix[col][row].value();
                int typeCurBlock = curBlock.getBlockType();

                // Check if there is any neighbor of the type of curBlock.
                // Offsets used to compute the neighbors.
                int dCol[] = {1, -1, 0, 0};
                int dRow[] = {0, 0, 1, -1};

                for (int i = 0; i < 4; i++)
                {
                    int newCol = col + dCol[i];
                    int newRow = row + dRow[i];

                    // Check if neighbor is valid (within bounds).
                    if (newRow >= 0 && newRow < height &&
                        newCol >= 0 && newCol < width)
                    {
                        if (matrix[newCol][newRow].has_value())
                        {
                            Block neighborBlock = matrix[newCol][newRow].value();
                            if (neighborBlock.getBlockType() == typeCurBlock)
                            {
                                return 1; // We found a pair no need to compute further.
                            }
                        }
                    }
                }
            }
        }
    }
    return 0; // If we arrive here we didn't find any pair left.
}

// Method that checks if the current game has ended.
bool Grid::hasEnded()
{
    if (gameEnded == 1) // The game has ended.
    {
        // Check if the current score is the new best score and save it.
        if (score > bestScore)
        {
            saveScore();
        }
        return true;
    }
    else
    {
        return false;
    }
}

// Constructor of the Cursor class
Cursor::Cursor(int x, int y)
{
    setX(x);
    setY(y);
}

// Accessors
int Cursor::getX()
{
    return x_coord;
}

int Cursor::getY()
{
    return y_coord;
}

// Mutators
void Cursor::setX(int newX)
{
    x_coord = newX;
}

void Cursor::setY(int newY)
{
    y_coord = newY;
}

// Method to draw the cursor at current cursor location.
void Cursor::drawCursor()
{
    M5.Lcd.drawRect(getX(), getY(), BLOCK_WIDTH, BLOCK_HEIGHT, white_color);
}

// Constructor of the Block class
Block::Block()
{
    ;
}

// Accessors
int Block::getBlockType()
{
    return blockType;
}

// Mutators

void Block::setBlockType(int newBlockType)
{
    blockType = newBlockType;
}

// Method to draw a block
void Block::drawBlock(int col, int row, int color, int topSpace)
{
    int x = col * BLOCK_WIDTH;
    int y = (row * BLOCK_HEIGHT) + topSpace;
    M5.Lcd.fillRect(x, y, width, height, color);
}

// Class menu constructor.
Menu::Menu(int selectedOpt, int numOpts)
{
    selectedOption = selectedOpt;
    numOptions = numOpts;
}

// Method that selects the next option.
void Menu::goDownMenu()
{
    selectedOption = (selectedOption + 1) % numOptions;
}

void Menu::drawMenu()
{
    // Fill the background screen in black.
    M5.Lcd.fillScreen(black_color);

    int firstOption = 15;
    int secondOption = 30;
    int thirdOption = 45;
    int fourthOption = 60;

    // Draw the arrow that shows which option is selected.
    int arrowPosition = 0;
    switch (selectedOption)
    {
    case 0:

        arrowPosition = firstOption;
        break;

    case 1:

        arrowPosition = secondOption;
        break;

    case 2:

        arrowPosition = thirdOption;
        break;

    case 3:

        arrowPosition = fourthOption;
        break;
    }

    // Draw the selection arrow.
    M5.Lcd.setCursor(40, arrowPosition);
    M5.Lcd.printf(">");

    // Draw all options.
    M5.Lcd.setCursor(50, firstOption);
    M5.Lcd.printf("return");

    M5.Lcd.setCursor(50, secondOption);
    M5.Lcd.printf("save");

    M5.Lcd.setCursor(50, thirdOption);
    M5.Lcd.printf("load");

    M5.Lcd.setCursor(50, fourthOption);
    M5.Lcd.printf("next level");
}