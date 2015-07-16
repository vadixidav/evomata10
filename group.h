#ifndef GROUP_H
#define GROUP_H

#include "cell.h"

struct Group {
    std::list<Cell> cells;
    phi::V3 dimensions;
    
    Group(const phi::V3 &dimensions);
};

#endif // GROUP_H

