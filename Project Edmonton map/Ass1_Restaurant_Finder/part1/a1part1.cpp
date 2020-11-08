/*
# -----------------------------------------
# Name: Yifan Zhang, Yuxi Chen
# ID: 1635595, 1649014
# CMPUT 275, Winter 2020
# Assignment #1: Restaurant Finder part 1
# -----------------------------------------
*/

#include <Arduino.h>
// core graphics library (written by Adafruit)
#include <Adafruit_GFX.h>
// Hardware-specific graphics library for MCU Friend 3.5" TFT LCD shield
#include <MCUFRIEND_kbv.h>
// LCD and SD card will communicate using the Serial Peripheral Interface (SPI)
// e.g., SPI is used to display images stored on the SD card
#include <SPI.h>
// needed for reading/writing to SD card
#include "lcd_image.h"
#include <SD.h>
#include <TouchScreen.h>

MCUFRIEND_kbv tft;

#define DISPLAY_WIDTH 480
#define DISPLAY_HEIGHT 320
#define YEG_SIZE 2048

lcd_image_t yegImage = {"yeg-big.lcd", YEG_SIZE, YEG_SIZE};

#define JOY_CENTER 512
#define JOY_DEADZONE 64
#define LOW_SPEED_ZONE 256 // the zone in which the cursor is low speed
#define LOW_SPEED 3
#define HIGH_SPEED 9

#define CURSOR_SIZE 9
#define NUM_RESTAURANTS 1066

#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 940
#define TS_MAXY 920

// thresholds to determine if there was a touch
#define MINPRESSURE 10
#define MAXPRESSURE 1000

// These constants are for the 2048 by 2048 map .
#define MAP_WIDTH 2048
#define MAP_HEIGHT 2048
#define LAT_NORTH 5361858ll
#define LAT_SOUTH 5340953ll
#define LON_WEST -11368652ll
#define LON_EAST -11333496ll

#define LIST_NUM 21 // the number of restaurant shown on the screen

#define SD_CS 10
#define JOY_VERT A9  // should connect A9 to pin VRx
#define JOY_HORIZ A8 // should connect A8 to pin VRy
#define JOY_SEL 53

// Because in x direction the cursor should be in the range
// [0,DISPLAY_WIDTH-60), so we need to minus 1 on the upper bound of the
// center of cursor
#define X_UPPERBOUND DISPLAY_WIDTH - 60 - CURSOR_SIZE / 2 - 1
#define X_LOWERBOUND CURSOR_SIZE / 2

// Because in y direction the cursor should be in the range
// [0,DISPLAY_HEIGHT), so we need to minus 1 on the upper bound of the
// center of cursor
#define Y_UPPERBOUND DISPLAY_HEIGHT - CURSOR_SIZE / 2 - 1
#define Y_LOWERBOUND CURSOR_SIZE / 2

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
Sd2Card card; // different than SD

int cursorX, cursorY;  // the cursor position on the display
uint32_t blockNum = 0; // the block number of the SD card

struct restaurant {
    int32_t lat;
    int32_t lon;
    uint8_t rating; // from 0 to 10
    char name[55];
} restBlock[8]; // a global struct array to store a block of restaurants

struct RestDist {
    uint16_t index; // index of restaurant from 0 to NUM_RESTAURANTS -1
    uint16_t dist;  // Manhatten distance to cursor position
};

// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);
// forward declaration for redrawing the map
void redrawMap(int lastLocationX, int lastLocationY);

void setup() {
    /*
        Description: Initialize serial port and the display
    */
    init();
    Serial.begin(9600);
    pinMode(JOY_SEL, INPUT_PULLUP);
    uint16_t ID = tft.readID(); // read ID from display
    Serial.print("ID = 0x");
    Serial.println(ID, HEX);
    if (ID == 0xD3D3)
        ID = 0x9481; // write-only shield
    // must come before SD.begin() ...
    tft.begin(ID); // LCD gets ready to work
    Serial.print("Initializing SD card...");
    if (!SD.begin(SD_CS)) {
        Serial.println("failed! Is it inserted properly?");
        while (true) {
        }
    }
    Serial.println("OK!");
    if (!card.init(SPI_HALF_SPEED, SD_CS)) {
        Serial.println("failed! Is the card inserted properly?");
        while (true) {
        }
    } else {
        Serial.println("OK!");
    }
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    // draws the centre of the Edmonton map, leaving the rightmost 60 columns
    // black
    int yegMiddleX = YEG_SIZE / 2 - (DISPLAY_WIDTH - 60) / 2;
    int yegMiddleY = YEG_SIZE / 2 - DISPLAY_HEIGHT / 2;
    lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY, 0, 0,
                   DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

    // initial cursor position is the middle of the screen
    cursorX = (DISPLAY_WIDTH - 60) / 2;
    cursorY = DISPLAY_HEIGHT / 2;

    redrawCursor(TFT_RED);
}

