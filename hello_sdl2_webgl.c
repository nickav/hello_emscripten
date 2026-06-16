//
// emcc app.c -o build/app.html -s USE_SDL=2 -s MIN_WEBGL_VERSION=2 --shell-file minimal_shell.html -s ENVIRONMENT=web -s ASSERTIONS=0 --closure=1
//
#include <SDL2/SDL.h>
#include <GLES3/gl3.h>
#include <emscripten.h>

SDL_Window* window = NULL;
SDL_GLContext context;

void main_loop() {
    // 1. Clear the screen with a color (Cornflower Blue)
    glClearColor(0.392f, 0.584f, 0.929f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 2. Present the buffer to the WebGL Canvas
    SDL_GL_SwapWindow(window);
}

int main(int argc, char* argv[]) {
    // 1. Initialize SDL2
    SDL_Init(SDL_INIT_VIDEO);

    // 2. Create the WebGL window
    window = SDL_CreateWindow("WebGL Hello World", 
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                              800, 600, SDL_WINDOW_OPENGL);

    // 3. Create the OpenGL ES / WebGL context
    context = SDL_GL_CreateContext(window);

    // 4. Emscripten main loop wrapper
    emscripten_set_main_loop(main_loop, 0, 1);

    return 0;
}
