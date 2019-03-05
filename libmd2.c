/********************************************************************************
    libmd2.c - a set of functions and structures to load and display MD2 models
    
    Version 1.0
    
    (c) 2005 Leander Seige, www.determinate.net/webdata/seg/snippets.html
    contact: snippets@determinate.net
    
    RELEASED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE (GPL) V3
    see www.determinate.net/webdata/seg/COPYING for more
    
    Read the included file README for more.
 ********************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <SDL/SDL_image.h>



/* the usual animation sequences of the keyframes */

#define MD2A_STAND		0
#define MD2A_RUN		1
#define MD2A_ATTACK		2
#define MD2A_PAIN_A		3
#define MD2A_PAIN_B		4
#define MD2A_PAIN_C		5
#define MD2A_JUMP		6
#define MD2A_FLIP		7
#define MD2A_SALUTE		8
#define MD2A_FALLBACK		9
#define MD2A_WAVE		10
#define MD2A_POINT		11
#define MD2A_CROUCH_STAND	12
#define MD2A_CROUCH_WALK	13
#define MD2A_CROUCH_ATTACK	14
#define MD2A_CROUCH_PAIN	15
#define MD2A_CROUCH_DEATH	16
#define MD2A_DEATH_FALLBACK	17
#define MD2A_DEATH_FALLFORWARD	18
#define MD2A_DEATH_FALLBACKSLOW	19
#define MD2A_BOOM		20
#define MD2A_MAXANIMATIONS	21



/* different rendermodes, designed to be flags but not used for now */

#define MD2D_FACENORMALS	1
#define MD2D_VERTEXNORMALS	2
#define MD2D_AVERAGENORMALS	4
#define MD2D_WIREFRAME		8
#define MD2D_POINTS		16



/* animation sequence specifications: startkeyframe, endkeyframe, fps, length in keyframes, length in seconds, name */

const GLint MD2A_START   [] ={       0,      40,      46,     54,     58,     62,     66,      72,      84,       95,     112,     123,      135,    154,      160,    169,    173,    178,    184,    190,    198};
const GLint MD2A_END     [] ={      39,      45,      53,     57,     61,     65,     71,      83,      94,      111,     122,     134,      153,    159,      168,    172,    177,    183,    189,    197,    198};
const GLint MD2A_FPS     [] ={       9,      10,      10,      7,      7,      7,      7,       7,       7,       10,       7,       6,       10,      7,       10,      7,      5,      7,      7,      7,      5};
const GLint MD2A_LEN     [] ={      40,       6,       8,      4,      4,      4,      6,      12,      11,       17,      11,      12,       19,      6,        9,      4,      5,      6,      6,      8,      1};
const GLdouble MD2A_TIME [] ={40.0/9.0,6.0/10.0,8.0/10.0,4.0/7.0,4.0/7.0,4.0/7.0,6.0/7.0,12.0/7.0,11.0/7.0,17.0/10.0,11.0/7.0,12.0/6.0,19.0/10.0,6.0/7.0, 9.0/10.0,4.0/7.0,5.0/5.0,6.0/7.0,6.0/7.0,8.0/7.0,1.0/5.0};
GLubyte *MD2A_NAME[]={ "STAND", "RUN", "ATTACK", "PAIN_A", "PAIN_B", "PAIN_C", "JUMP", "FLIP", "SALUTE", "FALLBACK", "WAVE", "POINT", "CROUCH_STAND", "CROUCH_WALK", "CROUCH_ATTACK", "CROUCH_PAIN", "CROUCH_DEATH", "DEATH_FALLBACK", "DEATH_FALLFORWARD", "DEATH_FALLBACKSLOW", "BOOM" };



/* some structures :) */

struct md2_uv { GLushort u,v; } ;

struct md2_face { GLushort point[3],uv[3]; } ;

struct md2_vertex { GLubyte v[3],normalidx; } ;

struct md2_vertexd { GLdouble v[3]; };

struct md2_boundingbox { GLdouble x1,x2,y1,y2,z1,z2; };

struct md2_frameheader { 
    GLfloat 		scale[3];
    GLfloat 		translate[3];
    GLubyte		name[16];
    struct md2_vertex	vertex[1];
};

