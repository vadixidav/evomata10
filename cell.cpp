#include "cell.h"

Cell::Cell(Cell &a, Cell &b, const phi::V3 &position, std::mt19937 &rand) : program(a.program),
           particle(1.0, position) {
    program.crossover(b.program, rand);
    particle.velocity = a.particle.velocity;
    particle.velocity += b.particle.velocity;
    particle.velocity *= 0.5; //Get average velocity between particles
    a.neighbors.push_back(this);
    b.neighbors.push_back(this);
    neighbors.push_back(&a);
    neighbors.push_back(&b);
}

Cell::Cell(const phi::V3 &position, const phi::V3 &velocity, std::mt19937 &rand) : 
           program(CELL_INPUTS, CELL_OUTPUTS, CELL_CHROMOSOMES, CELL_CHROMOSOME_SIZE, rand),
           particle(1.0, position, velocity) {
}

