#include "draw_route.h"
#include "map_drawing.h"

extern shared_vars shared;

// Draw the lines in terms of
// the route we obtained
void draw_route() {
    for (int i = 0; i < shared.num_waypoints - 1; i++) {
        // get the start and end points of each edge.
        int32_t x1 =
            longitude_to_x(shared.map_number, shared.waypoints[i].lon) -
            shared.map_coords.x;

        int32_t y1 = latitude_to_y(shared.map_number, shared.waypoints[i].lat) -
                     shared.map_coords.y;

        int32_t x2 =
            longitude_to_x(shared.map_number, shared.waypoints[i + 1].lon) -
            shared.map_coords.x;
        int32_t y2 =
            latitude_to_y(shared.map_number, shared.waypoints[i + 1].lat) -
            shared.map_coords.y;
        
        // if the points are in the screen, print them
        if (x1 >= 0 && x1 < displayconsts::display_width && y1 >= 0 &&
            y1 < displayconsts::display_height && x2 >= 0 &&
            x2 < displayconsts::display_width && y2 >= 0 &&
            y2 < displayconsts::display_height) {
            shared.tft->drawLine(x1, y1, x2, y2, TFT_BLUE);
        }
    }
}