struct md2_model {
    GLuint	ID;
    GLuint	Version;
    GLuint	TexWidth;
    GLuint	TexHeight;
    GLuint	FrameSize;
    GLuint	nTextures;
    GLuint	nVertices;
    GLuint	nTexCoords;
    GLuint	nFaces;
    GLuint	nGLCommands;
    GLuint	nFrames;
    GLuint	TexOffset;
    GLuint	UVOffset;
    GLuint	FaceOffset;
    GLuint	FrameOffset;
    GLuint	GLCmdOffset;
    GLuint	EOFOffset;
    GLubyte *		TexNames;
    GLubyte *		FrameNames;
    struct md2_uv *		UV;
    struct md2_face *		Faces;
    struct md2_vertexd *	Vertex;
    GLuint *			GLCmds;
    struct md2_vertexd * 	VNormal;
    struct md2_vertexd * 	FNormal;
};

struct md2_texture {
    GLint w,h;
    GLuint name;
};



/* two small calculation helper functions */

void MD2_normalize(struct md2_vertexd *vf) {
    GLdouble len;

    len=sqrt((vf->v[0]*vf->v[0])+(vf->v[1]*vf->v[1])+(vf->v[2]*vf->v[2]));
    if(len) {
	vf->v[0]=(GLdouble)(vf->v[0]/len);
	vf->v[1]=(GLdouble)(vf->v[1]/len);
	vf->v[2]=(GLdouble)(vf->v[2]/len);    
    } else {
	vf->v[0]=0;
	vf->v[1]=0;
	vf->v[2]=0;
    }
}

void MD2_calc_normal(struct md2_vertexd *avf, struct md2_vertexd *bvf, struct md2_vertexd *cvf, struct md2_vertexd *rvf) {
    GLdouble x1,y1,z1,x2,y2,z2;

    x1=avf->v[0]-bvf->v[0];
    y1=avf->v[1]-bvf->v[1];
    z1=avf->v[2]-bvf->v[2];
    x2=avf->v[0]-cvf->v[0];
    y2=avf->v[1]-cvf->v[1];
    z2=avf->v[2]-cvf->v[2];
    rvf->v[0]=(y1*z2)-(z1*y2);
    rvf->v[1]=(z1*x2)-(x1*z2);
    rvf->v[2]=(x1*y2)-(y1*x2);
    MD2_normalize(rvf);
}



/* texture loading, independent from model loading, so you can have multiple textures for one model or use whatever as texture */

struct md2_texture * MD2_loadtexture (GLubyte * fn) {
    SDL_Surface *surf1,*surf2;
    struct md2_texture *tex;
    SDL_PixelFormat cform;
    
    cform.palette=NULL;
    cform.BitsPerPixel=24;
    cform.BytesPerPixel=3;
    cform.Rmask=0xFF0000;
    cform.Gmask=0x00FF00;
    cform.Bmask=0x0000FF;
    cform.Amask=0;
    cform.Rshift=cform.Gshift=cform.Bshift=0;
    cform.Rloss =cform.Gloss =cform.Bloss =0;
    cform.colorkey=0; cform.alpha=0;
    
    tex=malloc(sizeof(struct md2_texture));
    if(!tex) {
	fprintf(stderr,"Out of memory, texture\n");
	return(NULL);
    }
    surf1=IMG_Load(fn);
    if(surf1==NULL) {
	fprintf(stderr,"Cannot load %s\n",fn);
	free(tex); return(NULL);
    }
    glGenTextures(1,&(tex->name));
    surf2=SDL_ConvertSurface(surf1,&cform,0);
    if(surf2==NULL) {
	fprintf(stderr,"Cannot convert %s\n",fn);
	SDL_FreeSurface(surf1); free(tex); return(NULL);
    }
    glBindTexture(GL_TEXTURE_RECTANGLE_NV,tex->name);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    tex->w=surf2->w;
    tex->h=surf2->h;
    glTexImage2D(GL_TEXTURE_RECTANGLE_NV,0,GL_RGB,surf2->w,surf2->h,0,GL_BGR,GL_UNSIGNED_BYTE,surf2->pixels);
    
