#include "serial_handling.h"

extern shared_vars shared;
// max size of buffer, including null terminator
const uint16_t buf_size = 256;

// current number of chars in buffer, not counting null terminator
uint16_t buf_len = 0;

// input buffer
char* buffer = (char*)malloc(buf_size);

// It communicate with the server over Serial
// using the protocol in the assignment description.
// Get all waypoints and store them to shared.
uint8_t get_waypoints(const lon_lat_32& start, const lon_lat_32& end) {
    // set up buffer as empty string
    buffer[buf_len] = 0;

    // for now, nothing is stored
    shared.num_waypoints = 0;

    // send the request
    Serial.print("R ");
    Serial.print(start.lat);
    Serial.print(" ");
    Serial.print(start.lon);
    Serial.print(" ");
    Serial.print(end.lat);
    Serial.print(" ");
    Serial.println(end.lon);

    // wait for the server's response
    if (!readFromSerial(10000))
        return 0;
    if (buffer[0] != 'N') {
        return 0;
    }

    // get rid of the first entry 'N'
    buffer[0] = ' ';

    // if there's no path or too long path
    // just finish the process and wait for user's new operation
    int32_t length = atoi(buffer);
    if (length == 0 || length > max_waypoints) {
        return 1;
    }

    // send acknowledge message
    Serial.println("A");

    do {
        // if time is out, restart the process
        if (!readFromSerial(1000))
            return 0;
        lon_lat_32 point;
        if (buffer[0] == 'W') {
            // get rid of the first two characters "W "
            int index = 2;
            // get the latitude and longitude of each point
            point.lat = string_to_int32_t(index);
            index++;
            point.lon = string_to_int32_t(index);
            
            shared.waypoints[shared.num_waypoints] = point;
            shared.num_waypoints++;
            Serial.println("A");
        }
    } while (buffer[0] == 'W');

    // if the last response is not 'E'
    // restart the process
    if (buffer[0] != 'E') {
        return 0;
    }

    // 1 indicates a successful exchange, of course you should only output 1
    // in your final solution if the exchange was indeed successful
    // (otherwise, return 0 if the communication failed)
    return 1;
}

// Convert string message to int32_t type.
int32_t string_to_int32_t(int& index) {
    bool isPositive = true;
    int32_t result = 0;
    for (; index < buf_len && buffer[index] != ' '; index++) {
        if (buffer[index] == '-')
            isPositive = false;
        else
            result = result * 10 + buffer[index] - '0';
    }
    if (!isPositive)
        result *= -1;
    return result;
}

// Read message from Server
bool readFromSerial(int time) {
    char in_char;
    bool stop = false;
    buf_len = 0;
    buffer[buf_len] = 0;
    unsigned long curr = millis();
    while (true) {
        // return if timeouts
        if (millis() - curr > time)
            return false;
        if (Serial.available()) {
            // read the incoming byte:
            char in_char = Serial.read();

            // if end of line is received, waiting for line is done:
            if (in_char == '\n' || in_char == '\r') {
                return true;
                // now we process the buffer
            } else {
                // add character to buffer, provided that we don't overflow.
                // drop any excess characters.
                if (buf_len < buf_size - 1) {
                    buffer[buf_len] = in_char;
                    buf_len++;
                    buffer[buf_len] = 0;
                }
            }
        }
    }
}
