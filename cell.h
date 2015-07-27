#ifndef CELL_H
#define CELL_H

#include "gpi/gpi.h"
#include "phitron/p3.h"
#include <list>

//Persistent program constants
#define CELL_PERSISTENT_VALUES 4
#define CELL_PERSISTENT_INPUTS (4 + CELL_PERSISTENT_VALUES)
#define CELL_PERSISTENT_STATIC_OUTPUTS 1
#define CELL_PERSISTENT_OUTPUTS (CELL_PERSISTENT_STATIC_OUTPUTS + CELL_PERSISTENT_VALUES)
#define CELL_PERSISTENT_CHROMOSOMES 32
#define CELL_PERSISTENT_CHROMOSOME_SIZE 64

//Neighbor program constants
#define CELL_NEIGHBOR_PERSISTENT_VALUES 4
#define CELL_NEIGHBOR_INPUTS (6 + CELL_PERSISTENT_VALUES + CELL_NEIGHBOR_PERSISTENT_VALUES)
#define CELL_NEIGHBOR_STATIC_OUTPUTS 5
#define CELL_NEIGHBOR_OUTPUTS (CELL_NEIGHBOR_STATIC_OUTPUTS + CELL_NEIGHBOR_PERSISTENT_VALUES)
#define CELL_NEIGHBOR_CHROMOSOMES 32
#define CELL_NEIGHBOR_CHROMOSOME_SIZE 64

//Signal program constants
#define CELL_SIGNAL_INPUTS (5 + CELL_PERSISTENT_VALUES + CELL_NEIGHBOR_PERSISTENT_VALUES)
#define CELL_SIGNAL_OUTPUTS 1
#define CELL_SIGNAL_CHROMOSOMES 32
#define CELL_SIGNAL_CHROMOSOME_SIZE 64

#define CELL_FORCE_LIMIT 1.0
#define CELL_DRAG_COEFFICIENT 0.01
#define CELL_CONNECT_DISTANCE 5.0
#define CELL_FOOD_CHILDREN_RATIO 0.5
#define CELL_TURN_FOOD_COST 100
#define CELL_INITIAL_FOOD 1<<20
#define CELL_MAX_INITIAL_VELOCITY 0.5

struct Cell;

struct NeighborDecision {
    //Eat neighbor
    bool eat;
    //Mate with neighbor
    bool mate;
    //Sever connection with neighbor
    bool sever;
    //Food to send to neighbor
    double send;
    //Apply pulling force (or pushing if negative)
    double force;
    //Signal to send
    double signal;
    //Persistant data
    double values[CELL_NEIGHBOR_PERSISTENT_VALUES];
    
    NeighborDecision() : signal(0.0) {
        for (int i = 0; i != CELL_NEIGHBOR_PERSISTENT_VALUES; i++)
            values[i] = 0;
    }
};

struct PersistentDecision {
    //Seek connections in area
    bool connect;
    //Persistent data
    double values[CELL_PERSISTENT_VALUES];
    
    PersistentDecision() {
        for (int i = 0; i != CELL_PERSISTENT_VALUES; i++)
            values[i] = 0;
    }
};

struct Neighbor {
    Cell* neighbor;
    NeighborDecision decision;
    std::list<Neighbor>::iterator neighborsDecision;
    
    Neighbor(Cell *neighbor) : neighbor(neighbor) {}
    
    bool operator==(Cell *other) const {
        return neighbor == other;
    }
};

struct Changes {
    bool death;
    uint64_t eatenBy;
    Cell *mate;
    
    void clear();
};

struct Cell {
    //Program to determine what actions to take
    gpi::Program neighborProgram;
    gpi::Program signalProgram;
    gpi::Program persistentProgram;
    PersistentDecision decision;
    phi::P3 particle;
    //Food is finite so that it does not get created or destroyed accidentally
    uint64_t food;
    std::list<Neighbor> neighbors;
    Changes changes;
    
    //Mate
    Cell(Cell &a, Cell &b, const phi::V3 &position, std::mt19937 &rand);
    
    //Generate
    Cell(const phi::V3 &position, const phi::V3 &velocity, std::mt19937 &rand);
    
    //Remove this from all neighbors and all neighbors from this
    void pluck();
    void solvePersistent();
    void solveSignal();
    void solveNeighbor();
    
    void enumerateConsumptions();
    void totalConsumptions();
    void accumulateSentFood();
    void decideMate();
    void starve();
    
    void die();
    
    void clear();
};

void connect(Cell &a, Cell &b);

#endif // CELL_H

