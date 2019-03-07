# rm *bmp
# gcc md2demo.c -o md2demo -lSDL $(sdl-config --libs --cflags) -lGL -lGLU -lglut -lSDL_image -lX11 -lXext -lXmu -lXi -lm -L/usr/X11R6/lib -w
# ./md2demo model/ratamahatta.md2 model/ratamahatta.png 
# convert -delay 4 -loop 0 -crop 1280x720+0+152 +repage -resize 60% -flip *bmp animation.gif

