// -----------------------------------------
// Name: Yifan Zhang, Yuxi Chen
// ID: 1635595, 1649014
// CMPUT 275, Winter 2020
// Assignment #1: Restaurant Finder part 2
// -----------------------------------------
#include "lcd_image.h"
#include "restaurant.h"
#include "yegmap.h"
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <MCUFRIEND_kbv.h>
#include <SD.h>
#include <SPI.h>
#include <TouchScreen.h>

// SD_CS pin for SD card reader
#define SD_CS 10

// joystick pins
#define JOY_VERT_ANALOG A9
#define JOY_HORIZ_ANALOG A8
#define JOY_SEL 53

// width/height of the display when rotated horizontally
#define TFT_WIDTH 480
#define TFT_HEIGHT 320

// layout of display for this assignment,
#define RATING_SIZE 60
#define DISP_WIDTH (TFT_WIDTH - RATING_SIZE)
#define DISP_HEIGHT TFT_HEIGHT

// layout of the buttons
#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 160
#define LINE_WIDTH 3

// constants for the joystick
#define JOY_DEADZONE 64
#define JOY_CENTRE 512
#define JOY_STEPS_PER_PIXEL 64

// touch screen pins, obtained from the documentation
#define YP A3 // must be an analog pin, use "An" notation!
#define XM A2 // must be an analog pin, use "An" notation!
#define YM 9  // can be a digital pin
#define XP 8  // can be a digital pin

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 100
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// thresholds to determine if there was a touch
#define MINPRESSURE 100
#define MAXPRESSURE 1000

// Cursor size. For best results, use an odd number.
#define CURSOR_SIZE 9

// number of restaurants to display
#define REST_DISP_NUM 21

// ********** BEGIN GLOBAL VARIABLES ************
MCUFRIEND_kbv tft;
Sd2Card card;

// A multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings.
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// The currently selected restaurant, if we are in mode 1.
int selectedRest;

// The current page in mode1.
int pageNum;

// The current star number of the star button.
int star = 1;

// The number of restaurants with the right number of stars.
int restNum = 0;

// The mode of screen.
enum DisplayMode { MAP, MENU } displayMode;

// The mode of sort.
enum SortMode { ISORT, QSORT, BOTH } sortMode = QSORT;

// the current map view and the previous one from last cursor movement
MapView curView, preView;

RestDist restaurants[NUM_RESTAURANTS];

lcd_image_t edmontonBig = {"yeg-big.lcd", MAPWIDTH, MAPHEIGHT};

// The cache of 8 restaurants for the getRestaurant function.
RestCache cache;

// ************ END GLOBAL VARIABLES ***************

void beginMode0();
void beginMode1();
void buttonDisplay();
void sortDisplay();
void starDisplay();

void setup() {
    init();

    Serial.begin(9600);

    // joystick button initialization
    pinMode(JOY_SEL, INPUT_PULLUP);

    // tft display initialization
    uint16_t ID = tft.readID();
    tft.begin(ID);
    tft.setRotation(1);
    tft.setTextWrap(false);

    // now initialize the SD card in both modes
    Serial.println("Initializing SD card...");

    // Initialize for reading through the FAT filesystem
    // (required for lcd_image drawing function).
    if (!SD.begin(SD_CS)) {
        Serial.println("failed!");
        Serial.println("Is the card inserted properly?");
        while (true) {
        }
    }

    // Also initialize the SD card for raw reads.
    Serial.println("Initializing SPI communication for raw reads...");
    if (!card.init(SPI_HALF_SPEED, SD_CS)) {
        Serial.println("failed!");
        while (true) {
        }
    }

    Serial.println("OK!");

    // initial cursor position is the centre of the screen
    curView.cursorX = DISP_WIDTH / 2;
    curView.cursorY = DISP_HEIGHT / 2;

    // initial map position is the middle of Edmonton
    curView.mapX = ((MAPWIDTH / DISP_WIDTH) / 2) * DISP_WIDTH;
    curView.mapY = ((MAPHEIGHT / DISP_HEIGHT) / 2) * DISP_HEIGHT;

    // This ensures the first getRestaurant() will load the block as all blocks
    // will start at REST_START_BLOCK, which is 4000000.
    cache.cachedBlock = 0;

    // will draw the initial map screen and other stuff on the display
    beginMode0();
}