// These functions convert between x/y map position and lat /lon
// (and vice versa .)
int32_t x_to_lon(int16_t x) {
    /*
        Description: convert between x map position and longitude
        Argument:
            x(int16_t): the x map position
        Return:
            lon(int32_t): the longitude
    */
    return map(x, 0, MAP_WIDTH, LON_WEST, LON_EAST);
}

int32_t y_to_lat(int16_t y) {
    /*
        Description: convert between y map position and latitude
        Argument:
            y(int16_t): the y map position
        Return:
            lat(int32_t): the latitude
    */
    return map(y, 0, MAP_HEIGHT, LAT_NORTH, LAT_SOUTH);
}

int16_t lon_to_x(int32_t lon) {
    /*
        Description: convert between longitude and x map position
        Argument:
            lon(int32_t): the longitude
        Return:
            x(int16_t): the x map position
    */
    return map(lon, LON_WEST, LON_EAST, 0, MAP_WIDTH);
}

int16_t lat_to_y(int32_t lat) {
    /*
        Description: convert between latitude and y map position
        Argument:
            lat(int32_t): the latitude
        Return:
            y(int16_t): the y map position
    */
    return map(lat, LAT_NORTH, LAT_SOUTH, 0, MAP_HEIGHT);
}

void redrawCursor(uint16_t colour) {
    /*
        Description: Draw the cursor by the given colour
    */
    tft.fillRect(cursorX - CURSOR_SIZE / 2, cursorY - CURSOR_SIZE / 2,
                 CURSOR_SIZE, CURSOR_SIZE, colour);
}

void redrawMap(int lastLocationX, int lastLocationY, int CurrentMapX,
               int CurrentMapY) {
    /*
        Description: Draw the small map of the position of the cursor last time

        Arguments:
            lastLocationX(int): x coordinate of the cursor location last time
            lastLocationY(int): y coordinate of the cursor location last time
    */
    lcd_image_draw(&yegImage, &tft,
                   CurrentMapX + lastLocationX - CURSOR_SIZE / 2,
                   CurrentMapY + lastLocationY - CURSOR_SIZE / 2,
                   lastLocationX - CURSOR_SIZE / 2,
                   lastLocationY - CURSOR_SIZE / 2, CURSOR_SIZE, CURSOR_SIZE);
}

void getRestaurantFast(int restIndex, restaurant *restPtr) {
    /*
        Description: find the restaurant in terms of the index and store it to
            the pointer *restPtr

        Arguments:
            restIndex(int): the index of restaurant
            *restPtr(restaurant): the pointer of restaurant list
    */

    // if the value of blockNum doesn't change
    // we can find the restaurant directly
    if (blockNum == REST_START_BLOCK + restIndex / 8) {
        *restPtr = restBlock[restIndex % 8];
        return;
    }
    blockNum = REST_START_BLOCK + restIndex / 8;
    while (!card.readBlock(blockNum, (uint8_t *)restBlock)) {
        Serial.println("Read blocks failed, trying again.");
    }
    *restPtr = restBlock[restIndex % 8];
}

void get_distance(int x, int y, RestDist *rest_dist) {
    /*
        Description: get indices and Manhattan distances of all restaurants
            store them to a pointer which points to rest_dist

        Arguments:
            x(int): the x coordinate of cursor
            y(int): the y coordinate of cursor
            *rest_dist(RestDist): the pointer to the rest_dist
    */
    restaurant rest;
    for (int i = 0; i < NUM_RESTAURANTS; ++i) {
        getRestaurantFast(i, &rest);
        rest_dist[i].index = i;

        // the Manhattan distance is d((x1, y1),(x2, y2)) = |x1 − x2| + |y1
        // − y2| (x1,y1) is the location of the cursor (x2,y2) is the location
        // of each restaurant
        rest_dist[i].dist =
            abs(y - lat_to_y(rest.lat)) + abs(x - lon_to_x(rest.lon));
    }
}

