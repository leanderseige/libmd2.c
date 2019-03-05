#!/bin/bash

# simple compile script for the libmd2.c sample applications
# change it to make it work on your system
# GNU GPL (c) 2005, Leander Seige

gcc md2view.c -o md2view -lSDL $(sdl-config --libs --cflags) -lGL -lGLU -lglut -lSDL_image -lX11 -lXext -lXmu -lXi -lm -L/usr/X11R6/lib -w
gcc md2info.c -o md2info -lGL -lSDL_image $(sdl-config --libs --cflags) -lm -w