// Draw the map patch of edmonton over the preView position, then
// draw the red cursor at the curView position.
void moveCursor() {
    lcd_image_draw(&edmontonBig, &tft,
                   preView.mapX + preView.cursorX - CURSOR_SIZE / 2,
                   preView.mapY + preView.cursorY - CURSOR_SIZE / 2,
                   preView.cursorX - CURSOR_SIZE / 2,
                   preView.cursorY - CURSOR_SIZE / 2, CURSOR_SIZE, CURSOR_SIZE);

    tft.fillRect(curView.cursorX - CURSOR_SIZE / 2,
                 curView.cursorY - CURSOR_SIZE / 2, CURSOR_SIZE, CURSOR_SIZE,
                 TFT_RED);
}

// Set the mode to 0 and draw the map and cursor according to curView
void beginMode0() {
    // Set the mode to 0 and draw the map,
    // cursor and buttons according to curView

    tft.fillRect(DISP_WIDTH, 0, RATING_SIZE, DISP_HEIGHT, TFT_BLACK);
    // draw two buttons
    buttonDisplay();
    // Draw the current part of Edmonton to the tft display.
    lcd_image_draw(&edmontonBig, &tft, curView.mapX, curView.mapY, 0, 0,
                   DISP_WIDTH, DISP_HEIGHT);
    // just the initial draw of the cursor on the map
    moveCursor();

    displayMode = MAP;
}

// Display the buttons on the right side of the screen
void buttonDisplay() {
    // Draw the two buttons and their borders
    tft.setTextColor(TFT_BLACK);
    tft.setTextSize(2);
    // The star button
    tft.fillRect(DISP_WIDTH, 0, BUTTON_WIDTH, BUTTON_HEIGHT, TFT_RED);
    tft.fillRect(DISP_WIDTH + LINE_WIDTH, LINE_WIDTH,
                 BUTTON_WIDTH - 2 * LINE_WIDTH, BUTTON_HEIGHT - 2 * LINE_WIDTH,
                 TFT_WHITE);
    starDisplay();

    // The sort button
    tft.fillRect(DISP_WIDTH, BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT,
                 TFT_GREEN);
    tft.fillRect(DISP_WIDTH + LINE_WIDTH, BUTTON_HEIGHT + LINE_WIDTH,
                 BUTTON_WIDTH - 2 * LINE_WIDTH, BUTTON_HEIGHT - 2 * LINE_WIDTH,
                 TFT_WHITE);
    sortDisplay();
}

// Display the number of stars on the star button
void starDisplay() {
    // Display the number of stars.
    star = constrain(star, 1, 5);
    // Clean the button
    tft.fillRect(DISP_WIDTH + LINE_WIDTH, LINE_WIDTH,
                 BUTTON_WIDTH - 2 * LINE_WIDTH, BUTTON_HEIGHT - 2 * LINE_WIDTH,
                 TFT_WHITE);
    // Set number in the center of the button
    tft.setCursor(445, 75);
    tft.print(star);
}

// Display the mode of sorting on the sort button
void sortDisplay() {
    // Display the sorting mode.
    const char quick[] = "QSORT";
    const char insert[] = "ISORT";
    const char both[] = "BOTH";
    // Clean the button
    tft.fillRect(DISP_WIDTH + LINE_WIDTH, BUTTON_HEIGHT + LINE_WIDTH,
                 BUTTON_WIDTH - 2 * LINE_WIDTH, BUTTON_HEIGHT - 2 * LINE_WIDTH,
                 TFT_WHITE);
    // Display the name of the mode in the center of the button
    if (sortMode == QSORT) {
        for (int i = 0; i < 5; i++) {
            tft.setCursor(445, 200 + i * 15);
            tft.print(quick[i]);
        }
    }
    if (sortMode == ISORT) {
        for (int i = 0; i < 5; i++) {
            tft.setCursor(445, 200 + i * 15);
            tft.print(insert[i]);
        }
    }
    if (sortMode == BOTH) {
        for (int i = 0; i < 4; i++) {
            tft.setCursor(445, 210 + i * 15);
            tft.print(both[i]);
        }
    }
}