    SDL_FreeSurface(surf1);
    SDL_FreeSurface(surf2);
    return(tex);
}



/* loading the model itself */

struct md2_model * MD2_loadmodel (GLubyte * fn) {
    FILE *file;
    struct md2_model *md2;
    GLuint n;
    GLubyte *frames;
    GLint c,i,s,d;    
    struct md2_frameheader *fh;
    struct md2_vertex *vb;
    struct md2_vertexd *vf,*avf,*bvf,*cvf,svf,rvf;
    
    file=fopen(fn,"rb");
    if(!file) {
	fprintf(stderr,"Cannot load %s\n",fn);
	return(NULL);
    }
    
    /* loading header */
    n=sizeof(struct md2_model); md2=malloc(n);
    if(!md2) {
	fprintf(stderr,"Out of memory, header\n");
	fclose(file); return(NULL);
    }
    if(n!=fread((void *)md2,1,n,file)) {
	fprintf(stderr,"Read error, header\n");
	fclose(file); return(NULL);
    }

    /* loading texture names */
    n=64*(md2->nTextures); md2->TexNames=malloc(n);
    if(md2->TexNames==NULL) {
	fprintf(stderr,"Out of memory, tex names\n"); fclose(file);
	free(md2); return(NULL);
    }
    fseek(file,md2->TexOffset,SEEK_SET);
    if(n!=fread(md2->TexNames,1,n,file)) {
	fprintf(stderr,"Read error, tex names\n"); fclose(file);
	free(md2->TexNames); free(md2); return(NULL);
    }
    
    /* loading texture coordinates */
    n=2*2*(md2->nTexCoords); md2->UV=malloc(n);
    if(md2->UV==NULL) {
	fprintf(stderr,"Out of memory, tex coo\n"); fclose(file);
	free(md2->TexNames); free(md2); return(NULL);
    }
    fseek(file,md2->UVOffset,SEEK_SET);
    if(n!=fread(md2->UV,1,n,file)) {
	fprintf(stderr,"Read error, tex coo\n"); fclose(file);
	free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }

