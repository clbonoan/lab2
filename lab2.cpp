//
//modified by: christine bonoan
//date:
//
//original author: Gordon Griesel
//date:            2025
//purpose:         OpenGL sample program
//
//This program needs some refactoring.
//We will do this in class together.
//
//to do list
// 01/31/2025 figure out how to fix box color when resizing window
// after speeding up or slowing down animation
//
//
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"

//some structures

class Global {
public:
	int xres, yres;
    float w;
    float dir;
    float vdir; //vertical direction
    float pos[2];
    int bounceCount;
    float speed;
    int prevXres;
    int prevYres;
    bool resized;
    bool speedChanged;
    int boxColor[3];
	Global();
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);


int main()
{
	init_opengl();
	int done = 0;
	//main game loop
	while (!done) {
		//look for external events such as keyboard, mouse.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			x11.check_mouse(&e);
			done = x11.check_keys(&e);
		}
		physics();
		render();
		x11.swapBuffers();
		usleep(200/g.speed);    //adjusts animation speed
	}
    cleanup_fonts();
	return 0;
}

Global::Global()
{
	xres = 400;
	yres = 200;
    w = 20.0f;
    dir = 30.0f;
    //add vertical movement
    vdir = 10.0f;  
    pos[0] = 0.0f+w;
    pos[1] = g.yres/2.0f;
    bounceCount = 0;
    //animation speed
    speed = 1.0f;
    prevXres = xres;
    prevYres = yres;
    resized = false;
    speedChanged = false;
    //initialize green color box upon execution
    boxColor[0] = 0;
    boxColor[1] = 255;
    boxColor[2] = 0;
}

X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 Lab-2");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
	//Window has been resized.
	g.xres = width;
	g.yres = height;lyly
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != g.xres || xce.height != g.yres) {
		//Window size did change.
        g.resized = true;
        g.prevXres = g.xres;
		reshape_window(xce.width, xce.height);

        //change box color based on window resize
        if (xce.width > g.prevXres || xce.height > g.prevYres) {
            g.boxColor[0] = 0; 
            g.boxColor[1] = 0;
            g.boxColor[2] = 255;
        }
        else if (xce.width < g.prevXres || xce.height < g.prevYres) {
            g.boxColor[0] = 255;
            g.boxColor[1] = 0;
            g.boxColor[2] = 0;
        }
	}
}
//-----------------------------------------------------------------------------

void X11_wrapper::check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed.
			//int y = g.yres - e->xbutton.y;
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.


		}
	}
}

int X11_wrapper::check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
            case XK_a:
                //'a' key was pressed
                g.speed *= 2.0f;
                g.speedChanged = true;
                break;
            case XK_b:
                //'b' key was pressed
                g.speed /= 2.0f;
                g.speedChanged = true;
                break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0); //black
    //glClearColor(1.0, 0.1, 0.1, 0.1); //red
    //glClearColor(1.0, 1.0, 0.1, 1.0);   //yellow
    
    //do this to enable fonts
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
}

void physics()
{
	//move the box
    g.pos[0] += g.dir;
    //move box vertically
    g.pos[1] += g.vdir;

    //collision detection
    //horizontal bouncing
    if (g.pos[0] >= (g.xres-g.w)) {
        g.pos[0] = (g.xres-g.w);
        g.dir = -g.dir;
        g.bounceCount++;
    }
    
    if (g.pos[0] <= g.w) {
        g.pos[0] = g.w;
        g.dir = -g.dir;
        g.bounceCount++;
    }

    //vertical bouncing
    if (g.pos[1] >= (g.yres-g.w)) {
        g.pos[1] = (g.yres-g.w);
        g.vdir = -g.vdir;
    }
    
    if (g.pos[1] <= g.w) {
        g.pos[1] = g.w;
        g.vdir = -g.vdir;
    }

    //add to disappear box if window width is smaller than box width
    /*if (g.xres < g.w * 2) {
        g.bounceCount = -1;
    }*/

}

void render()
{
	//clear the window
	glClear(GL_COLOR_BUFFER_BIT);
    //hide box if window is too small
	/*if (g.bounceCount == -1) 
        return;*/

    //draw the box
	glPushMatrix();
    glColor3ub(g.boxColor[0], g.boxColor[1], g.boxColor[2]);  //green box at start
    //change box color if window is resized
    if (g.resized) {
        if (g.xres > g.prevXres || g.yres > g.prevYres) {
            glColor3ub(g.boxColor[0] = 0, g.boxColor[1] = 0, 
                    g.boxColor[2] = 255); //blue
        }
        else if (g.xres < g.prevXres || g.yres < g.prevYres) {
            glColor3ub(g.boxColor[0] = 255, g.boxColor[1] = 0, 
                    g.boxColor[2] = 0); //red
        }
        g.resized = false;
        g.speedChanged = false;
    }

    if (g.speed > 1.5f) {
        glColor3ub(255, 0, 0); //red when faster
    }
    else if (g.speed < 1.0f) {
        glColor3ub(0, 0, 255); //blue when slower
    }
    g.speedChanged = false;
    //glColor3ub(250, 120, 220);    //color of moving box
	glTranslatef(g.pos[0], g.pos[1], 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(-g.w, -g.w);
		glVertex2f(-g.w,  g.w);
		glVertex2f( g.w,  g.w);
		glVertex2f( g.w, -g.w);
	glEnd();
	glPopMatrix();

    Rect r;
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;
    ggprint8b(&r, 16, 0x00ff0000, "3350 lab-2");
    ggprint8b(&r, 16, 0x00ffff00, "Esc to exit");
    ggprint8b(&r, 16, 0x00ffff00, "A    speed up");
    ggprint8b(&r, 16, 0x00ffff00, "B    slow down");
}






