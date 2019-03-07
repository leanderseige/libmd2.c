/********************************************************************************
    md2view.c - a sample application for libmd2.c

    Version 1.0

    (c) 2005 Leander Seige, www.determinate.net/webdata/seg/snippets.html
    contact: snippets@determinate.net

    RELEASED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE (GPL) V3
    see www.determinate.net/webdata/seg/COPYING for more

    Read the included file README for more.
 ********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <GL/glut.h>
#include <SDL.h>

#include "libmd2.c"

#ifdef PI
#undef PI
#endif

#define PI 3.1415926535897932384626433832795

#define SCREENW 1280
#define SCREENH 1024
#define SCREENF SDL_OPENGL // |SDL_FULLSCREEN

int bpp=0,rgb_size[3];
const SDL_VideoInfo* info = NULL;
SDL_Event event;
SDL_Surface *window;
Uint32 time_now=0,time_next=0;

GLfloat light0_pos[]={0,-20,0,0};
GLfloat f100[]={1,1,1,1};
GLfloat f025[]={.25,.25,.25,1};

void draw_bbox (struct md2_boundingbox * bb) {
    glBegin(GL_LINES);
	glVertex3f(bb->x1,bb->y1,bb->z1);
	glVertex3f(bb->x1,bb->y1,bb->z2);
	glVertex3f(bb->x1,bb->y2,bb->z1);
	glVertex3f(bb->x1,bb->y2,bb->z2);
	glVertex3f(bb->x2,bb->y1,bb->z1);
	glVertex3f(bb->x2,bb->y1,bb->z2);
	glVertex3f(bb->x2,bb->y2,bb->z1);
	glVertex3f(bb->x2,bb->y2,bb->z2);

	glVertex3f(bb->x1,bb->y1,bb->z1);
	glVertex3f(bb->x1,bb->y2,bb->z1);
	glVertex3f(bb->x1,bb->y1,bb->z2);
	glVertex3f(bb->x1,bb->y2,bb->z2);
	glVertex3f(bb->x2,bb->y1,bb->z1);
	glVertex3f(bb->x2,bb->y2,bb->z1);
	glVertex3f(bb->x2,bb->y1,bb->z2);
	glVertex3f(bb->x2,bb->y2,bb->z2);

	glVertex3f(bb->x1,bb->y1,bb->z1);
	glVertex3f(bb->x2,bb->y1,bb->z1);
	glVertex3f(bb->x1,bb->y1,bb->z2);
	glVertex3f(bb->x2,bb->y1,bb->z2);
	glVertex3f(bb->x1,bb->y2,bb->z1);
	glVertex3f(bb->x2,bb->y2,bb->z1);
	glVertex3f(bb->x1,bb->y2,bb->z2);
	glVertex3f(bb->x2,bb->y2,bb->z2);
    glEnd();
}

void printinfo() {
    printf("md2view <path/model.md2> <path/texture.pcx>\n");
    printf("Keys:   'T' - toggle texturing \n");
    printf("        'B' - toggle boundingbox \n");
    printf("        'R' - toggle rotation \n");
    printf("  Cursor up - Gamma + \n");
    printf("       down - Gamma - \n");
    printf("        'L' - toggle looping\n");
    printf("    <space> - cycle through display modes\n");
    printf("    Esc/'Q' - exit\n");

    printf("\nswitch to facenormals\n");
}

void Screenshot(int f)
{
    unsigned char pixels[SCREENW*SCREENH*4]; // 4 bytes for RGBA
    unsigned char fn[100];
    glReadPixels(0, 0, SCREENW, SCREENH, GL_BGRA, GL_UNSIGNED_BYTE, pixels);

    SDL_Surface * surf = SDL_CreateRGBSurfaceFrom(pixels, SCREENW, SCREENH, 8*4, SCREENW*4, 0,0,0,0);
    snprintf(fn,100,"%02d.bmp",f);
    SDL_SaveBMP(surf, fn);

    SDL_FreeSurface(surf);
}

int main (int argc, char** argv)
{
    struct md2_model *mymodel;
    struct md2_texture *mytex;
    int displaymode,animation,rotate,show_bb,show_texture,quit,looping,sf,ef;
    double scale,gamma;
    double spin=0.0;
    struct md2_boundingbox bb;

    if(argc!=3) {
        printf("Usage: ./md2view <path/modelfile.md2> <path/texture.pcx>\n"); exit(1);
    }

    if(SDL_Init(SDL_INIT_VIDEO)<0) {
	printf("SDL ERROR Video initialization failed: %s\n", SDL_GetError() );
    	exit(1);
    }

    info = SDL_GetVideoInfo();

    if( !info ) {
	printf("SDL ERROR Video query failed: %s\n", SDL_GetError() );
        exit(1);
    }

    bpp = info->vfmt->BitsPerPixel;
    switch (bpp) {
	case 8:
	    rgb_size[0]=2;rgb_size[1]=3;rgb_size[2]=3;break;
	case 15:
	case 16:
	    rgb_size[0]=5;rgb_size[1]=5;rgb_size[2]=5;break;
	default:
	    rgb_size[0]=8;rgb_size[1]=8;rgb_size[2]=8;break;
    }

    SDL_GL_SetAttribute( SDL_GL_RED_SIZE,  rgb_size[0] );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE,rgb_size[1] );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, rgb_size[2] );
    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,1 );

    if( (window=SDL_SetVideoMode( SCREENW, SCREENH, bpp, SCREENF)) == 0 ) {
	printf("SDL ERROR Video mode set failed: %s\n", SDL_GetError() );
        exit(1);
    }

    glClearColor(0.1,0.1,0.15,0.0);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    displaymode=MD2D_FACENORMALS; animation=MD2A_STAND; rotate=0; sf=0; ef=1;
    show_bb=0; show_texture=1; scale=0; gamma=1.8; quit=0; looping=0;

    mymodel=MD2_loadmodel(argv[1]);
    mytex=MD2_loadtexture(argv[2]);

    glViewport(0,0,SCREENW,SCREENH);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45,1.333,1.0,-100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(8.5,0,0, 0,0,0, 0,0,1);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  f100);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  f100);
    glLightfv(GL_LIGHT0, GL_SPECULAR, f100);

    glLightModelfv(GL_LIGHT_MODEL_AMBIENT,f100);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,GL_TRUE);

    printinfo();

    time_next=SDL_GetTicks()+40;
    while(!quit) {
	SDL_SetGamma(gamma,gamma,gamma);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	light0_pos[0]= 60*sin(2*PI*spin/360.0);
	light0_pos[2]= 60*cos(2*PI*spin/360.0);

	glLightfv(GL_LIGHT0, GL_POSITION, light0_pos);

        glPushMatrix();
	if(rotate) glRotatef(spin,0,0,1);
	glScalef(.075,.075,.075);
	if(displaymode==MD2D_WIREFRAME) {
	    glDisable(GL_TEXTURE_RECTANGLE_NV);
	    glDisable(GL_LINE_STIPPLE);
	    glLineWidth(2.0);
	    glColor3f(1,1,1);
	} else if(displaymode==MD2D_POINTS) {
	    glDisable(GL_TEXTURE_RECTANGLE_NV);
	    glPointSize(2.0);
	    glColor3f(1,1,1);
	} else {
	    if(show_texture) {
		glEnable(GL_TEXTURE_RECTANGLE_NV);
		glBindTexture(GL_TEXTURE_RECTANGLE_NV,mytex->name);
	    } else {
		glDisable(GL_TEXTURE_RECTANGLE_NV);
	    }
	}
	if(looping) {
	    MD2_display(mymodel,mytex,sf,ef,scale,displaymode,&bb);
	    scale+=.3;
	    if(scale>1.0) { scale-=1.0; sf++; ef=sf+1; sf%=mymodel->nFrames; ef%=mymodel->nFrames; }
	} else {
      glScalef(.7,.7,.7);
      glTranslatef(0,-60,0);
      MD2_anim_display(mymodel,mytex,animation,scale/MD2A_TIME[animation],displaymode,&bb);
      glDisable(GL_TEXTURE_RECTANGLE_NV);
      glTranslatef(0,40,0);
      MD2_anim_display(mymodel,mytex,animation,scale/MD2A_TIME[animation],displaymode,&bb);
      glTranslatef(0,40,0);
      glDisable(GL_LINE_STIPPLE);
      glLineWidth(2.0);
      glColor3f(1,1,1);
      glDisable(GL_LIGHTING);
      MD2_anim_display(mymodel,mytex,animation,scale/MD2A_TIME[animation],MD2D_POINTS,&bb);
      glEnable(GL_LIGHTING);
      glTranslatef(0,40,0);
      glDisable(GL_TEXTURE_RECTANGLE_NV);
	    glPointSize(2.0);
	    glColor3f(1,1,1);
      MD2_anim_display(mymodel,mytex,animation,scale/MD2A_TIME[animation],MD2D_WIREFRAME,&bb);
	    scale+=.1;
	    if(scale>MD2A_TIME[animation]) scale-=MD2A_TIME[animation];
      printf("%02d\n",(int)(10*scale));
      Screenshot((int)(10*scale));
	}
	if(show_bb) {
	    glDisable(GL_TEXTURE_RECTANGLE_NV);
	    glEnable(GL_LINE_STIPPLE);
	    glLineStipple(1,0xC3C3);
	    draw_bbox(&bb);
	}
	glPopMatrix();

	// spin+=1; spin=fmod(spin,360.0);
  spin=270;

	SDL_GL_SwapBuffers();
  while( SDL_PollEvent( &event ) )
        {   switch( event.type )
            {       case SDL_QUIT:
		    quit=1;
                        break;
		case SDL_KEYDOWN: {
		    switch(event.key.keysym.sym) {
			case SDLK_b:
			    show_bb=1-show_bb;
			    break;
			case SDLK_r:
			    rotate=1-rotate;
			    break;
			case SDLK_t:
			    show_texture=1-show_texture;
			    break;
			case SDLK_DOWN:
			    if(gamma>0) gamma-=.1;
			    break;
			case SDLK_UP:
			    gamma+=.1;
			    break;
			case SDLK_l:
			    looping=1-looping;
			    scale=0; sf=0; ef=1;
			    printf("%sloop\n",looping?"":"no ");
			    break;
			case SDLK_RETURN:
			    animation++; animation%=MD2A_MAXANIMATIONS-1;
			    break;
			case SDLK_SPACE:
			    if(displaymode==MD2D_VERTEXNORMALS) {
				printf("switch to wireframe\n");
				displaymode=MD2D_WIREFRAME;
			    }
			    else if(displaymode==MD2D_FACENORMALS) {
				printf("switch to averagenormals\n");
				displaymode=MD2D_AVERAGENORMALS;
			    }
			    else if(displaymode==MD2D_AVERAGENORMALS) {
				printf("switch to vertexnormals\n");
				displaymode=MD2D_VERTEXNORMALS;
			    }
			    else if(displaymode==MD2D_WIREFRAME) {
				printf("switch to points\n");
				displaymode=MD2D_POINTS;
			    }
			    else if(displaymode==MD2D_POINTS) {
				printf("switch to facenormals\n");
				displaymode=MD2D_FACENORMALS;
			    }
			    break;
			case SDLK_ESCAPE:
			case SDLK_q:
			    quit=1;
			    break;
			default:
			    break;
		    }
		}
                    default:
                        break;
            }
        }
        time_now=SDL_GetTicks(); if (time_now<time_next) SDL_Delay(time_next-time_now);
        time_next+=40;
    }
    SDL_Quit();
    return (0);
}
