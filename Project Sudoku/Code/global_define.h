#ifndef _STATE_H_
#define _STATE_H_
// The custom definition for global using

// The difficulty of the sudoku
enum difficulty { EASY, NORMAL, TOUGH };

// the state of the game
enum states { Menu, Game, Stat };

// the record of the game
struct Record {
    int run[3];
    unsigned long time[3];
};

#endif