#include <iostream>
#include "gpi/gpi.h"
#include "phitron/phitron.h"
#include "window.h"
#include "draw.h"
#include <chrono>
#include <thread>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

//Amount of particles in test simulation
#define OBJECTS 128

#define FPS 20

using namespace std;
using namespace chrono;

uint16_t toHalfFloat(float x) {
    uint32_t &f = reinterpret_cast<uint32_t&>(x);
    return ((f >> 16) & 0x8000) | ((((f & 0x7f800000) - 0x38000000) >> 13) & 0x7c00) | ((f >> 13) & 0x03ff);
}

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
    
    uint16_t *posbuffer = (uint16_t*)gr.buffer.buffer.map(GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
    
    while (true) {
        SDL_Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case SDL_QUIT:
                return 0;
            }
        }
        
        #pragma omp parallel for
        for (unsigned i = 0; i < OBJECTS; i++)
            for (unsigned j = 0; j != OBJECTS; j++)
                if (i != j)
                    particles[i].gravitate(0.0001, particles[j].position, 0.1);
        
        for (unsigned i = 0; i != OBJECTS; i++) {
            particles[i].advance();
            posbuffer[i*7 + 0] = toHalfFloat(particles[i].position.x);
            posbuffer[i*7 + 1] = toHalfFloat(particles[i].position.y);
            posbuffer[i*7 + 2] = toHalfFloat(particles[i].position.z);
            
            posbuffer[i*7 + 3] = toHalfFloat(colors[i].x);
            posbuffer[i*7 + 4] = toHalfFloat(colors[i].y);
            posbuffer[i*7 + 5] = toHalfFloat(colors[i].z);
            
            posbuffer[i*7 + 6] = toHalfFloat(0.1);
        }
        
        gr.buffer.buffer.sync();
        
        gr.render(OBJECTS, WINDOW_WIDTH, WINDOW_HEIGHT);
        
        this_thread::sleep_until(lastTime + duration<double>(1.0/FPS));
        steady_clock::time_point thisTime = steady_clock::now();
        double timeDelta = duration_cast<duration<double>>(thisTime - lastTime).count();
        cout << "FPS: " << (1.0/timeDelta) << endl;
        lastTime = thisTime;
        
        window.flip();
    }
    return 0;
}

