#include "cell.h"
#include "assert.h"
#include <iostream>

void Changes::clear() {
    death = false;
    eatenBy = 0;
    mate = nullptr;
}

Cell::Cell(Cell &a, Cell &b, const phi::V3 &position, std::mt19937 &rand) : neighborProgram(a.neighborProgram),
           signalProgram(a.signalProgram), persistentProgram(a.persistentProgram), particle(1.0, position), food(0) {
    //Create crossover programs
    neighborProgram.crossover(b.neighborProgram, rand);
    signalProgram.crossover(b.signalProgram, rand);
    persistentProgram.crossover(b.persistentProgram, rand);
    
    //Average velocity
    particle.velocity = a.particle.velocity;
    particle.velocity += b.particle.velocity;
    particle.velocity *= 0.5; //Get average velocity between particles
    
    //Add this cell as a connection to a and b
    connect(a, *this);
    connect(b, *this);
    
    //species = (a.species == b.species) ? a.species : ((uint64_t(rand()) << 32) | uint64_t(rand()));
    species = (a.species & 0xFFFFFFFF00000000) | (b.species & 0x00000000FFFFFFFF);
}

Cell::Cell(Cell &parent, const phi::V3 &position) : neighborProgram(parent.neighborProgram),
    signalProgram(parent.signalProgram), persistentProgram(parent.persistentProgram),
    particle(1.0, position, parent.particle.velocity), food(0), species(parent.species) {
    connect(parent, *this);
}

Cell::Cell(const phi::V3 &position, const phi::V3 &velocity, std::mt19937 &rand) : 
           neighborProgram(CELL_NEIGHBOR_INPUTS, CELL_NEIGHBOR_OUTPUTS, CELL_NEIGHBOR_CHROMOSOMES,
                           CELL_NEIGHBOR_CHROMOSOME_SIZE, rand),
           signalProgram(CELL_SIGNAL_INPUTS, CELL_SIGNAL_OUTPUTS, CELL_SIGNAL_CHROMOSOMES,
                           CELL_SIGNAL_CHROMOSOME_SIZE, rand),
           persistentProgram(CELL_PERSISTENT_INPUTS, CELL_PERSISTENT_OUTPUTS, CELL_PERSISTENT_CHROMOSOMES,
                             CELL_PERSISTENT_CHROMOSOME_SIZE, rand),
           particle(1.0, position, velocity), food(CELL_INITIAL_FOOD) {
    species = (uint64_t(rand()) << 32) | uint64_t(rand());
}

void Cell::pluck() {
    //Erase this from all neighbors
    for (Neighbor &n : neighbors)
        n.neighbor->neighbors.erase(n.neighborsDecision);
    //Clear this cell's neighbors
    neighbors.clear();
}

bool Cell::isDead() {
    return changes.death;
}

void Cell::die() {
    pluck();
}

void Cell::clear() {
    changes.clear();
}

void Cell::solvePersistent() {
    double inputs[CELL_PERSISTENT_INPUTS];
    for (int i = 0; i != CELL_PERSISTENT_VALUES; i++)
        inputs[i] = decision.values[i];
    double *ioff = inputs + CELL_PERSISTENT_VALUES;
    ioff[0] = 0;
    ioff[1] = 1;
    ioff[2] = 2;
    ioff[3] = food;
    
    persistentProgram.startSolve();
    decision.connect = uint64_t(persistentProgram.solveOutput(0, inputs)) == 0 ? true : false;
    for (int i = 0; i != CELL_PERSISTENT_VALUES; i++)
        decision.values[i] = persistentProgram.solveOutput(CELL_PERSISTENT_STATIC_OUTPUTS + i, inputs);
}

void Cell::solveSignal() {
    for (Neighbor &n : neighbors) {
        double inputs[CELL_SIGNAL_INPUTS];
        for (int i = 0; i != CELL_PERSISTENT_VALUES; i++)
            inputs[i] = decision.values[i];
        for (int i = 0; i != CELL_NEIGHBOR_PERSISTENT_VALUES; i++)
            inputs[CELL_PERSISTENT_VALUES + i] = n.decision.values[i];
        double *ioff = inputs + CELL_PERSISTENT_VALUES + CELL_NEIGHBOR_PERSISTENT_VALUES;
        ioff[0] = 0;
        ioff[1] = 1;
        ioff[2] = 2;
        ioff[3] = food;
        ioff[4] = n.neighbor->food;
        
        signalProgram.startSolve();
        n.decision.signal = signalProgram.solveOutput(0, inputs);
        if (!std::isnormal(n.decision.signal))
            n.decision.signal = 0;
    }
}