// Print the i'th restaurant in the sorted list
void printRestaurant(int i) {
    // Print the i'th restaurant in the sorted list.
    restaurant r;

    // get the i'th restaurant
    getRestaurant(&r, restaurants[i + pageNum * REST_DISP_NUM].index, &card,
                  &cache);

    // Set its colour based on whether or not it is the selected restaurant.
    if (i != selectedRest) {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
    } else {
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
    }
    tft.setCursor(0, i * 15);
    tft.print(r.name);
}

// Begin mode 1 by sorting the restaurants around the cursor
// and then displaying the list.
void beginMode1() {
    /*
    Description: Sort the restaurants around the cursor
        and then display the list, and there are three modes:
        quickSort, insertionSort and both.
    */

    tft.setCursor(0, 0);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);

    // Get the RestDist information for this cursor position.
    // Sort it by the current sort mode.
    if (sortMode == QSORT) {
        getAndQuickSortRestaurants(curView, restaurants, &card, &cache, star,
                                   restNum);
    }
    if (sortMode == ISORT) {
        getAndInsertSortRestaurants(curView, restaurants, &card, &cache, star,
                                    restNum);
    }
    if (sortMode == BOTH) {
        getAndInsertSortRestaurants(curView, restaurants, &card, &cache, star,
                                    restNum);
        getAndQuickSortRestaurants(curView, restaurants, &card, &cache, star,
                                   restNum);
    }

    // Initially have the closest restaurant highlighted.
    selectedRest = 0;
    pageNum = 0;

    // Print the list of restaurants.
    for (int i = 0; i < min(REST_DISP_NUM, restNum); ++i) {
        printRestaurant(i);
    }

    displayMode = MENU;
}

// Checks if the edge was nudged and scrolls the map if it was.
void checkRedrawMap() {
    // A flag to indicate if we scrolled.
    bool scroll = false;

    // If we nudged the left or right edge, shift the map over.
    if (curView.cursorX == DISP_WIDTH - CURSOR_SIZE / 2 - 1 &&
        curView.mapX != MAPWIDTH - DISP_WIDTH) {
        curView.mapX += DISP_WIDTH;
        curView.cursorX = DISP_WIDTH / 2;
        scroll = true;
    } else if (curView.cursorX == CURSOR_SIZE / 2 && curView.mapX != 0) {
        curView.mapX -= DISP_WIDTH;
        curView.cursorX = DISP_WIDTH / 2;
        scroll = true;
    }
    // If we nudged the top or bottom edge, shift the map up or down.
    if (curView.cursorY == DISP_HEIGHT - CURSOR_SIZE / 2 - 1 &&
        curView.mapY != MAPHEIGHT - DISP_HEIGHT) {
        curView.mapY += DISP_HEIGHT;
        curView.cursorY = DISP_HEIGHT / 2;
        scroll = true;
    } else if (curView.cursorY == CURSOR_SIZE / 2 && curView.mapY != 0) {
        curView.mapY -= DISP_HEIGHT;
        curView.cursorY = DISP_HEIGHT / 2;
        scroll = true;
    }
    // If we nudged the edge, recalculate and draw the new rectangular portion
    // of Edmonton to display.
    if (scroll) {
        // Make sure we didn't scroll outside of the map.
        curView.mapX = constrain(curView.mapX, 0, MAPWIDTH - DISP_WIDTH);
        curView.mapY = constrain(curView.mapY, 0, MAPHEIGHT - DISP_HEIGHT);

        lcd_image_draw(&edmontonBig, &tft, curView.mapX, curView.mapY, 0, 0,
                       DISP_WIDTH, DISP_HEIGHT);
    }
}

