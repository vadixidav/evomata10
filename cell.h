#ifndef CELL_H
#define CELL_H

#include "gpi/gpi.h"
#include "phitron/p3.h"
#include <list>

#define CELL_INPUTS 1
#define CELL_OUTPUTS 1
#define CELL_CHROMOSOMES 32
#define CELL_CHROMOSOME_SIZE 64

struct Cell {
    gpi::Program program;
    phi::P3 particle;
    std::list<Cell*> neighbors;
    
    //Mate
    Cell(Cell &a, Cell &b, const phi::V3 &position, std::mt19937 &rand);
    
    //Generate
    Cell(const phi::V3 &position, const phi::V3 &velocity, std::mt19937 &rand);
};

#endif // CELL_H

