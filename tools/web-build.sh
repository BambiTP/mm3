#!/usr/bin/env sh
# Build the browser (WebAssembly) version into build-web/dist/.
# Needs Emscripten on PATH (source emsdk_env.sh first).
set -e
cd "$(dirname "$0")/.."
emcmake cmake -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
(cd build-web && ctest --output-on-failure)
mkdir -p build-web/dist
cp build-web/index.html build-web/index.js build-web/index.wasm build-web/dist/
echo "Web build ready in build-web/dist/"
