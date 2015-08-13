#include "group.h"
#include <algorithm>
#include <iostream>

//Rand from 0 to 1
double normRand(std::mt19937 &rand) {
    return double(rand())/rand.max();
}

//Rand from -1 to 1
double balancedRand(std::mt19937 &rand) {
    return normRand(rand) * 2 - 1;
}

Group::Group(const phi::V3 &dimensions, uint32_t seed) : dimensions(dimensions), rand(seed) {
}

double Group::distanceSquared(const Cell &a, const Cell &b) {
    phi::V3 dis = b.particle.position;
    dis -= a.particle.position;
    wrapVector(dis);
    return dis.magnitudeSquared();
}

void Group::update() {
    //Clear cells
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::clear, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Run cell persistent programs
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::solvePersistent, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Connect cells that request it
    for (auto i = cells.begin(); i != cells.end(); i++) {
        Cell &c = *i;
        //If the cell decided to connect
        if (c.decision.connect) {
            //Check every cell
            for (auto j = cells.begin(); j != cells.end(); j++) {
                //If they are not the same cell and i is not already connected to j
                if (i != j && std::find(c.neighbors.begin(), c.neighbors.end(), &*j) == c.neighbors.end()) {
                    //Also check radius
                    phi::V3 dis = c.particle.position;
                    dis -= j->particle.position;
                    wrapVector(dis);
                    //If radius is less than the connection distance
                    if (dis.magnitudeSquared() < PHYSICS_CONNECT_DISTANCE * PHYSICS_CONNECT_DISTANCE)
                        //Connect these two cells
                        connect(c, *j);
                }
            }
        }
    }
    
    //Find all cell distances
    for (Cell &c : cells) {
        c.thread = std::thread([&c, this](){
            for (Neighbor &n : c.neighbors) {
                phi::V3 dis = n.neighbor->particle.position;
                dis -= c.particle.position;
                wrapVector(dis);
                n.distance = dis.magnitude();
            }
        });
    }
    for (Cell &c : cells)
        c.thread.join();
    
    //Run cell signal programs
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::solveSignal, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Run cell neighbor programs
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::solveNeighbor, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Compute consumptions
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::enumerateConsumptions, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Determine results of consumptions
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::totalConsumptions, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Kill off cells that were consumed
    updateDeaths();
    
    //For cells that are still alive, send and recieve food
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::accumulateSentFood, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Apply the food cost to exist
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::handleStarve, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Kill off cells that starved
    updateDeaths();
    
    //Disconnect all cells that ask to be disconnected or that are too far
    for (Cell &c : cells) {
        for (auto i = c.neighbors.begin(); i != c.neighbors.end(); ) {
            Neighbor &n = *i;
            if (n.decision.sever || distanceSquared(c, *n.neighbor) >
                    PHYSICS_DISCONNECT_DISTANCE * PHYSICS_DISCONNECT_DISTANCE) {
                n.neighbor->neighbors.erase(n.neighborsDecision);
                i = c.neighbors.erase(i);
            } else {
                i++;
            }
        }
    }
    
    //Determine what the actual mate will be
    for (Cell &c : cells)
        c.thread = std::thread(&Cell::decideMate, &c);
    for (Cell &c : cells)
        c.thread.join();
    
    //Handle mating
    for (Cell &c : cells) {
        //If a mate was chosen and the mate chose this cell
        if (c.changes.mate && &c == c.changes.mate->changes.mate) {
            //Clear the mate on the other cell to avoid double-breeding
            c.changes.mate->changes.mate = nullptr;
            
#ifdef CELL_DIVIDE_FOOD_THRESHOLD
            //Dont allow cells to mate if below threshold
            if (c.food < CELL_DIVIDE_FOOD_THRESHOLD || c.changes.mate->food < CELL_DIVIDE_FOOD_THRESHOLD)
                continue;
#endif
            
            //Find the vector point towards the other cell from this cell
            phi::V3 dis = c.changes.mate->particle.position;
            dis -= c.particle.position;
            //Wrap the distance so that it is the shortest distance possible toroidially
            wrapVector(dis);
            //Get half of the distance
            dis /= 2;
            //Add it to the original position
            dis += c.particle.position;
            //Randomly move the cell in the area to create randomness
            dis += phi::V3(balancedRand(rand) * PHYSICS_CONNECT_DISTANCE,
                           balancedRand(rand) * PHYSICS_CONNECT_DISTANCE,
                           balancedRand(rand) * PHYSICS_CONNECT_DISTANCE);
            //Finally wrap the new vector that is between the previous vectors
            wrapVector(dis);
            //Make the new cell using the computed
            cells.emplace_front(c, *c.changes.mate, dis, rand);
            cells.front().food += c.food * CELL_FOOD_CHILDREN_RATIO + c.changes.mate->food * CELL_FOOD_CHILDREN_RATIO;
            c.food -= c.food * CELL_FOOD_CHILDREN_RATIO;
            c.changes.mate->food -= c.changes.mate->food * CELL_FOOD_CHILDREN_RATIO;
        } else
            c.changes.mate = nullptr;
        ///At this point in the code, one cell in each pair of mated cells contains a reference.
        ///All other cells are nulled if they did not mate.
    }
    
    //Update physics
    for (Cell &c : cells)
        c.thread = std::thread([this, &c](){
            processPhysics(c);
        });
    for (Cell &c : cells)
        c.thread.join();
    
    //Kill off cells that did something they werent supposed to with the laws of physics
    updateDeaths();
    
    for (Cell &c : cells)
        if (normRand(rand) < CELL_MUTATION_CHANCE)
            c.mutate(rand);
}

