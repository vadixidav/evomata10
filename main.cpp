#include <iostream>
#include "gpi/gpi.h"
#include "phitron/phitron.h"
#include "window.h"
#include "draw.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

//Amount of particles in test simulation
#define OBJECTS 256

//The maximum amount of things to be drawn each iteration of orbProgram
//This must be the same as DRAWABLES in the fragment shader
#define DRAWABLES 2048

using namespace std;

int main() {
    Window window("testing", WINDOW_WIDTH, WINDOW_HEIGHT);
    Renderer renderer(window);
    
    ShaderProgram orbProgram;
        orbProgram.addShader(GL_VERTEX_SHADER,
            R"(#version 300 es
            in vec2 pos;
            smooth out vec2 inter;
            uniform float ratio;
        
            void main() {
                inter = pos;
                inter.x *= ratio;
                gl_Position = vec4(pos.x, pos.y, 0, 1);
            })");
        orbProgram.addShader(GL_FRAGMENT_SHADER,
            R"(#version 300 es
            #define DRAWABLES 2048
            precision highp float;
            smooth in vec2 inter;
            out vec4 color;
            uniform vec3 orbs[DRAWABLES * 2];
            uniform int count;
            
            void main() {
                color = vec4(0.0, 0.0, 0.0, 1.0);
                int i;
                for (i = 0; i != count; i++) {
                    vec3 delta = vec3(orbs[i*2].x - inter.x, orbs[i*2].y - inter.y, orbs[i*2].z);
                    float dis = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                    if (dis < 0.1 * 0.15) {
                        float mag = (0.1 - sqrt(dis));
                        color.rgb += orbs[i*2+1] * mag;
                    }
                }
            })");
    orbProgram.link();
    
    ShaderProgram screenProgram;
        screenProgram.addShader(GL_VERTEX_SHADER,
            R"(#version 300 es
            in vec2 pos;
            smooth out vec2 inter;
            void main() {
                inter = pos;
                inter += vec2(1.0, 1.0);
                inter /= vec2(2.0, 2.0);
                gl_Position = vec4(pos.x, pos.y, 0, 1);
            })");
        screenProgram.addShader(GL_FRAGMENT_SHADER,
            R"(#version 300 es
            precision highp float;
            smooth in vec2 inter;
            out vec4 color;
            uniform float gamma;
            uniform sampler2D linear;
            
            void main() {
                color.rgb = pow(vec3(texture(linear, inter).rgb), vec3(1.0/gamma));
                color.a = 1.0;
            })");
    screenProgram.link();
    
    const float vertices[] = {1, 1, -1, 1, -1, -1, 1, -1};
    
    VAO orbVAO;
        orbVAO.addAttribute(orbVAO.addVertexBuffer(sizeof(vertices), vertices, GL_STATIC_DRAW),
                            orbProgram.getAttributeLocation("pos"), 2, GL_FLOAT);
        FBO orbFrame(GL_TEXTURE_2D);
        orbFrame.bind();
        orbFrame.colors.setParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        orbFrame.colors.setParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        orbFrame.colors.bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, orbFrame.colors.texture, 0);
    orbVAO.unbind();
    
    VAO screenVAO;
        screenVAO.addAttribute(screenVAO.addVertexBuffer(sizeof(vertices), vertices, GL_STATIC_DRAW),
                               screenProgram.getAttributeLocation("pos"), 2, GL_FLOAT);
    screenVAO.unbind();
    
    screenProgram.setUniform("gamma", 2.2f);
    screenProgram.setUniform("linear", 0);
    orbProgram.setUniform("ratio", float(WINDOW_WIDTH)/float(WINDOW_HEIGHT));
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
    GLint orbsLocation = orbProgram.getUniformLocation("orbs");
    GLint countLocation = orbProgram.getUniformLocation("count");
    
    phi::P3 particles[OBJECTS];
    phi::V3 colors[OBJECTS];
    
    std::mt19937 rand(3);
    
    for (unsigned i = 0; i != OBJECTS; i++) {
        particles[i].position = phi::V3(rand()/double(rand.max())*2-1, rand()/double(rand.max())*2-1, 0.0);
        particles[i].velocity = phi::V3(rand()/double(rand.max())*2-1, rand()/double(rand.max())*2-1, rand()/double(rand.max())*2-1);
        particles[i].velocity *= 0.0005;
        particles[i].inertia = 100.0;
        
        colors[i] = phi::V3(rand()/double(rand.max()), rand()/double(rand.max()), rand()/double(rand.max()));
    }
    
    float posbuffer[OBJECTS*6];
    
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
            
            posbuffer[i*6 + 0] = particles[i].position.x;
            posbuffer[i*6 + 1] = particles[i].position.y;
            posbuffer[i*6 + 2] = particles[i].position.z;
            
            posbuffer[i*6 + 3] = colors[i].x;
            posbuffer[i*6 + 4] = colors[i].y;
            posbuffer[i*6 + 5] = colors[i].z;
        }
        
        //Draw orbs
        orbProgram.use();
        orbFrame.bind();
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        orbVAO.bind();
            for (int i = 0; i != (OBJECTS-1)/DRAWABLES + 1; i++) {
                int amount = i * DRAWABLES > OBJECTS ? DRAWABLES : OBJECTS - i * DRAWABLES;
                glUniform3fv(orbsLocation, amount * 2, posbuffer + 6*DRAWABLES*i);
                orbProgram.setUniform(countLocation, amount);
                glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
            }
        orbVAO.unbind();
        
        //Draw screen
        screenProgram.use();
        FBO::bindDefault();
        glActiveTexture(GL_TEXTURE0);
        orbFrame.colors.bind();
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        screenVAO.bind();
            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        screenVAO.unbind();
        
        window.flip();
    }
    return 0;
}

