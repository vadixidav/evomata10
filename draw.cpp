#include "draw.h"
#include <iostream>
#include <GL/glu.h>

ShaderProgram::ShaderProgram(const std::string &name) : name(name) {
    program = glCreateProgram();
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(program);
}

Shader& ShaderProgram::addShader(GLenum type, const std::string &name, const std::string &code) {
    shaders.emplace_front(type, name, this->name, code);
    glAttachShader(program, shaders.front().shader);
    return shaders.front();
}

GLuint ShaderProgram::getAttributeLocation(const std::string &name) {
    GLint attr = glGetAttribLocation(program, name.c_str());
    if (attr == -1) {
        std::cerr << "Failed to find shader attribute: " << name << std::endl;
        exit(1);
    }
    return attr;
}

GLint ShaderProgram::getUniformLocation(const std::string &name) {
    GLint unif = glGetUniformLocation(program, name.c_str());
    if (unif == -1) {
        std::cerr << "Failed to find shader uniform: " << name << std::endl;
        exit(1);
    }
    return unif;
}

GLuint ShaderProgram::getUniformBlockIndex(const std::string &name) {
    GLuint unif = glGetUniformBlockIndex(program, name.c_str());
    if (unif == GL_INVALID_INDEX) {
        std::cerr << "Failed to find shader uniform block: " << name << std::endl;
        exit(1);
    }
    return unif;
}

GLint ShaderProgram::getUniformInt(const std::string &name) {
    GLint value;
    glGetUniformiv(program, getUniformLocation(name), &value);
    return value;
}

GLuint ShaderProgram::getUniformUnsigned(const std::string &name) {
    GLuint value;
    glGetUniformuiv(program, getUniformLocation(name), &value);
    return value;
}

void ShaderProgram::setUniform(const std::string &name, GLint value) {
    glProgramUniform1i(program, getUniformLocation(name), value);
}

void ShaderProgram::setUniform(const std::string &name, GLfloat value) {
    glProgramUniform1f(program, getUniformLocation(name), value);
}

void ShaderProgram::setUniform(const std::string &name, GLfloat value1, GLfloat value2, GLfloat value3) {
    glProgramUniform3f(program, getUniformLocation(name), value1, value2, value3);
}

void ShaderProgram::setUniform(GLint location, GLint value) {
    glProgramUniform1i(program, location, value);
}

void ShaderProgram::setUniform(GLint location, GLfloat value) {
    glProgramUniform1f(program, location, value);
}

void ShaderProgram::setUniform(GLint location, GLfloat value1, GLfloat value2, GLfloat value3) {
    glProgramUniform3f(program, location, value1, value2, value3);
}

void ShaderProgram::link() {
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        std::cerr << "Failed to link program: " << name << std::endl;
        if (!glIsProgram(program)) {
            std::cerr << "Failed to get program linker errors." << std::endl;
            exit(1);
        }
        
        int maxLength;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
        char *log = new char[maxLength];
        
        glGetProgramInfoLog(program, maxLength, nullptr, log);
        std::cerr << log << std::endl;
        
        delete [] log;
        exit(1);
    }
    
    for (Shader &s : shaders)
        //Detatch all shaders so they can be freed
        glDetachShader(program, s.shader);
    
    //Clear the shader list in case link is called again for some reason
    shaders.clear();
}

void ShaderProgram::use() {
    glUseProgram(program);
}

Shader::Shader(GLenum type, const std::string &name, const std::string &progName, const std::string &code) {
    shader = glCreateShader(type);
    
    GLchar *source = (GLchar*)code.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        std::cerr << "Failed to compile shader \"" << name << "\" from program \"" << progName << "\":" << std::endl;
        if (!glIsShader(shader)) {
            std::cerr << "Failed to get shader compile error." << std::endl;
            exit(1);
        }
        
        int maxLength;
        
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char *log = new char[maxLength];
        
        glGetShaderInfoLog(shader, maxLength, nullptr, log);
        std::cerr << log << std::endl;
        
        delete [] log;
        exit(1);
    }
}

Shader::~Shader() {
    glDeleteShader(shader);
}

Texture::Texture(GLenum target) : target(target) {
    glGenTextures(1, &texture);
    //We must bind so that the texture will error if it is rebound to another target
    bind();
}

Texture::~Texture() {
    glDeleteTextures(1, &texture);
}

void Texture::setParam(GLenum pname, GLfloat param) {
    bind();
    glTexParameterf(target, pname, param);
}

void Texture::setParam(GLenum pname, GLint param) {
    bind();
    glTexParameteri(target, pname, param);
}

void Texture::bind() {
    glBindTexture(target, texture);
}

void Texture::unbind() {
    glBindTexture(target, 0);
}

