# Adventure3d

This is a work-in-progress 3d game.

![Preview](/screenshots/preview.png)

## Build and Run

```bash

    # Make sure you have GLFW (X11) and GLEW installed
    sudo pacman -S --needed glfw glew

    ./build.sh
    ./adventure
```

## Host Server

```bash
    ./server.sh
```

## Join Server

Public server

```bash
    ./client.sh
```

Local server

```bash
    ./client_local.sh
```

## Controls

```
    w = Move Forward
    a = Strafe Left
    s = Move Backwards
    d = Strafe Right

    mouse movement  = look around

    shift = run
    space = Jump

    tab = toggle wireframe

    r = Toggle camera between 1st and 3rd person
    m = Toggle camera mode (free or follow player)

    arrow-up   = increase ambient brightness
    arrow-down = decrease ambient brightness
```