void Cell::solveNeighbor() {
    for (Neighbor &n : neighbors) {
        double inputs[CELL_NEIGHBOR_INPUTS];
        for (int i = 0; i != CELL_PERSISTENT_VALUES; i++)
            inputs[i] = decision.values[i];
        for (int i = 0; i != CELL_NEIGHBOR_PERSISTENT_VALUES; i++)
            inputs[CELL_PERSISTENT_VALUES + i] = n.decision.values[i];
        double *ioff = inputs + CELL_PERSISTENT_VALUES + CELL_NEIGHBOR_PERSISTENT_VALUES;
        ioff[0] = 0;
        ioff[1] = 1;
        ioff[2] = 2;
        ioff[3] = food;
        ioff[4] = n.neighbor->food;
        ioff[5] = n.neighborsDecision->decision.signal;
        ioff[6] = n.distance;
        
        neighborProgram.startSolve();
        //Update neighbor decision paramaters
#ifdef CELL_ALLOW_CONSUMPTION
#ifdef CELL_PREVENT_CANNIBALISM
        if (n.neighbor->species == species)
            n.decision.eat = false;
        else
#endif
            n.decision.eat = uint64_t(neighborProgram.solveOutput(0, inputs)) == 0 ? true : false;
#else
        n.decision.eat = false;
#endif
        n.decision.mate = neighborProgram.solveOutput(1, inputs);
        //Ensure mate is a number
        if (!std::isnormal(n.decision.mate))
            n.decision.mate = 0;
        n.decision.sever = uint64_t(neighborProgram.solveOutput(2, inputs)) == 0 ? true : false;
        n.decision.force = neighborProgram.solveOutput(3, inputs);
        //Ensure force is a number
        if (!std::isnormal(n.decision.force))
            n.decision.force = 0;
        n.decision.send = neighborProgram.solveOutput(4, inputs);
        //Update neighbor decision persistent values
        for (int i = 0; i != CELL_NEIGHBOR_PERSISTENT_VALUES; i++)
            n.decision.values[i] = neighborProgram.solveOutput(CELL_NEIGHBOR_STATIC_OUTPUTS + i, inputs);
    }
}

void Cell::enumerateConsumptions() {
    for (Neighbor &n : neighbors)
        if (n.neighborsDecision->decision.eat)
            changes.eatenBy++;
}

void Cell::totalConsumptions() {
    //We also die if somebody has eaten us
    if (changes.eatenBy != 0) {
        changes.death = true;
        std::cerr << "Cell was eaten!" << std::endl;
        return;
    }
    uint64_t sentFood = 0;
    for (Neighbor &n : neighbors) {
        if (n.decision.eat)
            food += n.neighbor->food/n.neighbor->changes.eatenBy;
        if (n.decision.send > 0)
            sentFood += uint64_t(n.decision.send);
    }
    
    if (sentFood >= food) {
        changes.death = true;
        std::cerr << "Cell sent too much food!" << std::endl;
        return;
    }
    food -= sentFood;
}

void Cell::accumulateSentFood() {
    for (Neighbor &n : neighbors)
        food += n.neighborsDecision->decision.send;
}

void Cell::decideMate() {
    double best = 0.0;
    for (Neighbor &n : neighbors)
        if (n.decision.mate > best) {
            best = n.decision.mate;
            changes.mate = n.neighbor;
            break;
        }
}

void Cell::handleStarve() {
    if (food < CELL_TURN_FOOD_COST) {
        std::cerr << "Cell starved!" << std::endl;
        changes.death = true;
        return;
    }
    
    food -= CELL_TURN_FOOD_COST;
}

void Cell::mutate(std::mt19937 &rand) {
    neighborProgram.mutate(rand);
    signalProgram.mutate(rand);
    persistentProgram.mutate(rand);
    species = (uint64_t(rand()) << 32) | uint64_t(rand());
}

void connect(Cell &a, Cell &b) {
    a.neighbors.emplace_front(&b);
    b.neighbors.emplace_front(&a);
    a.neighbors.front().neighborsDecision = b.neighbors.begin();
    b.neighbors.front().neighborsDecision = a.neighbors.begin();
}

