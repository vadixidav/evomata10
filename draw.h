#ifndef DRAW_H
#define DRAW_H

#include "group.h"
#include "window.h"
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <list>

struct Shader {
    GLuint shader;
    
    //Type is either GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
    Shader(GLenum type, const std::string &code);
    ~Shader();
};

struct BO {
    GLuint bo;
    GLenum target;
    GLsizeiptr size;
    
    GLintptr mappedOffset;
    GLsizeiptr mappedLength;
    
    BO(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
    ~BO();
    
    void* map(GLbitfield access);
    void* map(GLintptr offset, GLsizeiptr length, GLbitfield access);
    void sync(); //Only call this if GL_MAP_FLUSH_EXPLICIT_BIT was used in access
    void unmap();
    
    void bind();
};

struct UBO {
    BO bo;
    GLuint index;
    
    UBO(GLsizeiptr size, const GLvoid *data, GLenum usage, GLuint index);
    
    void bind();
    //Use this if multiple programs use the same UBO
    void bind(GLuint index);
};

struct Texture {
    GLuint texture;
    GLenum target;
    
    Texture(GLenum target);
    ~Texture();
    
    void setParam(GLenum pname, GLint param);
    void setParam(GLenum pname, GLfloat param);
    
    void bind();
    void unbind();
};

struct RBO {
    GLuint id;
    
    RBO(GLenum format, GLsizei width, GLsizei height);
    ~RBO();
    
    void bind();
};

struct FBO {
    GLuint fbo;
    Texture colors;
    
    FBO(GLenum colorsTarget);
    ~FBO();
    
    void bind();
    
    //Bind main screen
    static void bindDefault();
};

struct VAO {
    GLuint vao;
    std::list<BO> bos;
    
    //This also binds the VAO (it is presumed that the user intends to add stuff to it immediately)
    VAO();
    ~VAO();
    
    //When adding to the VAO, it still binds it before
    BO& addVertexBuffer(GLsizeiptr size, const GLvoid *data, GLenum usage);
    BO& addIndexBuffer(GLsizeiptr size, const GLvoid *data, GLenum usage);
    void addAttribute(BO& bo, GLuint attribute, GLint dimensions, GLenum datatype);
    void bind();
    void unbind();
};

struct ShaderProgram {
    GLuint program;
    
    std::list<Shader> shaders;
    
    ShaderProgram();
    ~ShaderProgram();
    
    Shader& addShader(GLenum type, const std::string &code);
    void link();
    
    GLuint getAttributeLocation(const std::string &name);
    GLint getUniformLocation(const std::string &name);
    GLuint getUniformBlockIndex(const std::string &name);
    
    GLint getUniformInt(const std::string &name);
    GLuint getUniformUnsigned(const std::string &name);
    
    void setUniform(GLint location, GLint value);
    void setUniform(GLint location, GLfloat value);
    
    void setUniform(GLint location, GLfloat value1, GLfloat value2, GLfloat value3);
    
    void setUniform(const std::string &name, GLint value);
    void setUniform(const std::string &name, GLfloat value);
    
    void setUniform(const std::string &name, GLfloat value1, GLfloat value2, GLfloat value3);
    
    void use();
};

class Renderer {
    SDL_GLContext glContext;
    
public:
    Renderer(Window &window);
    ~Renderer();
    
    void bind(Window &window);
    
    void drawGroup(Group &g);
};

#endif // DRAW_H

