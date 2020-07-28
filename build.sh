#!/bin/sh
gcc game.c \
    window.c \
    mesh.c \
    shader.c \
    util.c \
    math3d.c \
    camera.c \
    transform.c \
    texture.c \
    player.c \
    sky.c \
    lighting.c \
    terrain.c \
    socket.c \
    net.c \
    timer.c \
    -lglfw -lGLU -lGLEW -lGL -lm \
    -o adventure
