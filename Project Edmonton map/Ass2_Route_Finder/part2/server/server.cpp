#include <cassert>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include "dijkstra.h"
#include "serialport.cpp"
#include "serialport.h"
#include "wdigraph.h"
enum State { fail, success };
struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
    long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
    return abs(dLat) + abs(dLon);
}

// finds the id of the point that is closest to the given point "pt"
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
    pair<int, Point> best = *points.begin();

    for (const auto& check : points) {
        if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
            best = check;
        }
    }
    return best.first;
}

// read the graph from the file that has the same format as the "Edmonton graph"
// file
void readGraph(const string& filename,
               WDigraph& g,
               unordered_map<int, Point>& points) {
    ifstream fin(filename);
    string line;

    while (getline(fin, line)) {
        // split the string around the commas, there will be 4 substrings either
        // way
        string p[4];
        int at = 0;
        for (auto c : line) {
            if (c == ',') {
                // start new string
                ++at;
            } else {
                // append character to the string we are building
                p[at] += c;
            }
        }

        if (at != 3) {
            // empty line
            break;
        }

        if (p[0] == "V") {
            // new Point
            int id = stoi(p[1]);
            assert(
                id ==
                stoll(p[1]));  // sanity check: asserts if some id is not 32-bit
            points[id].lat = static_cast<long long>(stod(p[2]) * 100000);
            points[id].lon = static_cast<long long>(stod(p[3]) * 100000);
            g.addVertex(id);
        } else {
            // new directed edge
            int u = stoi(p[1]), v = stoi(p[2]);
            g.addEdge(u, v, manhattan(points[u], points[v]));
        }
    }
}
// Convert the string type of message
// to long long and store to point variables.
void string_to_longlong(string line, Point& sPoint, Point& ePoint) {
    int index;
    line = line.substr(2);
    sPoint.lat = stoll(line);
    index = line.find(' ');
    line = line.substr(index + 1);
    sPoint.lon = stoll(line);
    index = line.find(' ');
    line = line.substr(index + 1);
    ePoint.lat = stoll(line);
    index = line.find(' ');
    line = line.substr(index + 1);
    ePoint.lon = stoll(line);
}

int main() {
    WDigraph graph;
    unordered_map<int, Point> points;
    SerialPort Serial("/dev/ttyACM0");
    string line;

    // build the graph
    readGraph("edmonton-roads-2.0.1.txt", graph, points);

    while (true) {
        // read a request
        Point sPoint, ePoint;

        // read the  lines until get the request
        do {
            line = Serial.readline();
            if (line[0] == 'R') {
                string_to_longlong(line, sPoint, ePoint);
            }
        } while (line[0] != 'R');
        cout << "Received: " << line;

        // get the points closest to the two points we read
        int start = findClosest(sPoint, points),
            end = findClosest(ePoint, points);

        // run dijkstra's, this is the unoptimized version that does not stop
        // when the end is reached but it is still fast enough
        unordered_map<int, PIL> tree;
        dijkstra(graph, start, tree);

        // no path
        if (tree.find(end) == tree.end()) {
            cout << "Sending: N 0" << endl;
            assert(Serial.writeline("N 0\n"));
        } else {
            // read off the path by stepping back through the search tree
            list<int> path;
            string message;
            State state = success;

            while (end != start) {
                path.push_front(end);
                end = tree[end].first;
            }
            path.push_front(start);

            // output the path
            message = to_string(path.size());
            cout << "Sending: N " << path.size() << endl;
            assert(Serial.writeline("N " + message + "\n"));

            // if meet timeouts, we restart it
            message = Serial.readline(1000);
            if (message != "A\n") {
                continue;
            }
            cout << "Received: " << message;

            // output the route
            for (int v : path) {
                if (state == success) {
                    cout << "Sending: W " << points[v].lat << ' '
                         << points[v].lon << endl;
                    message = "W " + to_string(points[v].lat) + " " +
                              to_string(points[v].lon);
                    assert(Serial.writeline(message + "\n"));

                    // if time is out or the reply is invalid,
                    // we change the state to fail and restart it
                    message = Serial.readline(1000);
                    cout << "Received: " << message;
                    if (message != "A\n") {
                        state = fail;
                    }
                }
            }

            // restart it if the state fails
            if (state == fail) {
                continue;
            } else {
                // the operation is successfully done
                cout << "Sending: E" << endl;
                assert(Serial.writeline("E\n"));
            }
        }
        cout << endl;
    }

    return 0;
}
