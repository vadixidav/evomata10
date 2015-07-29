#include <iostream>
#include "gpi/gpi.h"
#include "phitron/phitron.h"
#include "window.h"
#include "draw.h"
#include <chrono>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

//Amount of particles in test simulation
#define OBJECTS 128

using namespace std;
using namespace chrono;

int main() {
    Window window("testing", WINDOW_WIDTH, WINDOW_HEIGHT);
    Renderer renderer(window);
    
    GroupRenderer gr(WINDOW_WIDTH, WINDOW_HEIGHT);
    
    steady_clock::time_point lastTime = steady_clock::now();
    
    phi::P3 particles[OBJECTS];
    phi::V3 colors[OBJECTS];
    
    std::mt19937 rand(3);
    
    for (unsigned i = 0; i != OBJECTS; i++) {
        particles[i].position = phi::V3(rand()/double(rand.max())*2-1, rand()/double(rand.max())*2-1, rand()/double(rand.max())/8);
        particles[i].velocity = phi::V3(rand()/double(rand.max())*2-1, rand()/double(rand.max())*2-1, rand()/double(rand.max())*2-1);
        particles[i].velocity *= 0.0005;
        particles[i].inertia = 100.0;
        
        colors[i] = phi::V3(rand()/double(rand.max()), rand()/double(rand.max()), rand()/double(rand.max()));
        colors[i] *= 1;
    }
    
    float *posbuffer = (float*)gr.buffer.buffer.map(GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
    
    while (true) {
        SDL_Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case SDL_QUIT:
                return 0;
            }
        }
        
        for (unsigned i = 0; i != OBJECTS; i++) {
            for (unsigned j = i + 1; j != OBJECTS; j++) {
                phi::attract(0.0001, particles[i], particles[j], 0.1);
            }
            particles[i].advance();
            
            posbuffer[i*7 + 0] = particles[i].position.x;
            posbuffer[i*7 + 1] = particles[i].position.y;
            posbuffer[i*7 + 2] = particles[i].position.z;
            
            posbuffer[i*7 + 3] = colors[i].x;
            posbuffer[i*7 + 4] = colors[i].y;
            posbuffer[i*7 + 5] = colors[i].z;
            
            posbuffer[i*7 + 6] = 0.1;
        }
        
        gr.buffer.buffer.sync();
        
        gr.render(OBJECTS, WINDOW_WIDTH, WINDOW_HEIGHT);
        
        window.flip();
        
        steady_clock::time_point thisTime = steady_clock::now();
        cout << "FPS: " << (1.0/duration_cast<duration<double>>(thisTime - lastTime).count()) << endl;
        lastTime = thisTime;
    }
    return 0;
}

