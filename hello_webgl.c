//
// emcc hello_webgl.c -o build/webgl.html -s USE_SDL=2 -s MIN_WEBGL_VERSION=2 --shell-file minimal_shell.html -s ENVIRONMENT=web -s ASSERTIONS=0 --closure=1
//
#include <stdio.h>
#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx;

void main_loop(void) {
    glClearColor(0.392f, 0.584f, 0.929f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    emscripten_webgl_commit_frame();
}

int main(void) {
    printf("hello from emscripten app\n");

    emscripten_set_canvas_element_size("#canvas", 800, 600);

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;

    ctx = emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(ctx);

    emscripten_set_main_loop(main_loop, 0, 1);
    return 0;
}