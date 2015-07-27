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
    void updatePhysics();
    
    void wrapVector(phi::V3 &delta);
    void wrapPosition(phi::V3 &position);
    bool isValid(phi::V3 &position);
    
    void processPhysics(Cell &c);
};

#endif // GROUP_H