void swap(RestDist &a, RestDist &b) {
    /*
        Description: swap 2 members in the restaurant list

        Arguments:
            &a(RestDist): the member a
            &b(RestDist): the member b
    */
    int t;
    /*swap(a.index, b.index);
    swap(a.dist, b.dist);*/
    t = a.index;
    a.index = b.index;
    b.index = t;
    t = a.dist;
    a.dist = b.dist;
    b.dist = t;
    return;
}

void isort(RestDist *rest_dist, int n) {
    /*
        Description: insertion sort, sort the struct array by each element's
            distance

        Arguments:
            *rest_dist(RestDist): the pointer to the restaurant list
            n(int): the number of members to sort
    */

    int i, j;
    for (i = 1; i < n; i++) {
        j = i;
        while (j > 0 && rest_dist[j - 1].dist > rest_dist[j].dist) {
            swap(rest_dist[j - 1], rest_dist[j]);
            j -= 1;
        }
    }
}

void displayText(int index, int selectedRest, RestDist *restDist) {
    /*
        Description: display the restaurant name highlighted if it is selected

        Arguments:
            index(int): the index of the restaurant
            selectedRest(int): the index of the highlighted restaurant
            *restDist(RestDist): the pointer to the list
    */

    tft.setCursor(0, 15 * index);
    if (index == selectedRest) {
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
    } else {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
    }
    restaurant r;
    getRestaurantFast(restDist[index].index, &r);
    tft.println(r.name);
}

void displayScreen(RestDist *restDist, int &selectedRest, bool &isMap) {
    /*
        Description: display restaurant names on the screen and deal with the
            joy stick movement

        Arguments:
            &isMap(bool): the flag to identity the mode
            &selectedRest(int): the index of the highlighted restaurant
            *restDist(RestDist): the pointer to the list
    */

    tft.fillScreen(0);
    tft.setTextWrap(false);
    tft.setTextSize(2);
    selectedRest = 0; // which restaurant is selected
    restaurant r;
    for (int16_t i = 0; i < 21; i++) {
        tft.setCursor(0, i * 15);
        getRestaurantFast(restDist[i].index, &r);
        if (i != selectedRest) { // not highlighted
            // white characters on black background
            tft.setTextColor(0xFFFF, 0x0000);
        } else { // highlighted
            // black characters on white background
            tft.setTextColor(0x0000, 0xFFFF);
        }
        tft.println(r.name);
    }
    // deal with the joystick movement
    // when the joystick moves up or down
    // the corresponding text will be highlighted
    int buttonVal = digitalRead(JOY_SEL);
    while (buttonVal != 0) {
        int yVal = analogRead(JOY_VERT);
        // move down
        if (yVal > JOY_CENTER + JOY_DEADZONE && selectedRest >= 0 &&
            selectedRest < LIST_NUM - 1) {
            selectedRest++;
            displayText(selectedRest - 1, selectedRest, restDist);
            displayText(selectedRest, selectedRest, restDist);
        }
        // move up
        if (yVal < JOY_CENTER - JOY_DEADZONE && selectedRest <= LIST_NUM &&
            selectedRest > 0) {
            selectedRest--;
            displayText(selectedRest, selectedRest, restDist);
            displayText(selectedRest + 1, selectedRest, restDist);
        }
        buttonVal = digitalRead(JOY_SEL);
    }

    // exit the mode 1
    isMap = true;
}