RBO::RBO(GLenum format, GLsizei width, GLsizei height) {
    glGenRenderbuffers(1, &id);
    bind();
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
}

RBO::~RBO() {
    glDeleteRenderbuffers(1, &id);
}

void RBO::bind() {
    glBindRenderbuffer(GL_RENDERBUFFER, id);
}

FBO::FBO(GLenum colorsTarget) : colors(colorsTarget) {
    glGenFramebuffers(1, &fbo);
}

FBO::~FBO() {
    glDeleteFramebuffers(1, &fbo);
}

void FBO::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void FBO::bindDefault() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BO::BO(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) : target(target), size(size) {
    glGenBuffers(1, &bo);
    bind();
    glBufferData(target, size, data, usage);
}

BO::~BO() {
    glDeleteBuffers(1, &bo);
}

void* BO::map(GLbitfield access) {
    bind();
    mappedOffset = 0;
    mappedLength = size;
    return glMapBufferRange(target, 0, size, access);
}

void* BO::map(GLintptr offset, GLsizeiptr length, GLbitfield access) {
    bind();
    mappedOffset = offset;
    mappedLength = length;
    return glMapBufferRange(target, offset, length, access);
}

void BO::sync() {
    bind();
    glFlushMappedBufferRange(target, mappedOffset, mappedLength);
}

void BO::unmap() {
    bind();
    glUnmapBuffer(target);
}

void BO::bind() {
    glBindBuffer(target, bo);
}

UBO::UBO(GLsizeiptr size, const GLvoid *data, GLenum usage, GLuint index) :
         bo(GL_UNIFORM_BUFFER, size, data, usage), index(index) {
}

void UBO::bind() {
    glBindBufferBase(GL_UNIFORM_BUFFER, index, bo.bo);
}

void UBO::bind(GLuint index) {
    glBindBufferBase(GL_UNIFORM_BUFFER, index, bo.bo);
}

TBO::TBO(GLsizeiptr size, GLenum format, const GLvoid *data, GLenum usage) : texture(GL_TEXTURE_BUFFER),
         buffer(GL_ARRAY_BUFFER, size, data, usage) {
    texture.bind();
    glTexBuffer(GL_TEXTURE_BUFFER, format, buffer.bo);
}

VAO::VAO() {
    glGenVertexArrays(1, &vao);
    bind();
}

VAO::~VAO() {
    glDeleteVertexArrays(1, &vao);
}

BO& VAO::addVertexBuffer(GLsizeiptr size, const GLvoid *data, GLenum usage) {
    bind();
    bos.emplace_front(GL_ARRAY_BUFFER, size, data, usage);
    return bos.front();
}

BO& VAO::addIndexBuffer(GLsizeiptr size, const GLvoid *data, GLenum usage) {
    bind();
    bos.emplace_front(GL_ELEMENT_ARRAY_BUFFER, size, data, usage);
    return bos.front();
}

