#include <iostream>
#include "gpi/gpi.h"
#include "phitron/phitron.h"
#include "window.h"
#include "group.h"
#include "draw.h"
#include <chrono>
#include <thread>

#define WINDOW_WIDTH 400
#define WINDOW_HEIGHT 400
#define CLOSENESS 20.0

#define FPS 1
#define CYCLES 8

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
    
    Group group(phi::V3(1.0, 1.0, 1.0), 1743);
    
    uint16_t *posbuffer = (uint16_t*)gr.buffer.buffer.map(GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT);
    
    uint64_t cycle = 0;
    
    while (true) {
        SDL_Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case SDL_QUIT:
                return 0;
            }
        }
        
        for (unsigned i = 0; i != CYCLES; i++) {
            group.spawn(group.rand() % 16 == 0);
            //group.spawn(4);
            group.update();
        }
        
        steady_clock::time_point betweenTime = steady_clock::now();
        
        if (group.cells.size() > DRAWABLES) {
            std::cerr << "Error: Exceeded max drawables!" << std::endl;
            return 1;
        }
        
        {
            unsigned index;
            auto i = group.cells.begin();
            for (index = 0; i != group.cells.end(); i++, index++) {
                Cell &c = *i;
                posbuffer[index * 7 + 0] = toHalfFloat(c.particle.position.x);
                posbuffer[index * 7 + 1] = toHalfFloat(c.particle.position.y);
                posbuffer[index * 7 + 2] = toHalfFloat(c.particle.position.z / CLOSENESS);
                
                posbuffer[index * 7 + 3] = toHalfFloat(((c.species & 0xFF << 0) >> 0) / double(0xFF));
                posbuffer[index * 7 + 4] = toHalfFloat(((c.species & 0xFF << 8) >> 8) / double(0xFF));
                posbuffer[index * 7 + 5] = toHalfFloat(((c.species & 0xFF << 16) >> 16) / double(0xFF));
                
                posbuffer[index * 7 + 6] = toHalfFloat(0.1);
            }
        }
        
        gr.buffer.buffer.sync();
        
        gr.render(group.cells.size(), WINDOW_WIDTH, WINDOW_HEIGHT);
        
        this_thread::sleep_until(lastTime + duration<double>(1.0/FPS));
        steady_clock::time_point thisTime = steady_clock::now();
        window.flip();
        
        cout << "\nCycle: " << cycle << endl;
        cout << "Count: " << group.cells.size() << endl;
        double timeDelta = duration_cast<duration<double>>(thisTime - lastTime).count();
        cout << "FPS: " << (1.0/timeDelta) << endl;
        double updateDelta = duration_cast<duration<double>>(betweenTime - lastTime).count();
        cout << "Update duration: " << updateDelta << endl;
        double renderDelta = duration_cast<duration<double>>(thisTime - betweenTime).count();
        cout << "Render duration: " << renderDelta << endl;
        
        lastTime = thisTime;
        cycle++;
    }
    return 0;
}

