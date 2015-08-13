#ifndef GROUP_H
#define GROUP_H

#include "cell.h"

struct Group {
    std::list<Cell> cells;
    phi::V3 dimensions;
    std::mt19937 rand;
    
    Group(const phi::V3 &dimensions, uint32_t seed);
    
    void update();
    void spawn(unsigned amnt);
    
    void updateDeaths();
    
    //Wrap an origin-centered vector based on the dimensions; changes referenced vector
    void wrapVector(phi::V3 &delta);
    //Determine if the vector is a valid origin-centered vector based on the dimensions
    bool isValid(const phi::V3 &delta);
    
    double distanceSquared(const Cell &a, const Cell &b);
    
    void processPhysics(Cell &c);
};

#endif // GROUP_H

