#include "window.h"
#include <iostream>

Window::Window(const std::string &name, unsigned width, unsigned height) {
    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to init SDL: " << SDL_GetError() << std::endl;
        exit(1);
    }
    
    
    
    window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    
    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        exit(1);
    }
}

Window::~Window() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Window::pollEvent(SDL_Event &event) {
    return SDL_PollEvent(&event);
}

void Window::flip() {
    SDL_GL_SwapWindow(window);
}

