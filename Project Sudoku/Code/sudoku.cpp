#include "sudoku.h"

#include <Arduino.h>
using namespace std;

Sudoku::Sudoku(difficulty level) { this->level = level; }

// generate a new sudoku game
void Sudoku::newGame(difficulty level) {
    this->level = level;
    time = 0;
    // initialize the grid
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            this->grid[i][i] = 0;
        }
    }

    // clean the grid array
    initiate_grid();
    // shuffle the number
    getRandomNumberList();
    // shuffle the who grids
    getRandomSpaceList();
    // get a complete sudoku solution
    solver();
    // store the solution
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            solution[i][j] = this->grid[i][j];
        }
    }
    // dig out grids
    generator();
}

// clean the array
void Sudoku::initiate_grid() {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            grid[i][j] = 0;
}

// check whether the number in this grid is valid
bool Sudoku::isExist(int row, int colum, int num) {
    for (int i = 0; i < 9; i++) {
        if (this->grid[row][i] == num) {
            return true;
        }
    }
    for (int i = 0; i < 9; i++) {
        if (this->grid[i][colum] == num) {
            return true;
        }
    }
    int square_row = row / 3;
    int square_colum = colum / 3;
    for (int i = square_row * 3; i < (square_row + 1) * 3; i++) {
        for (int j = square_colum * 3; j < (square_colum + 1) * 3; j++) {
            if (this->grid[i][j] == num) {
                return true;
            }
        }
    }
    return false;
}

// shuffle the number 1 to 9
void Sudoku::getRandomNumberList() {
    for (int i = 0; i < 9; i++) {
        NumberList[i] = i + 1;
    }
    for (int i = 8; i >= 0; i--) {
        int randomIndex = random(1, i + 1); // 1 to 9
        int tmp = NumberList[randomIndex];
        NumberList[randomIndex] = NumberList[i];
        NumberList[i] = tmp;
    }
}

// shuffle the whole space
void Sudoku::getRandomSpaceList() {
    for (int i = 0; i < 81; i++) {
        SpaceList[i] = i;
    }

    for (int i = 80; i >= 0; i--) {
        int randomIndex = random(1, i + 1);
        int tmp = SpaceList[randomIndex];
        SpaceList[randomIndex] = SpaceList[i];
        SpaceList[i] = tmp;
    }
}

// find the first empty grid
bool Sudoku::findEmpty(int &row, int &colum) {
    for (row = 0; row < 9; row++) {
        for (colum = 0; colum < 9; colum++) {
            if (this->grid[row][colum] == 0) {
                return true;
            }
        }
    }
    return false;
}

// generate a sudoku solution
bool Sudoku::solver() {
    this->mode = 1;
    isFound = 0;
    dfs_find_solution();
    isFound = 0;
}

// use backtracking to generate the solution
void Sudoku::dfs_find_solution() {
    int row, colum;
    // mode 1 generate a game
    if (this->mode == 1) {
        if (isFound)
            return;
        if (!findEmpty(row, colum)) {
            // a solution is found
            isFound = 1;
            return;
        }
    }
    // mode 2 generate puzzles
    if (this->mode == 2) {
        if (puzzleNum > 1)
            // if there are multiple solutions,
            // it fails, and we will go other ways
            return;
        if (!findEmpty(row, colum)) {
            // find one solution
            puzzleNum++;
            return;
        }
    }

    // randomly generate solution
    for (int i = 0; i < 9; i++) {
        if (!isExist(row, colum, NumberList[i])) {
            this->grid[row][colum] = NumberList[i];
            dfs_find_solution();
            if (isFound)
                return;
            this->grid[row][colum] = 0;
        }
    }
}

// When the user touches the tips button,
// a solution of one grid will be shown on the screen
void Sudoku::getTips(int &grid_y, int &grid_x, int &num) {
    int randNumber = random(1, this->leftNum + 1);
    int count = 0;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            if (this->grid[i][j] == 0) {
                count++;
                if (count == randNumber) {
                    fillGrid(i, j, this->solution[i][j]);
                    grid_y = i;
                    grid_x = j;
                    num = solution[i][j];
                    return;
                }
            }
        }
}

// generate puzzles
void Sudoku::generator() {
    // easy -> 40 digits are preserved
    // normal -> 35 digits are preserved
    // tough -> 30 digits are preserved
    int preservedNum;
    if (this->level == TOUGH)
        preservedNum = 30;
    if (this->level == NORMAL)
        preservedNum = 35;
    if (this->level == EASY)
        preservedNum = 40;
    this->leftNum = 81 - preservedNum;

    // use count to store the left digits on the game board
    int count = 81;
    for (int i = 0; i < 81 && count > preservedNum; i++) {
        int x = (this->SpaceList[i]) / 9;
        int y = (this->SpaceList[i]) % 9;
        int temp = this->grid[x][y];
        this->grid[x][y] = 0;
        this->mode = 2;
        this->puzzleNum = 0;
        dfs_find_solution();
        if (this->puzzleNum == 1) {
            count--;
        } else {
            // if the digit fails to be digged out
            // the grid's value will be back
            this->grid[x][y] = temp;
        }
    }
}
// get the current value of the grid
int Sudoku::getValue(int x, int y) { return this->grid[x][y]; }

// get the correct solution of the grid
int Sudoku::getSolution(int x, int y) { return this->solution[x][y]; }

// if there's value on the grid, then it is not modifiable
bool Sudoku::isModifiable(int x, int y) {
    if (this->grid[x][y] == 0) {
        return true;
    }
    return false;
}

// fill in the grid
void Sudoku::fillGrid(int x, int y, int num) { this->grid[x][y] = num; }

// set resumed grid
void Sudoku::setResumeGrid(int x, int y, int num) { this->grid[x][y] = num; }

// set resumed solution
void Sudoku::setResumeSolution(int x, int y, int num) {
    this->solution[x][y] = num;
}

// set resumed level
void Sudoku::setResumeLevel(int x) {
    if (x == 0) {
        level = EASY;
    } else if (x == 1) {
        level = NORMAL;
    } else {
        level = TOUGH;
    }
}

// A new digit is filled in, the left number will minus 1
bool Sudoku::countResult() {
    this->leftNum--;
    if (this->leftNum == 0)
        return true;
    else
        return false;
}

// set time
void Sudoku::setTime(unsigned long time) { this->time = time; }

// get time
unsigned long Sudoku::getTime() { return time; }

// get level
int Sudoku::getLevel() { return level; }

// set level
void Sudoku::setLevel(int l) { level = l; }