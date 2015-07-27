#include "draw.h"
#include <iostream>
#include <GL/glu.h>

ShaderProgram::ShaderProgram() {
    program = glCreateProgram();
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(program);
}

Shader& ShaderProgram::addShader(GLenum type, const std::string &code) {
    shaders.emplace_front(type, code);
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
        std::cerr << "Failed to link program:" << std::endl;
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

Shader::Shader(GLenum type, const std::string &code) {
    shader = glCreateShader(type);
    
    GLchar *source = (GLchar*)code.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        std::cerr << "Failed to compile vertex shader:" << std::endl;
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


