/********************************************************************************
    md2info.c - a sample application for libmd2.c

    Version 1.0

    (c) 2005 Leander Seige, www.determinate.net/webdata/seg/snippets.html
    contact: snippets@determinate.net

    RELEASED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE (GPL) V3
    see www.determinate.net/webdata/seg/COPYING for more

    Read the included file README for more.
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "libmd2.c"

int main(int argc, char **argv) {
    struct md2_model * mymodel;

    if(argc!=3) {
	printf("Usage: md2info <path/model.md2> <level>\n");
	exit(1);
    }
    mymodel=MD2_loadmodel(argv[1]);
    MD2_modelinfo(mymodel,atoi(argv[2]));
    MD2_freemodel(mymodel);
    return(0);
}
