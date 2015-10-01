#ifndef DRAW_H
#define DRAW_H

#include "group.h"
#include <drew/draw.h>

struct GroupRenderer {
    ShaderProgram orbProgram;
    ShaderProgram screenProgram;
    VAO orbVAO;
    VAO screenVAO;
    FBO orbFrame;
    GLint orbsLocation;
    GLint countLocation;
    TBO buffer;
    
public:
    GroupRenderer(unsigned width, unsigned height);
    
    void render(unsigned total, unsigned width, unsigned height);
};

#endif // DRAW_H