void processTouchScreen(int CurrentMapX, int CurrentMapY, RestDist *rest_dist) {
    /*
        Description: display a small dot in the location of each restaurant in
            the current view

        Arguments:
            CurrentMapX(int): the x coordinate of current map
            CurrentMapY(int): the y coordinate of current map
            *rest_dist(RestDist): the pointer to the restaurant list
    */

    // initialize the touchscreen
    TSPoint touch = ts.getPoint();
    pinMode(YP, OUTPUT);
    pinMode(XM, OUTPUT);
    if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {
    } else {
        int16_t screen_x = map(touch.y, TS_MINX, TS_MAXX, DISPLAY_WIDTH - 1, 0);
        int16_t screen_y =
            map(touch.x, TS_MINY, TS_MAXY, DISPLAY_HEIGHT - 1, 0);
        if (screen_x >= 0 && screen_x < DISPLAY_WIDTH - 60 && screen_y >= 0 &&
            screen_y < DISPLAY_HEIGHT) {
            restaurant r;
            int i = 0;
            getRestaurantFast(rest_dist[i].index, &r);
            /*
                when the Manhattan distance of the restaurant is bigger than
                DISPLAY_HEIGHT + DISPLAY_WIDTH - 60, that is the Manhattan
                distance when cursor is on the right top of the map and the
                restaurant is on the left bottom of the map, the restaurant is
                outside the view.
            */
            while (i < NUM_RESTAURANTS &&
                   rest_dist[i].dist <= DISPLAY_HEIGHT + DISPLAY_WIDTH - 60) {
                getRestaurantFast(rest_dist[i].index, &r);
                // get the relative coordinates
                int coordinate_x = lon_to_x(r.lon) - CurrentMapX;
                int coordinate_y = lat_to_y(r.lat) - CurrentMapY;
                // only show the restaurant in the map
                if (coordinate_y >= Y_LOWERBOUND &&
                    coordinate_y <= Y_UPPERBOUND &&
                    coordinate_x >= X_LOWERBOUND &&
                    coordinate_x <= X_UPPERBOUND) {
                    tft.fillCircle(coordinate_x, coordinate_y, 3, TFT_BLUE);
                }
                i++;
            }
        }
    }
}