// Process joystick and touchscreen input when in mode 0.
void scrollingMap() {
    /*
    Description: Process joystick and touchscreen input when in
        mode 0. The joystick has movement and click operations, and
        the touchscreen input can be buttons or drawing dots.
    */
    int v = analogRead(JOY_VERT_ANALOG);
    int h = analogRead(JOY_HORIZ_ANALOG);
    int invSelect = digitalRead(JOY_SEL);

    // A flag to indicate if the cursor moved or not.
    bool cursorMove = false;

    // If there was vertical movement, then move the cursor.
    if (abs(v - JOY_CENTRE) > JOY_DEADZONE) {
        // First move the cursor.
        int delta = (v - JOY_CENTRE) / JOY_STEPS_PER_PIXEL;
        // Clamp it so it doesn't go outside of the screen.
        curView.cursorY = constrain(curView.cursorY + delta, CURSOR_SIZE / 2,
                                    DISP_HEIGHT - CURSOR_SIZE / 2 - 1);
        // And now see if it actually moved.
        cursorMove |= (curView.cursorY != preView.cursorY);
    }

    // If there was horizontal movement, then move the cursor.
    if (abs(h - JOY_CENTRE) > JOY_DEADZONE) {
        // Ideas are the same as the previous if statement.
        int delta = -(h - JOY_CENTRE) / JOY_STEPS_PER_PIXEL;
        curView.cursorX = constrain(curView.cursorX + delta, CURSOR_SIZE / 2,
                                    DISP_WIDTH - CURSOR_SIZE / 2 - 1);
        cursorMove |= (curView.cursorX != preView.cursorX);
    }

    // If the cursor actually moved.
    if (cursorMove) {
        // Check if the map edge was nudged, and move it if so.
        checkRedrawMap();
        preView.mapX = curView.mapX;
        preView.mapY = curView.mapY;
        moveCursor();
    }

    preView = curView;

    // Did we click the joystick?
    if (invSelect == LOW) {
        beginMode1();
        displayMode = MENU;

        // Just to make sure the restaurant is not selected by accident
        // because the button was pressed too long.
        while (digitalRead(JOY_SEL) == LOW) {
            delay(10);
        }
    }

    // Check for touchscreen press and draw dots for each restaurant
    TSPoint touch = ts.getPoint();

    // Necessary to resume TFT display functions
    pinMode(YP, OUTPUT);
    pinMode(XM, OUTPUT);

    // If there was an actual touch, draw the dots
    if (touch.z >= MINPRESSURE && touch.z <= MAXPRESSURE) {
        int16_t screen_x = map(touch.y, TS_MINX, TS_MAXX, TFT_WIDTH - 1, 0);
        int16_t screen_y = map(touch.x, TS_MINY, TS_MAXY, TFT_HEIGHT - 1, 0);
        if (screen_x <= DISP_WIDTH) {
            // If the location of pressure is on the map,
            // draw the dots.
            restaurant r;

            for (int i = 0; i < NUM_RESTAURANTS; ++i) {
                getRestaurant(&r, i, &card, &cache);
                int16_t rest_x_tft = lon_to_x(r.lon) - curView.mapX,
                        rest_y_tft = lat_to_y(r.lat) - curView.mapY;
                int rest_star = max(floor((r.rating + 1) / 2), 1);

                // only draw if entire radius-3 circle will be in the map
                // display and the star fits the selector
                if (rest_x_tft >= 3 && rest_x_tft < DISP_WIDTH - 3 &&
                    rest_y_tft >= 3 && rest_y_tft < DISP_HEIGHT - 3 &&
                    rest_star >= star) {
                    tft.fillCircle(rest_x_tft, rest_y_tft, 3, TFT_BLUE);
                }
            }
        } else {
            // If the location of pressure is on buttons,
            // change value and redraw the button.
            if (screen_y <= BUTTON_HEIGHT) {
                star++;
                if (star > 5) {
                    star = 0;
                }
                starDisplay();
                delay(200);
            } else {
                if (sortMode == QSORT) {
                    sortMode = ISORT;
                } else if (sortMode == ISORT) {
                    sortMode = BOTH;
                } else {
                    sortMode = QSORT;
                }
                sortDisplay();
                delay(200);
            }
        }
    }
}

