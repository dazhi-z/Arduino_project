// # -------------------------------------------
// # Name: Yifan Zhang, Yuxi Chen
// # ID: 1635595, 1649014
// # CMPUT 275, Winter 2020
// # Assignment #2: Driving Route Finder part 1
// # -------------------------------------------
#include "dijkstra.h"
#include "wdigraph.h"
#include <fstream>
#include <iostream>
#include <stack>
#include <string>

using namespace std;
typedef long long ll;

struct Point {
    long long lat; // latitude of the point
    long long lon; // longitude of the point
};

// the manhattan distance function
long long manhattan(const Point &pt1, const Point &pt2) {
    // Return the Manhattan distance between the two given points
    return abs(pt1.lat - pt2.lat) + abs(pt1.lon - pt2.lon);
}

// read the map file and build the graph
void readGraph(string filename, WDigraph &graph,
               unordered_map<int, Point> &points) {
    ifstream file;
    file.open(filename, ios::in);
    string line;
    while (getline(file, line)) {
        // for the vertex information
        if (line[0] == 'V') {
            line = line.substr(2);
            auto tmp = line.find(',');

            // get ID
            auto Id_String = line.substr(0, tmp);
            line = line.substr(tmp + 1);

            // get lat and lon
            tmp = line.find(',');

            // use function stod is to convert string to double
            double lat_double = stod(line.substr(0, tmp));
            double lon_double = stod(line.substr(tmp + 1));
            ll lat = static_cast<ll>(lat_double * 100000);
            ll lon = static_cast<ll>(lon_double * 100000);

            // convert ID from string to int
            int ID = 0;
            for (auto i : Id_String) {
                ID = ID * 10 + i - '0';
            }

            // add vertex
            points[ID].lat = lat;
            points[ID].lon = lon;
            graph.addVertex(ID);
        }
        // for the edge information
        if (line[0] == 'E') {
            line = line.substr(2);
            // get the substring between the first "," and the second ","
            string firstValue = line.substr(0, line.find(','));

            // get rid of the first value and ","
            line = line.substr(line.find(',') + 1);

            // get the substring between the second "," and the third ","
            string secondValue = line.substr(0, line.find(','));

            // convert IDs from string to int
            int ID_1 = 0, ID_2 = 0;
            for (auto i : firstValue) {
                ID_1 = ID_1 * 10 + i - '0';
            }
            for (auto i : secondValue) {
                ID_2 = ID_2 * 10 + i - '0';
            }
            // get the points and add the edge
            Point p1 = points[ID_1];
            Point p2 = points[ID_2];
            graph.addEdge(ID_1, ID_2, manhattan(p1, p2));
        }
    }
    file.close();
}

// get points from the input file, and find the nearest vertices
void input(int &startVertex, int &endVertex, unordered_map<int, Point> points) {
    string line;
    Point start, end;

    char key;
    cin >> key;
    if (key == 'R') {
        cin >> start.lat >> start.lon >> end.lat >> end.lon;
    }

    ll dist1 = manhattan(start, points.begin()->second);
    ll dist2 = manhattan(end, points.begin()->second);
    startVertex = points.begin()->first;
    endVertex = points.begin()->first;

    // traverse all the vertexes to find the nearest vertexes of the points
    for (auto i : points) {
        ll tmp1 = manhattan(start, i.second);
        ll tmp2 = manhattan(end, i.second);
        if (dist1 > tmp1) {
            dist1 = tmp1;
            startVertex = i.first;
        }
        if (dist2 > tmp2) {
            dist2 = tmp2;
            endVertex = i.first;
        }
    }
}

// find the route from start vertex to the end vertex
stack<Point> findRoute(int startVertex, int endVertex,
                       unordered_map<int, Point> points,
                       unordered_map<int, PIL> tree) {
    stack<Point> s; // the stack to store the path

    // when the parent of the end vertex is not the given start vertex, add this
    // vertex to the stack and update the end vertex to its parent
    while (tree[endVertex].first != startVertex) {
        s.push(points[endVertex]);
        endVertex = tree[endVertex].first;
    }

    // add the second vertex and the start vertex
    s.push(points[endVertex]);
    s.push(points[startVertex]);
    return s;
}

// print results to the output file based on the input instructions
void output(stack<Point> route) {

    char key;
    Point point;

    // No matter whether it has the path or not, it always output the route size
    cout << "N " << route.size() << endl;

    // if there exist a path from start vertex to end vertex, output the path
    // step by step based on the acknowledge ('A') in the input file
    if (!route.empty()) {
        bool stop = false; // the flag to stop
        while (cin >> (key) && !stop) {
            // if all the points in the path has been output and it still
            // receive the acknowledge, output the end flag and stop the loop
            if (key == 'A' && route.empty()) {
                cout << "E" << endl;
                stop = true;
            }

            // if it still have points in the path, output the a point
            if (key == 'A' && !route.empty()) {
                point = route.top();
                route.pop();
                cout << "W " << point.lat << " " << point.lon << endl;
            }
        }
    }
}

int main(int argc, char const *argv[]) {
    int startVertex, endVertex;
    WDigraph graph;
    unordered_map<int, Point> points;

    // the tree to store the vertex which could be reached from the start vertex
    // and their shortest paths
    unordered_map<int, PIL> tree;
    stack<Point> route;
    string filename = "edmonton-roads-2.0.1.txt";

    readGraph(filename, graph, points);

    // get the vertexes with the points given in the file
    input(startVertex, endVertex, points);

    // find the shortest path from start vertex to all vertexes reached
    dijkstra(graph, startVertex, tree);

    // find the route if it can reached the end vertex from the first vertex
    if (tree.find(endVertex) != tree.end()) {
        route = findRoute(startVertex, endVertex, points, tree);
    }

    // output the result to the file
    output(route);

    return 0;
}
