#!/bin/sh
gcc game.c window.c import.c shader.c util.c math3d.c camera.c transform.c texture.c player.c lighting.c -lglfw -lGLU -lGLEW -lGL -lm -o adventure