void VAO::addAttribute(BO& bo, GLuint attribute, GLint dimensions, GLenum datatype) {
    bind();
    bo.bind();
    glVertexAttribPointer(attribute, dimensions, datatype, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(attribute);
}

void VAO::bind() {
    glBindVertexArray(vao);
}

void VAO::unbind() {
    glBindVertexArray(0);
}

Renderer::Renderer(Window &window) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    glContext = SDL_GL_CreateContext(window.window);
    
    if (!glContext) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << std::endl;
        exit(1);
    }
    
    glewExperimental = GL_TRUE; //Link all opengl extensions possible
    auto error = glewInit();
    if (error != GLEW_OK) {
        std::cerr << "Failed to init GLEW: " << glewGetErrorString(error) << std::endl;
        exit(1);
    }
    
    if (SDL_GL_SetSwapInterval(1) < 0) {
        std::cerr << "Failed to enable VSync: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

Renderer::~Renderer() {
    SDL_GL_DeleteContext(glContext);
}

void Renderer::bind(Window &window) {
    if (SDL_GL_MakeCurrent(window.window, glContext)) {
        std::cerr << "Failed to assign GL context: " << SDL_GetError() << std::endl;
    }
}

GroupRenderer::GroupRenderer(unsigned width, unsigned height) : orbProgram("Orbs"), screenProgram("Screen"),
                             orbFrame(GL_TEXTURE_2D), buffer(DRAWABLES*7*2, GL_R16F, nullptr, GL_STREAM_DRAW) {
    orbProgram.addShader(GL_VERTEX_SHADER, "Vertex Main",
R"(#version 140
in vec2 pos;
smooth out vec2 inter;
uniform float ratio;

void main() {
    inter = pos;
    inter.x *= ratio;
    gl_Position = vec4(pos.x, pos.y, 0, 1);
})");
    orbProgram.addShader(GL_FRAGMENT_SHADER, "Fragment Main",
R"(#version 140
precision highp float;
smooth in vec2 inter;
out vec4 finalColor;
uniform samplerBuffer orbs;
uniform int count;

void main() {
    vec3 color = vec3(0.0, 0.0, 0.0);
    int i;
    for (i = 0; i != count; i++) {
        vec3 delta = vec3(texelFetch(orbs, i*7+0).r - inter.x,
                          texelFetch(orbs, i*7+1).r - inter.y,
                          texelFetch(orbs, i*7+2).r);
        float dis2DSquared = delta.x * delta.x + delta.y * delta.y;
        float zSquared = delta.z * delta.z;
        float dis3DSquared = dis2DSquared + zSquared;
        float radius = texelFetch(orbs, i*7+6).r;
        if (dis3DSquared < radius * radius) {
            vec3 orbColor = vec3(texelFetch(orbs, i*7+3).r,
                                 texelFetch(orbs, i*7+4).r,
                                 texelFetch(orbs, i*7+5).r);
            //Adjusted for ring
            float radius2DSquared = radius * radius - zSquared;
            float radius2DSquaredAdjusted = radius2DSquared * 0.5;
            if (dis2DSquared < radius2DSquaredAdjusted)
                color += orbColor * (sqrt(radius2DSquaredAdjusted) - sqrt(dis2DSquared)) / radius;
            else {
                float radius2D = sqrt(radius2DSquared);
                float adjusted2D = sqrt(radius2DSquaredAdjusted);
                //Get positive distance from adjusted radius
                float disAdjusted =
                        0.5 * abs(abs(2.0 * sqrt(dis2DSquared) - radius2D - adjusted2D) + adjusted2D - radius2D);
                color -= orbColor * disAdjusted / radius;
            }
        }
    }
    
    finalColor.rgb = color;
})");
    orbProgram.link();
    
    screenProgram.addShader(GL_VERTEX_SHADER, "Vertex Main",
R"(#version 140
in vec2 pos;
smooth out vec2 inter;
void main() {
    inter = pos;
    inter += vec2(1.0, 1.0);
    inter /= vec2(2.0, 2.0);
    gl_Position = vec4(pos.x, pos.y, 0, 1);
})");
    screenProgram.addShader(GL_FRAGMENT_SHADER, "Fragment Main",
R"(#version 140
precision highp float;
smooth in vec2 inter;
out vec4 color;
uniform float gamma;
uniform sampler2D linear;

void main() {
    color.rgb = pow(abs(texture(linear, inter).rgb), vec3(1.0/gamma, 1.0/gamma, 1.0/gamma));
    color.a = 1.0;
})");
    screenProgram.link();
    
    const float vertices[] = {1, 1, -1, 1, -1, -1, 1, -1};
    
    orbVAO.bind();
    orbVAO.addAttribute(orbVAO.addVertexBuffer(sizeof(vertices), vertices, GL_STATIC_DRAW),
                        orbProgram.getAttributeLocation("pos"), 2, GL_FLOAT);
    orbVAO.unbind();
    
    screenVAO.bind();
    screenVAO.addAttribute(screenVAO.addVertexBuffer(sizeof(vertices), vertices, GL_STATIC_DRAW),
                           screenProgram.getAttributeLocation("pos"), 2, GL_FLOAT);
    screenVAO.unbind();
    
    orbFrame.bind();
    orbFrame.colors.setParam(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    orbFrame.colors.setParam(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    orbFrame.colors.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_INT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, orbFrame.colors.texture, 0);
    
    screenProgram.setUniform("gamma", 2.2f);
    screenProgram.setUniform("linear", 0);
    orbProgram.setUniform("ratio", float(width)/float(height));
    
    /*
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);*/
    
    orbsLocation = orbProgram.getUniformLocation("orbs");
    countLocation = orbProgram.getUniformLocation("count");
}

void GroupRenderer::render(unsigned total, unsigned width, unsigned height) {
    //Draw orbs
    //FBO::bindDefault();
    orbProgram.use();
    orbFrame.colors.unbind();
    orbFrame.bind();
    glActiveTexture(GL_TEXTURE0);
    buffer.texture.bind();
    glViewport(0, 0, width, height);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    orbVAO.bind();
        orbProgram.setUniform(countLocation, (int)total);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    orbVAO.unbind();
    
    //Draw screen
    
    screenProgram.use();
    FBO::bindDefault();
    glActiveTexture(GL_TEXTURE0);
    orbFrame.colors.bind();
    glViewport(0, 0, width, height);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    screenVAO.bind();
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    screenVAO.unbind();
}