    /* loading glcommands */
    n=4*(md2->nGLCommands); md2->GLCmds=malloc(n);
    if(md2->GLCmds==NULL) {
	fprintf(stderr,"Out of memory, tex coo\n"); fclose(file);
	free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    fseek(file,md2->GLCmdOffset,SEEK_SET);
    if(n!=fread(md2->GLCmds,1,n,file)) {
	fprintf(stderr,"Read error, tex coo\n"); fclose(file);
	free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }

    /* loading faces */
    n=2*6*(md2->nFaces); md2->Faces=malloc(n);
    if(md2->Faces==NULL) {
	fprintf(stderr,"Out of memory, faces\n"); fclose(file);
	free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    fseek(file,md2->FaceOffset,SEEK_SET);
    if(n!=fread(md2->Faces,1,n,file)) {
	fprintf(stderr,"Read error, faces\n"); fclose(file);
	free(md2->Faces); free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    
    /* loading frames */
    n=md2->FrameSize*md2->nFrames; frames=malloc(n);
    if(!frames) {
	fprintf(stderr,"Out of memory, frames (1)\n"); fclose(file);
	free(md2->Faces); free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    fseek(file,md2->FrameOffset,SEEK_SET);
    if(n!=fread(frames,1,n,file)) {
	fprintf(stderr,"Read error, faces\n"); fclose(file);
	free(frames); free(md2->Faces); free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    n=md2->nFrames*md2->nVertices*sizeof(struct md2_vertexd); md2->Vertex=malloc(n);
    if(md2->Vertex==NULL) {
	fprintf(stderr,"Out of memory, frames (2)\n"); fclose(file);
	free(frames); free(md2->Faces); free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    for(n=0;n<(md2->nFrames);n++) {
	for(c=0;c<(md2->nVertices);c++) {
	    fh=(struct md2_frameheader *)(frames+(md2->FrameSize*n));
	    vb=&(fh->vertex[c]);
	    vf=&(md2->Vertex[md2->nVertices*n+c]);
	    vf->v[0]=(((GLdouble)vb->v[0])*fh->scale[0])+fh->translate[0];
	    vf->v[1]=(((GLdouble)vb->v[1])*fh->scale[1])+fh->translate[1];
	    vf->v[2]=(((GLdouble)vb->v[2])*fh->scale[2])+fh->translate[2];
	}
    }

    free(frames);
    fclose(file);

    /* building vertex normals */
    n=md2->nFrames*md2->nVertices*sizeof(struct md2_vertexd); md2->VNormal=malloc(n);
    if(md2->VNormal==NULL) {
	fprintf(stderr,"Out of memory, frames (2)\n");
	free(md2->Vertex); free(md2->Faces); free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    for(n=0;n<(md2->nFrames);n++) {
	for(c=0;c<(md2->nVertices);c++) {
	    svf.v[0]=svf.v[1]=svf.v[2]=0; s=0;
	    for(i=0;i<(md2->nFaces);i++) {
		for(d=0;d<3;d++) {
		    if(((&(md2->Faces[i]))->point[d])==c) {
		    	avf=&(md2->Vertex[(&(md2->Faces[i]))->point[0]+(n*(md2->nVertices))]);
		    	bvf=&(md2->Vertex[(&(md2->Faces[i]))->point[1]+(n*(md2->nVertices))]);
		    	cvf=&(md2->Vertex[(&(md2->Faces[i]))->point[2]+(n*(md2->nVertices))]);
			MD2_calc_normal(avf,bvf,cvf,&rvf);
			svf.v[0]+=rvf.v[0];
			svf.v[1]+=rvf.v[1];
			svf.v[2]+=rvf.v[2];
			s++;
		    }
		}
	    }
	    svf.v[0]/=((GLdouble)s);
	    svf.v[1]/=((GLdouble)s);
	    svf.v[2]/=((GLdouble)s);
	    MD2_normalize(&svf);
	    (&(md2->VNormal[c+(n*(md2->nVertices))]))->v[0]=svf.v[0];
	    (&(md2->VNormal[c+(n*(md2->nVertices))]))->v[1]=svf.v[1];
	    (&(md2->VNormal[c+(n*(md2->nVertices))]))->v[2]=svf.v[2];
	}
    }

    /* building face normals */
    n=md2->nFrames*md2->nFaces*sizeof(struct md2_vertexd); md2->FNormal=malloc(n);
    if(md2->FNormal==NULL) {
        fprintf(stderr,"Out of memory, frames (2)\n");
        free(md2->VNormal); free(md2->Vertex); free(md2->Faces); free(md2->GLCmds); free(md2->UV); free(md2->TexNames); free(md2); return(NULL);
    }
    for(n=0;n<(md2->nFrames);n++) {
        for(i=0;i<(md2->nFaces);i++) {
            avf=&(md2->Vertex[(&(md2->Faces[i]))->point[0]+(n*(md2->nVertices))]);
            bvf=&(md2->Vertex[(&(md2->Faces[i]))->point[1]+(n*(md2->nVertices))]);
            cvf=&(md2->Vertex[(&(md2->Faces[i]))->point[2]+(n*(md2->nVertices))]);
            MD2_calc_normal(avf,bvf,cvf,&rvf);
            (&(md2->FNormal[i+(n*(md2->nFaces))]))->v[0]=rvf.v[0];
            (&(md2->FNormal[i+(n*(md2->nFaces))]))->v[1]=rvf.v[1];
            (&(md2->FNormal[i+(n*(md2->nFaces))]))->v[2]=rvf.v[2];
        }
    }

    return(md2);
}



/* different render functions, average normals means the average of per vertex and per face normals */

int MD2_display_average_normals (struct md2_model * md2, struct md2_texture * tex, GLint sf, GLint ef, GLdouble s, struct md2_boundingbox * bb) {
    GLint n,c;
    struct md2_vertexd *svf,*evf,*snf,*enf,*saf,*eaf,mv,nv,av;
    struct md2_uv *uv;

    if(bb) bb->x1=bb->x2=bb->y1=bb->y2=bb->z1=bb->z2=0;
    glBegin(GL_TRIANGLES);
    for(n=0;n<(md2->nFaces);n++) {
        for(c=0;c<3;c++) {
            svf=&(md2->Vertex[(&(md2->Faces[n]))->point[c]+(sf*(md2->nVertices))]);
            evf=&(md2->Vertex[(&(md2->Faces[n]))->point[c]+(ef*(md2->nVertices))]);
            snf=&(md2->FNormal[n+(sf*(md2->nFaces))]);
            enf=&(md2->FNormal[n+(ef*(md2->nFaces))]);
            saf=&(md2->VNormal[(&(md2->Faces[n]))->point[c]+(sf*(md2->nVertices))]);
            eaf=&(md2->VNormal[(&(md2->Faces[n]))->point[c]+(ef*(md2->nVertices))]);
            mv.v[0]=svf->v[0]+s*(evf->v[0]-svf->v[0]);
            mv.v[1]=svf->v[1]+s*(evf->v[1]-svf->v[1]);
            mv.v[2]=svf->v[2]+s*(evf->v[2]-svf->v[2]);
	    if(bb) {
		if		(mv.v[0]>bb->x1) bb->x1=mv.v[0];
		else if 	(mv.v[0]<bb->x2) bb->x2=mv.v[0];
		if		(mv.v[1]>bb->y1) bb->y1=mv.v[1];
		else if 	(mv.v[1]<bb->y2) bb->y2=mv.v[1];
		if		(mv.v[2]>bb->z1) bb->z1=mv.v[2];
		else if 	(mv.v[2]<bb->z2) bb->z2=mv.v[2];
	    }
            nv.v[0]=snf->v[0]+s*(enf->v[0]-snf->v[0]);
            nv.v[1]=snf->v[1]+s*(enf->v[1]-snf->v[1]);
            nv.v[2]=snf->v[2]+s*(enf->v[2]-snf->v[2]);
            av.v[0]=saf->v[0]+s*(eaf->v[0]-saf->v[0]);
            av.v[1]=saf->v[1]+s*(eaf->v[1]-saf->v[1]);
            av.v[2]=saf->v[2]+s*(eaf->v[2]-saf->v[2]);
	    nv.v[0]=(nv.v[0]+av.v[0])/2;
	    nv.v[1]=(nv.v[1]+av.v[1])/2;
	    nv.v[2]=(nv.v[2]+av.v[2])/2;
            if(tex) {
                uv=&(md2->UV[(&(md2->Faces[n]))->uv[c]]);
                glTexCoord2s(uv->u,uv->v);
            }
            glNormal3f(nv.v[0],nv.v[1],nv.v[2]);
            glVertex3f(mv.v[0],mv.v[1],mv.v[2]);
        }
    }
    glEnd();
    return(1);
}



/* render function, per face normals only */

int MD2_display_per_face_normals (struct md2_model * md2, struct md2_texture * tex, GLint sf, GLint ef, GLdouble s, struct md2_boundingbox * bb) {
    GLint n,c;
    struct md2_vertexd *svf,*evf,*snf,*enf,mv,nv;
    struct md2_uv *uv;

    if(bb) bb->x1=bb->x2=bb->y1=bb->y2=bb->z1=bb->z2=0;
    glBegin(GL_TRIANGLES);
    for(n=0;n<(md2->nFaces);n++) {
        for(c=0;c<3;c++) {
            svf=&(md2->Vertex[(&(md2->Faces[n]))->point[c]+(sf*(md2->nVertices))]);
            evf=&(md2->Vertex[(&(md2->Faces[n]))->point[c]+(ef*(md2->nVertices))]);
            snf=&(md2->FNormal[n+(sf*(md2->nFaces))]);
            enf=&(md2->FNormal[n+(ef*(md2->nFaces))]);
    	    mv.v[0]=svf->v[0]+s*(evf->v[0]-svf->v[0]);
            mv.v[1]=svf->v[1]+s*(evf->v[1]-svf->v[1]);
            mv.v[2]=svf->v[2]+s*(evf->v[2]-svf->v[2]);
	    if(bb) {
		if		(mv.v[0]>bb->x1) bb->x1=mv.v[0];
		else if 	(mv.v[0]<bb->x2) bb->x2=mv.v[0];
		if		(mv.v[1]>bb->y1) bb->y1=mv.v[1];
		else if 	(mv.v[1]<bb->y2) bb->y2=mv.v[1];
		if		(mv.v[2]>bb->z1) bb->z1=mv.v[2];
		else if 	(mv.v[2]<bb->z2) bb->z2=mv.v[2];
	    }
            nv.v[0]=snf->v[0]+s*(enf->v[0]-snf->v[0]);
            nv.v[1]=snf->v[1]+s*(enf->v[1]-snf->v[1]);
            nv.v[2]=snf->v[2]+s*(enf->v[2]-snf->v[2]);
            if(tex) {
                uv=&(md2->UV[(&(md2->Faces[n]))->uv[c]]);
                glTexCoord2s(uv->u,uv->v);
            }
            glNormal3f(nv.v[0],nv.v[1],nv.v[2]);
            glVertex3f(mv.v[0],mv.v[1],mv.v[2]);
        }
    }
    glEnd();
    return(1);
}



/* render function, per vertex normals only */

int MD2_display_per_vertex_normals (struct md2_model * md2, struct md2_texture * tex, GLint sf, GLint ef, GLdouble s, struct md2_boundingbox * bb) {
    GLint c,i,w;
    struct md2_vertexd *svf,*evf,*snf,*enf,mv,nv;

    if(bb) bb->x1=bb->x2=bb->y1=bb->y2=bb->z1=bb->z2=0;
    i=0; while((w=(md2->GLCmds[i++]))) {
	if(w>0) {
	    glBegin(GL_TRIANGLE_STRIP);
	} else {
	    glBegin(GL_TRIANGLE_FAN); w=abs(w);
	}
	for(c=0;c<w;c++) {
	    if(tex) 
		glTexCoord2f(((GLfloat *)md2->GLCmds)[i+0]*tex->w,((GLfloat *)md2->GLCmds)[i+1]*tex->h); i+=2;
	    svf=&(md2->Vertex[md2->GLCmds[i]+(sf*(md2->nVertices))]);
	    evf=&(md2->Vertex[md2->GLCmds[i]+(ef*(md2->nVertices))]);
	    snf=&(md2->VNormal[md2->GLCmds[i]+(sf*(md2->nVertices))]);
	    enf=&(md2->VNormal[md2->GLCmds[i]+(ef*(md2->nVertices))]);
	    mv.v[0]=svf->v[0]+s*(evf->v[0]-svf->v[0]);
	    mv.v[1]=svf->v[1]+s*(evf->v[1]-svf->v[1]);
	    mv.v[2]=svf->v[2]+s*(evf->v[2]-svf->v[2]);
	    if(bb) {
		if		(mv.v[0]>bb->x1) bb->x1=mv.v[0];
		else if 	(mv.v[0]<bb->x2) bb->x2=mv.v[0];
		if		(mv.v[1]>bb->y1) bb->y1=mv.v[1];
		else if 	(mv.v[1]<bb->y2) bb->y2=mv.v[1];
		if		(mv.v[2]>bb->z1) bb->z1=mv.v[2];
		else if 	(mv.v[2]<bb->z2) bb->z2=mv.v[2];
	    }
	    nv.v[0]=snf->v[0]+s*(enf->v[0]-snf->v[0]);
	    nv.v[1]=snf->v[1]+s*(enf->v[1]-snf->v[1]);
	    nv.v[2]=snf->v[2]+s*(enf->v[2]-snf->v[2]);
	    glNormal3f(nv.v[0],nv.v[1],nv.v[2]);
	    glVertex3f(mv.v[0],mv.v[1],mv.v[2]);
	    i++;
	}	
	glEnd();
    }
    return(1);
}



/* render function, suitable for rendering as wireframe */

int MD2_wire_display (struct md2_model * md2, GLint sf, GLint ef, GLfloat s, struct md2_boundingbox * bb) {
    GLint n,c;
    struct md2_vertexd *svf,*evf,*snf,*enf,mv,nv;

    if(bb) bb->x1=bb->x2=bb->y1=bb->y2=bb->z1=bb->z2=0;
    for(n=0;n<(md2->nFaces);n++) {
	glBegin(GL_LINE_STRIP);
	for(c=0;c<3;c++) {
	    svf=&(md2->Vertex[(&(md2->Faces[n]))->point[c]+(sf*(md2->nVertices))]);
	    evf=&(md2->Vertex[(&(md2->Faces[n]))->point[c]+(ef*(md2->nVertices))]);
	    snf=&(md2->VNormal[(&(md2->Faces[n]))->point[c]+(sf*(md2->nVertices))]);
	    enf=&(md2->VNormal[(&(md2->Faces[n]))->point[c]+(ef*(md2->nVertices))]);
	    mv.v[0]=svf->v[0]+s*(evf->v[0]-svf->v[0]);
	    mv.v[1]=svf->v[1]+s*(evf->v[1]-svf->v[1]);
	    mv.v[2]=svf->v[2]+s*(evf->v[2]-svf->v[2]);
	    if(bb) {
		if		(mv.v[0]>bb->x1) bb->x1=mv.v[0];
		else if 	(mv.v[0]<bb->x2) bb->x2=mv.v[0];
		if		(mv.v[1]>bb->y1) bb->y1=mv.v[1];
		else if 	(mv.v[1]<bb->y2) bb->y2=mv.v[1];
		if		(mv.v[2]>bb->z1) bb->z1=mv.v[2];
		else if 	(mv.v[2]<bb->z2) bb->z2=mv.v[2];
	    }
	    nv.v[0]=snf->v[0]+s*(enf->v[0]-snf->v[0]);
	    nv.v[1]=snf->v[1]+s*(enf->v[1]-snf->v[1]);
	    nv.v[2]=snf->v[2]+s*(enf->v[2]-snf->v[2]);
	    glNormal3f(nv.v[0],nv.v[1],nv.v[2]);
	    glVertex3f(mv.v[0],mv.v[1],mv.v[2]);
	}
	glEnd();
    }
    return(1);
}



/* render function, suitable to render the points only */

int MD2_point_display (struct md2_model * md2, GLint sf, GLint ef, GLfloat s, struct md2_boundingbox * bb) {
    GLint n;
    struct md2_vertexd *svf,*evf,mv;
    
    if(bb) bb->x1=bb->x2=bb->y1=bb->y2=bb->z1=bb->z2=0;
    glBegin(GL_POINTS);
    for(n=0;n<(md2->nVertices);n++) {
	    svf=&(md2->Vertex[n+(sf*(md2->nVertices))]);
	    evf=&(md2->Vertex[n+(ef*(md2->nVertices))]);
	    mv.v[0]=svf->v[0]+s*(evf->v[0]-svf->v[0]);
	    mv.v[1]=svf->v[1]+s*(evf->v[1]-svf->v[1]);
	    mv.v[2]=svf->v[2]+s*(evf->v[2]-svf->v[2]);
	    if(bb) {
		if		(mv.v[0]>bb->x1) bb->x1=mv.v[0];
		else if 	(mv.v[0]<bb->x2) bb->x2=mv.v[0];
		if		(mv.v[1]>bb->y1) bb->y1=mv.v[1];
		else if 	(mv.v[1]<bb->y2) bb->y2=mv.v[1];
		if		(mv.v[2]>bb->z1) bb->z1=mv.v[2];
		else if 	(mv.v[2]<bb->z2) bb->z2=mv.v[2];
	    }
	    glNormal3f(mv.v[0],mv.v[1],mv.v[2]);
	    glVertex3f(mv.v[0],mv.v[1],mv.v[2]);
    }
    glEnd();
    return(1);
}



/* super render function, arbitrary start and end frames */

int MD2_display (struct md2_model * md2, struct md2_texture * tex, GLint sf, GLint ef, GLdouble s, GLint mode, struct md2_boundingbox * bb) {

    if(	sf>=(md2->nFrames)
    ||	ef>=(md2->nFrames)
    ||  s>1.0
    ||  s<0.0
    ||	ef<0
    ||	sf<0 ) return(0);

    switch (mode) {
	case MD2D_WIREFRAME:
	    MD2_wire_display (md2, sf, ef, s, bb);
	    break;
	case MD2D_POINTS:
	    MD2_point_display (md2, sf, ef, s, bb);
	    break;
	case MD2D_VERTEXNORMALS:
	    MD2_display_per_vertex_normals (md2, tex, sf, ef, s, bb); 
	    break;
	case MD2D_AVERAGENORMALS:
	    MD2_display_average_normals (md2, tex, sf, ef, s, bb); 
	    break;
	case MD2D_FACENORMALS:
	default:
	    MD2_display_per_face_normals(md2, tex, sf, ef, s, bb); 
	    break;
    }
    return(1);
}



/* super render function, easier access to usual md2 animation sequences */

int MD2_anim_display (struct md2_model * md2, struct md2_texture * tex, GLint anim, GLdouble s, GLint mode, struct md2_boundingbox * bb) {
    GLint sf,ef;
    GLdouble t;

    if(	anim<0
    ||	anim>MD2A_MAXANIMATIONS
    ||	md2->nFrames<198
    ||	s<0 ) return(0);

    sf=floor(((GLdouble)MD2A_LEN[anim])*s)+MD2A_START[anim];
    if(sf==MD2A_END[anim]) ef=MD2A_START[anim];
    else ef=sf+1;
    
    t=1/((GLdouble)MD2A_LEN[anim]);
    s=(s-(floor(s/t)*t))/t;
    
    MD2_display(md2, tex, sf, ef, s, mode, bb);
    
    return(1);
}



/* dump some informations about the model */

int MD2_modelinfo (struct md2_model * md2, GLint level) {
    GLint c;
    struct md2_vertexd *vf,*wf;
    
    fprintf(stderr,"ID                : 0x%x\n",md2->ID);
    fprintf(stderr,"Version           : %d\n",md2->Version);
    fprintf(stderr,"Texture Width     : %d\n",md2->TexWidth);
    fprintf(stderr,"Texture Height    : %d\n",md2->TexHeight);
    fprintf(stderr,"Frame Size        : %d\n",md2->FrameSize);
    fprintf(stderr,"Number of Textures: %d\n",md2->nTextures);
    fprintf(stderr,"Number of Vertices: %d\n",md2->nVertices);
    fprintf(stderr,"Number of TexCoo  : %d\n",md2->nTexCoords);
    fprintf(stderr,"Number of Faces   : %d\n",md2->nFaces);
    fprintf(stderr,"Number of GLCmds  : %d\n",md2->nGLCommands);
    fprintf(stderr,"Number of Frames  : %d\n",md2->nFrames);
    if(level>1) {
	for(c=0;c<(md2->nTextures);c++) 
	    fprintf(stderr,"Texture %3d       : %s\n",c,(md2->TexNames)+c);
	if(level>2) {
	    for(c=0;c<(md2->nTexCoords);c++) 
		fprintf(stderr,"Texture Coo %6d   : %d,%d\n",c,((md2->UV)+c)->u,((md2->UV)+c)->v);
	    if(level>3) {
		for(c=0;c<md2->nVertices;c++) {
		    vf=&(md2->Vertex[c]);
		    wf=&(md2->VNormal[c]);
		    fprintf(stdout,"vertex %f %f %f normal %f %f %f\n",vf->v[0],vf->v[1],vf->v[2],wf->v[0],wf->v[1],wf->v[2]);
		}
	    }
	}
    }
    return(1);
}



/* free all model memory */

int MD2_freemodel (struct md2_model * md2) {
    free(md2->FNormal);
    free(md2->VNormal);
    free(md2->Vertex);
    free(md2->Faces);
    free(md2->GLCmds);
    free(md2->UV);
    free(md2->TexNames);
    free(md2);
    return(1);
}

/* free a texture */

int MD2_freetexture (struct md2_texture * tex) {
    glDeleteTextures(1,&(tex->name));
    free(tex);
    return(1);
}