void processMode(int &CurrentMapX, int &CurrentMapY, bool &isMap,
                 RestDist *rest_dist) {
    /*
        Description: Running different modes in terms of the joystick the mode
            changes when the button is pressed
        Arguments:
            &CurrentMapX(int): the x coordinate of current map
            &CurrentMapY(int): the y coordinate of current map
            *rest_dist(RestDist): the pointer to the restaurant list
            &isMap(bool): the flag to identity the mode
    */

    int xVal = analogRead(JOY_HORIZ);
    int yVal = analogRead(JOY_VERT);
    int buttonVal = digitalRead(JOY_SEL);

    // deal with the touchscreen
    processTouchScreen(CurrentMapX, CurrentMapY, rest_dist);

    // change mode when button pressed
    if (buttonVal == 0) {
        isMap = false;
    }

    if (isMap) {
        // Mode 0
        // store the last location of cursor
        int lastLocationX = cursorX;
        int lastLocationY = cursorY;
        int lastMapX = CurrentMapX;
        int lastMapY = CurrentMapY;
        bool isChange = false; // the flag for the map changing

        // cursor moves up or down
        if (yVal < JOY_CENTER - JOY_DEADZONE) {
            if (yVal < JOY_CENTER - LOW_SPEED_ZONE) {
                cursorY -= HIGH_SPEED;
            } else {
                cursorY -= LOW_SPEED;
            }
        } else if (yVal > JOY_CENTER + JOY_DEADZONE) {
            if (yVal > JOY_CENTER + LOW_SPEED_ZONE) {
                cursorY += HIGH_SPEED;
            } else {
                cursorY += LOW_SPEED;
            }
        }
        // cursor moves left or right
        if (xVal > JOY_CENTER + JOY_DEADZONE) {
            if (xVal > JOY_CENTER + LOW_SPEED_ZONE) {
                cursorX -= HIGH_SPEED;
            } else {
                cursorX -= LOW_SPEED;
            }
        } else if (xVal < JOY_CENTER - JOY_DEADZONE) {
            if (xVal < JOY_CENTER - LOW_SPEED_ZONE) {
                cursorX += HIGH_SPEED;
            } else {
                cursorX += LOW_SPEED;
            }
        }
        // update the map
        if (cursorX < X_LOWERBOUND) {
            isChange = true;
            CurrentMapX -= (DISPLAY_WIDTH - 60);
        }
        if (cursorX > X_UPPERBOUND) {
            isChange = true;
            CurrentMapX += (DISPLAY_WIDTH - 60);
        }
        if (cursorY < Y_LOWERBOUND) {
            isChange = true;
            CurrentMapY -= DISPLAY_HEIGHT;
        }
        if (cursorY > Y_UPPERBOUND) {
            isChange = true;
            CurrentMapY += DISPLAY_HEIGHT;
        }
        // limit the range of the map
        CurrentMapX =
            constrain(CurrentMapX, 0, (YEG_SIZE - (DISPLAY_WIDTH - 60) - 1));
        CurrentMapY =
            constrain(CurrentMapY, 0, (YEG_SIZE - DISPLAY_HEIGHT) - 1);

        // if the new location is the same as the old
        // does not change the map
        if (lastMapX == CurrentMapX && lastMapY == CurrentMapY) {
            isChange = false;
        }
        if (isChange) {
            // redraw the map
            lcd_image_draw(&yegImage, &tft, CurrentMapX, CurrentMapY, 0, 0,
                           DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);

            // get indices and Manhattan distances of all restaurants
            get_distance(cursorX + CurrentMapX, cursorY + CurrentMapY,
                         rest_dist);

            // sort restaurants by distances
            isort(rest_dist, NUM_RESTAURANTS);

            cursorX = (DISPLAY_WIDTH - 60) / 2;
            cursorY = DISPLAY_HEIGHT / 2;
            redrawCursor(TFT_RED);

        } else {

            // eliminate the cursor's track and draw the new cursor
            cursorX = constrain(cursorX, X_LOWERBOUND, X_UPPERBOUND);
            cursorY = constrain(cursorY, Y_LOWERBOUND, Y_UPPERBOUND);
            if (lastLocationX != cursorX || lastLocationY != cursorY) {
                redrawMap(lastLocationX, lastLocationY, CurrentMapX,
                          CurrentMapY);
                redrawCursor(TFT_RED);
            }
        }
    } else {
        // mode 1

        int selectedRest = 0;
        // get indices and Manhattan distances of all restaurants
        get_distance(cursorX + CurrentMapX, cursorY + CurrentMapY, rest_dist);
        // sort restaurants by distances
        isort(rest_dist, NUM_RESTAURANTS);

        // display the list of nearby restaurants
        displayScreen(rest_dist, selectedRest, isMap);
        tft.fillScreen(0); // clean the screen

        // get selected restaurant information and update the map and cursor
        restaurant r;
        getRestaurantFast(rest_dist[selectedRest].index, &r);

        CurrentMapX = lon_to_x(r.lon) - (DISPLAY_WIDTH - 60) / 2;
        CurrentMapY = lat_to_y(r.lat) - DISPLAY_HEIGHT / 2;
        CurrentMapX =
            constrain(CurrentMapX, 0, (YEG_SIZE - 1 - (DISPLAY_WIDTH - 60)));
        CurrentMapY =
            constrain(CurrentMapY, 0, (YEG_SIZE - 1 - DISPLAY_HEIGHT));
        lcd_image_draw(&yegImage, &tft, CurrentMapX, CurrentMapY, 0, 0,
                       DISPLAY_WIDTH - 60, DISPLAY_HEIGHT);
        cursorX = lon_to_x(r.lon) - CurrentMapX;
        cursorY = lat_to_y(r.lat) - CurrentMapY;
        cursorX = constrain(cursorX, Y_LOWERBOUND, X_UPPERBOUND);
        cursorY = constrain(cursorY, Y_LOWERBOUND, Y_UPPERBOUND);
        redrawCursor(TFT_RED);
    }

    delay(20);
}

int main() {
    setup();
    bool isMap = true; // the flag to identity the mode
    int yegMiddleX = YEG_SIZE / 2 - (DISPLAY_WIDTH - 60) / 2;
    int yegMiddleY = YEG_SIZE / 2 - DISPLAY_HEIGHT / 2;

    // initialize the list to show the restaurant on the map
    RestDist rest_dist[NUM_RESTAURANTS];
    get_distance(cursorX + yegMiddleX, cursorY + yegMiddleY, rest_dist);
    isort(rest_dist, NUM_RESTAURANTS);
    while (true) {
        processMode(yegMiddleX, yegMiddleY, isMap, rest_dist);
    }
    Serial.end();
    return 0;
}