void scrollingMenu() {
    /*
    Description: Process joystick movement when in mode 1.
        Display the list of restaurants that have the corresponding
        number of stars by pages. Each page displays 21 restaurants.
    */
    int oldRest = selectedRest;

    int v = analogRead(JOY_VERT_ANALOG);

    // if the joystick was pushed up or down, change restaurants accordingly.
    if (v > JOY_CENTRE + JOY_DEADZONE) {
        ++selectedRest;
    } else if (v < JOY_CENTRE - JOY_DEADZONE) {
        --selectedRest;
    }

    // Page up (if not the first page).
    if (selectedRest < 0 && pageNum > 0) {
        tft.fillScreen(TFT_BLACK);
        pageNum--;
        selectedRest = 0;
        // Print the list of restaurants.
        for (int i = 0; i < REST_DISP_NUM; ++i) {
            printRestaurant(i);
        }
    }
    // or page down (if not the last page).
    else if (selectedRest >= REST_DISP_NUM &&
             pageNum < ((restNum - 1) / REST_DISP_NUM)) {
        tft.fillScreen(TFT_BLACK);
        pageNum++;
        selectedRest = 0;

        // Print the list of restaurants.
        // If it is the last page, it might
        // print less than 21 restaurants.
        if (pageNum == (restNum / REST_DISP_NUM)) {
            for (int i = 0; i < restNum % REST_DISP_NUM; ++i) {
                printRestaurant(i);
            }
        } else {
            for (int i = 0; i < REST_DISP_NUM; ++i) {
                printRestaurant(i);
            }
        }
    } else {
        // Otherwise, set borders
        if (pageNum == (restNum) / REST_DISP_NUM)
            selectedRest =
                constrain(selectedRest, 0, restNum % REST_DISP_NUM - 1);
        else
            selectedRest = constrain(selectedRest, 0, REST_DISP_NUM - 1);

        // If we picked a new restaurant, update the way it and the previously
        // selected restaurant are displayed.
        if (oldRest != selectedRest) {
            printRestaurant(oldRest);
            printRestaurant(selectedRest);
            delay(50); // so we don't scroll too fast
        }
    }

    // If we clicked on a restaurant.
    if (digitalRead(JOY_SEL) == LOW) {
        restaurant r;
        getRestaurant(&r,
                      restaurants[selectedRest + pageNum * REST_DISP_NUM].index,
                      &card, &cache);
        // Calculate the new map view.
        // Center the map view at the restaurant, constraining against the edge
        // of the map if necessary.
        curView.mapX = constrain(lon_to_x(r.lon) - DISP_WIDTH / 2, 0,
                                 MAPWIDTH - DISP_WIDTH);
        curView.mapY = constrain(lat_to_y(r.lat) - DISP_HEIGHT / 2, 0,
                                 MAPHEIGHT - DISP_HEIGHT);

        // Draw the cursor, clamping to an edge of the map if needed.
        curView.cursorX =
            constrain(lon_to_x(r.lon) - curView.mapX, CURSOR_SIZE / 2,
                      DISP_WIDTH - CURSOR_SIZE / 2 - 1);
        curView.cursorY =
            constrain(lat_to_y(r.lat) - curView.mapY, CURSOR_SIZE / 2,
                      DISP_HEIGHT - CURSOR_SIZE / 2 - 1);

        preView = curView;

        beginMode0();

        // Ensures a long click of the joystick will not register twice.
        while (digitalRead(JOY_SEL) == LOW) {
            delay(10);
        }
    }
}

int main() {
    setup();

    // Switch between the two modes.
    while (true) {
        if (displayMode == MAP) {
            scrollingMap();
        } else {
            scrollingMenu();
        }
    }

    Serial.end();
    return 0;
}
