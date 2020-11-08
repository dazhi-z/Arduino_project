#include "dijkstra.h"
#include "heap.h"
using namespace std;

void dijkstra(const WDigraph& graph,
              int startVertex,
              unordered_map<int, PIL>& tree) {
    /*
        Compute minimum cost paths that start from a given vertex
        Use a binary heap to efficiently retrieve the nearest unexplored
        vertex the start vertex at every iteration

        PIL is an alias for "pair<int, long long>" type as discussed in class

        PARAMETERS:
        WDigraph: an instance of the weighted directed graph (WDigraph) class
        startVertex: a vertex in this graph which serves as the root of the
                search tree
        tree: used to store minimum cost paths from the start vertex to each
                vertex
    */
    BinaryHeap<pair<int, int>, long long> events;
    // add the start point to heap
    events.insert(make_pair(startVertex, startVertex), 0);
    while (events.size() > 0) {
        // for each iteration, directly choose the nearest path, pop it
        // and add more available paths to the heap
        HeapItem<pair<int, int>, long long> heapItem = events.min();
        events.popMin();

        int u = heapItem.item.first;
        int v = heapItem.item.second;
        long long cost = heapItem.key;
        // if v is approachable, go through it
        if (tree.find(v) == tree.end()) {
            // maintain the shortest path
            tree[v] = make_pair(u, cost);

            for (auto i = graph.neighbours(v); i != graph.endIterator(v); i++) {
                int w = *i;
                // adding accumulated paths
                int new_cost = cost + graph.getCost(v, w);
                events.insert(make_pair(v, w), new_cost);
            }
        }
    }
}
