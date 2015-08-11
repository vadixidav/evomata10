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
#define CELL_PERSISTENT_CHROMOSOMES 8
#define CELL_PERSISTENT_CHROMOSOME_SIZE 32

//Neighbor program constants
#define CELL_NEIGHBOR_PERSISTENT_VALUES 4
#define CELL_NEIGHBOR_INPUTS (7 + CELL_PERSISTENT_VALUES + CELL_NEIGHBOR_PERSISTENT_VALUES)
#define CELL_NEIGHBOR_STATIC_OUTPUTS 5
#define CELL_NEIGHBOR_OUTPUTS (CELL_NEIGHBOR_STATIC_OUTPUTS + CELL_NEIGHBOR_PERSISTENT_VALUES)
#define CELL_NEIGHBOR_CHROMOSOMES 8
#define CELL_NEIGHBOR_CHROMOSOME_SIZE 32

//Signal program constants
#define CELL_SIGNAL_INPUTS (5 + CELL_PERSISTENT_VALUES + CELL_NEIGHBOR_PERSISTENT_VALUES)
#define CELL_SIGNAL_OUTPUTS 1
#define CELL_SIGNAL_CHROMOSOMES 9
#define CELL_SIGNAL_CHROMOSOME_SIZE 32

//Various cell constants
#define CELL_FORCE_LIMIT 0.005
#define CELL_FORCE_COEFFICIENT 1.0
#define CELL_EQUILIBRIUM_DISTANCE 0
#define CELL_DRAG_COEFFICIENT 0.001
#define CELL_CONNECT_DISTANCE 0.1
#define CELL_FOOD_CHILDREN_RATIO 0.5
#define CELL_TURN_FOOD_COST 1<<4
#define CELL_INITIAL_FOOD 1<<8
#define CELL_MAX_INITIAL_VELOCITY 0.001
#define CELL_MUTATION_CHANCE 0.0001

//Cell behavioral controls
//#define CELL_DIVIDE_FOOD_THRESHOLD 1<<11
#define CELL_PREVENT_CANNIBALISM
#define CELL_ALLOW_CONSUMPTION

struct Cell;

struct NeighborDecision {
    //Eat neighbor
    bool eat;
    //Mate with neighbor (strength of decision)
    double mate;
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
    double distance;
    NeighborDecision decision;
    std::list<Neighbor>::iterator neighborsDecision;
    
    Neighbor(Cell *neighbor) : neighbor(neighbor) {}
    
    bool operator==(const Cell *other) const {
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
    
    uint64_t species;
    
    //Mate
    Cell(Cell &a, Cell &b, const phi::V3 &position, std::mt19937 &rand);
    
    //Divide
    Cell(Cell &parent, const phi::V3 &position);
    
    //Generate
    Cell(const phi::V3 &position, const phi::V3 &velocity, std::mt19937 &rand);
    
    //Remove this from all neighbors and all neighbors from this
    void pluck();
    //Compute inputs and solve persistent program (for determining persistent inputs and global cell actions)
    void solvePersistent();
    //Compute inputs and solve signal program (for determining the signal to send to neighbors)
    void solveSignal();
    //Compute inputs and solve neighbor program (for determining actions on neighbors
    void solveNeighbor();
    
    //Determine how many times this cell has been consumed
    void enumerateConsumptions();
    //Determine if this cell dies from being eaten and compute result of receiving/giving food
    void totalConsumptions();
    //Accumulate the food sent from all surviving neighbors
    void accumulateSentFood();
    //Determine if cell starved and apply food cost
    void handleStarve();
    //Determine the actual mate
    void decideMate();
    //Handle mutation
    void mutate(std::mt19937 &rand);
    
    //Check if cell has been killed
    bool isDead();
    //Run cell cleanup before it must be removed
    void die();
    
    //Ready cell for next cycle
    void clear();
};

//Connect cell a and b
void connect(Cell &a, Cell &b);

#endif // CELL_H

