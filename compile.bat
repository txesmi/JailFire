emcc -o bin/JailFire.html TxStack.c TxQuad.c TxControls.c TxGame.c -Os -Wall bin/raylib/libraylib.a -I. -I../raylib/src/ -s USE_GLFW=3 --shell-file minshell.html -DPLATFORM_WEB --preload-file resources/