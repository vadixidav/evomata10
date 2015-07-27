#include "cell.h"
#include "assert.h"

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
}

Cell::Cell(const phi::V3 &position, const phi::V3 &velocity, std::mt19937 &rand) : 
           neighborProgram(CELL_NEIGHBOR_INPUTS, CELL_NEIGHBOR_OUTPUTS, CELL_NEIGHBOR_CHROMOSOMES,
                           CELL_NEIGHBOR_CHROMOSOME_SIZE, rand),
           signalProgram(CELL_SIGNAL_INPUTS, CELL_SIGNAL_OUTPUTS, CELL_SIGNAL_CHROMOSOMES,
                           CELL_SIGNAL_CHROMOSOME_SIZE, rand),
           persistentProgram(CELL_PERSISTENT_INPUTS, CELL_PERSISTENT_OUTPUTS, CELL_PERSISTENT_CHROMOSOMES,
                             CELL_PERSISTENT_CHROMOSOME_SIZE, rand),
           particle(1.0, position, velocity), food(CELL_INITIAL_FOOD) {
}

void Cell::pluck() {
    //Erase this from all neighbors
    for (Neighbor &n : neighbors)
        n.neighbor->neighbors.erase(n.neighborsDecision);
    //Clear this cell's neighbors
    neighbors.clear();
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
    decision.connect = uint64_t(persistentProgram.solveOutput(0, inputs)) == 0 ? false : true;
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
        
        neighborProgram.startSolve();
        //Update neighbor decision paramaters
        n.decision.eat = uint64_t(neighborProgram.solveOutput(0, inputs)) == 0 ? false : true;
        n.decision.mate = uint64_t(neighborProgram.solveOutput(1, inputs)) == 0 ? false : true;
        n.decision.sever = uint64_t(neighborProgram.solveOutput(2, inputs)) == 0 ? false : true;
        n.decision.force = neighborProgram.solveOutput(3, inputs);
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
        return;
    }
    food -= sentFood;
}

void Cell::accumulateSentFood() {
    for (Neighbor &n : neighbors)
        food += n.neighborsDecision->decision.send;
}

void Cell::decideMate() {
    for (Neighbor &n : neighbors)
        if (n.decision.mate) {
            changes.mate = n.neighbor;
            break;
        }
}

void Cell::starve() {
    if (food < CELL_TURN_FOOD_COST) {
        changes.death = true;
        return;
    }
    
    food -= CELL_TURN_FOOD_COST;
}

void connect(Cell &a, Cell &b) {
    a.neighbors.emplace_front(&b);
    b.neighbors.emplace_front(&a);
    a.neighbors.front().neighborsDecision = b.neighbors.begin();
    b.neighbors.front().neighborsDecision = a.neighbors.begin();
}

