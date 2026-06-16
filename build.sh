#!/bin/bash
set -e

project_root="$(cd "$(dirname "$0")" && pwd -P)"

pushd $project_root
    mkdir -p build

    # can also use -O3
    # and --closure=1 for optimizing the JS

    ntime emcc hello_sdl.c -o build/hello.html --shell-file minimal_shell.html -s ENVIRONMENT=web -s ASSERTIONS=0 -O0

    ntime emcc hello_webgl.c -o build/webgl.html -s USE_SDL=2 -s MIN_WEBGL_VERSION=2 --shell-file minimal_shell.html -s ENVIRONMENT=web -s ASSERTIONS=0

    ntime emcc hello_webgpu.c -o build/webgpu.html --shell-file minimal_shell.html --use-port=emdawnwebgpu -s ENVIRONMENT=web -s ASSERTIONS=0 -O0

    (sleep 0.1 && open http://localhost:8080/webgpu.html) &
    python -m http.server 8080 --directory build
popd