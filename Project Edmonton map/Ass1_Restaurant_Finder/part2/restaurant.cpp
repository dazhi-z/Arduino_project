#include "restaurant.h"

/*
        Sets *ptr to the i'th restaurant. If this restaurant is already in the
   cache, it just copies it directly from the cache to *ptr. Otherwise, it
   fetches the block containing the i'th restaurant and stores it in the cache
   before setting *ptr to it.
*/
void getRestaurant(restaurant *ptr, int i, Sd2Card *card, RestCache *cache) {
    // calculate the block with the i'th restaurant
    uint32_t block = REST_START_BLOCK + i / 8;

    // if this is not the cached block, read the block from the card
    if (block != cache->cachedBlock) {
        while (!card->readBlock(block, (uint8_t *)cache->block)) {
            Serial.print("readblock failed, try again");
        }
        cache->cachedBlock = block;
    }

    // either way, we have the correct block so just get the restaurant
    *ptr = cache->block[i % 8];
}

// Swaps the two restaurants (which is why they are pass by reference).
void swap(RestDist &r1, RestDist &r2) {
    RestDist tmp = r1;
    r1 = r2;
    r2 = tmp;
}

// Insertion sort to sort the restaurants.
void insertionSort(RestDist restaurants[], int num) {
    // Invariant: at the start of iteration i, the
    // array restaurants[0 .. i-1] is sorted.
    for (int i = 1; i < num; ++i) {
        // Swap restaurant[i] back through the sorted list restaurants[0 .. i-1]
        // until it finds its place.
        for (int j = i; j > 0 && restaurants[j].dist < restaurants[j - 1].dist;
             --j) {
            swap(restaurants[j - 1], restaurants[j]);
        }
    }
}

int partition(RestDist restaurant[], int begin, int end) {
    // Find the pivot and return it.
    int key = restaurant[end].dist;
    int i = begin - 1;
    for (int j = begin; j < end; j++) {
        if (restaurant[j].dist <= key) {
            i++;
            swap(restaurant[i], restaurant[j]);
        }
    }
    swap(restaurant[i + 1], restaurant[end]);
    return i + 1;
}

void quickSort(RestDist restaurant[], int begin, int end) {
    // Sort the arrays before and after the pivot
    int position = 0;
    if (begin < end) {
        position = partition(restaurant, begin, end);
        quickSort(restaurant, begin, position - 1);
        quickSort(restaurant, position + 1, end);
    }
}

// Computes the manhattan distance between two points (x1, y1) and (x2, y2).
int16_t manhattan(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

/*
        Fetches all restaurants from the card, saves their RestDist information
        in restaurants[], and then sorts them based on their distance to the
        point on the map represented by the MapView.
*/
void getAndInsertSortRestaurants(const MapView &mv, RestDist restaurants[],
                                 Sd2Card *card, RestCache *cache, int star,
                                 int &num) {
    /*
    Description: Get restaurants that have the corresponding
        number of stars and use insertion sort to sort the
        restaurant array by distance.
    */
    restaurant r;

    // Count the number of corresponding restaurants
    num = 0;
    for (int i = 0; i < NUM_RESTAURANTS; ++i) {
        getRestaurant(&r, i, card, cache);
        int rest_star = max(floor((r.rating + 1) / 2), 1);
        if (rest_star >= star) {
            restaurants[num].index = i;
            restaurants[num].dist =
                manhattan(lat_to_y(r.lat), lon_to_x(r.lon),
                          mv.mapY + mv.cursorY, mv.mapX + mv.cursorX);
            num++;
        }
    }

    // Now sort them and record running time.
    Serial.print("isort ");
    Serial.print(num);
    Serial.print(" restaurants: ");
    unsigned long time = millis();
    insertionSort(restaurants, num);
    Serial.print(millis() - time);
    Serial.println(" ms");
}

void getAndQuickSortRestaurants(const MapView &mv, RestDist restaurants[],
                                Sd2Card *card, RestCache *cache, int star,
                                int &num) {
    /*
    Description: Get restaurants that have the corresponding
        number of stars and use insertion sort to sort the
        restaurant array by distance.
    */
    restaurant r;

    // Count the number of corresponding restaurants
    num = 0;
    for (int i = 0; i < NUM_RESTAURANTS; ++i) {
        getRestaurant(&r, i, card, cache);
        int rest_star = max(floor((r.rating + 1) / 2), 1);
        if (rest_star >= star) {
            restaurants[num].index = i;
            restaurants[num].dist =
                manhattan(lat_to_y(r.lat), lon_to_x(r.lon),
                          mv.mapY + mv.cursorY, mv.mapX + mv.cursorX);
            num++;
        }
    }
    // Now sort them and record running time.
    Serial.print("qsort ");
    Serial.print(num);
    Serial.print(" restaurants: ");
    unsigned long time = millis();
    quickSort(restaurants, 0, num - 1);
    Serial.print(millis() - time);
    Serial.println(" ms");
}