void Group::spawn(unsigned amnt) {
    for (unsigned i = 0; i != amnt; i++) {
        cells.emplace_front(phi::V3(balancedRand(rand) * dimensions.x, balancedRand(rand) * dimensions.y,
                                    balancedRand(rand) * dimensions.z),
                            phi::V3(balancedRand(rand) * PHYSICS_MAX_INITIAL_VELOCITY,
                                    balancedRand(rand) * PHYSICS_MAX_INITIAL_VELOCITY,
                                    balancedRand(rand) * PHYSICS_MAX_INITIAL_VELOCITY),
                            rand);
        for (unsigned j = 0; j != CELL_SPAWN_PARTNERS; j++) {
            cells.emplace_front(cells.front(),
                                phi::V3(cells.front().particle.position.x + balancedRand(rand) * PHYSICS_CONNECT_DISTANCE,
                                        cells.front().particle.position.y + balancedRand(rand) * PHYSICS_CONNECT_DISTANCE,
                                        cells.front().particle.position.z + balancedRand(rand) * PHYSICS_CONNECT_DISTANCE));
            cells.front().food = CELL_INITIAL_FOOD;
            wrapVector(cells.front().particle.position);
        }
    }
}

void Group::updateDeaths() {
    for (auto i = cells.begin(); i != cells.end(); ) {
        Cell &c = *i;
        //Death condition
        if (c.isDead()) {
            //Perform death operations
            c.die();
            i = cells.erase(i);
        } else
            //Otherwise go to next cell
            i++;
    }
}

void Group::wrapVector(phi::V3 &delta) {
    if (std::abs(delta.x) > dimensions.x)
        delta.x -= 2 * copysign(dimensions.x, delta.x);
    if (std::abs(delta.y) > dimensions.y)
        delta.y -= 2 * copysign(dimensions.y, delta.y);
    if (std::abs(delta.z) > dimensions.z)
        delta.z -= 2 * copysign(dimensions.z, delta.z);
}

bool Group::isValid(const phi::V3 &delta) {
    return std::isnormal(delta.x) && std::isnormal(delta.y) && std::isnormal(delta.z) &&
           std::abs(delta.x) < dimensions.x && std::abs(delta.y) < dimensions.y && std::abs(delta.z) < dimensions.z;
}

void Group::processPhysics(Cell &c) {
    //Apply drag
    c.particle.drag(CELL_DRAG_COEFFICIENT);
    //Process neighbor springing forces
    for (Neighbor &n : c.neighbors) {
        double force = CELL_FORCE_COEFFICIENT * n.decision.force * n.neighborsDecision->decision.force;
        if (std::abs(force) > CELL_FORCE_LIMIT)
            force = copysign(CELL_FORCE_LIMIT, force);
        phi::V3 adjDis = n.neighbor->particle.position;
        adjDis -= c.particle.position;
        wrapVector(adjDis);
        phi::V3 adjPos = c.particle.position;
        adjPos += adjDis;
        
        c.particle.spring(force, PHYSICS_EQUILIBRIUM_DISTANCE, adjPos);
        c.particle.gravitate(-PHYSICS_REPULSION_COEFFICIENT, adjPos, PHYSICS_REPULSION_RADIUS);
    }
    c.particle.advance();
    wrapVector(c.particle.position);
    if (!isValid(c.particle.position))
        c.changes.death = true;
}

