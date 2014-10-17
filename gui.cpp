
#include <stdio.h>

#include "gui.h"

bool gtdc_gui::input_quit=false;
bool gtdc_gui::input_go_forward=false;
bool gtdc_gui::input_go_back=false;
bool gtdc_gui::input_go_left=false;
bool gtdc_gui::input_go_right=false;
bool gtdc_gui::input_interact=false;
bool gtdc_gui::input_use=false;
bool gtdc_gui::input_1=false;
bool gtdc_gui::input_2=false;
bool gtdc_gui::input_3=false;
bool gtdc_gui::input_esc=false;
int gtdc_gui::input_mouse_dx=0;
int gtdc_gui::input_mouse_dy=0;
int gtdc_gui::width=0;
int gtdc_gui::height=0;
bool gtdc_gui::input_mouse_clicked[2];
bool gtdc_gui::input_mouse_down[2];

bool gtdc_gui::input_menu_enter=false;
bool gtdc_gui::input_menu_up=false;
bool gtdc_gui::input_menu_down=false;

bool gtdc_gui::wrap_mouse=false;
bool gtdc_gui::resized=false;;

bool gtdc_gui::open(int _width,int _height,bool fullscreen) {

	for(int i=0;i<2;i++) {
		input_mouse_clicked[i]=false;
		input_mouse_down[i]=false;
	}

    if(SDL_Init(SDL_INIT_VIDEO)<0) {
    	printf("Can't init SDL :(\n");
    	return false;
    }

    width=_width;
    height=_height;

	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, true);

	char* title=(char*)"Zombies Are Coming";
    SDL_WM_SetCaption(title,title);

    //SDL_WM_GrabInput(SDL_GRAB_ON);

    SDL_Surface* window= SDL_SetVideoMode(width,height,32,
    		SDL_RESIZABLE | SDL_HWSURFACE | SDL_OPENGL);
    SDL_EnableUNICODE(1);

	if(!window) {
		printf("Failed to create SDL window :(\n");
		printf("%s\n",SDL_GetError());
	    return false;
	}

	int HalfScreenX=width/2;
	int HalfScreenY=height/2;
	SDL_WarpMouse(HalfScreenX,HalfScreenY);

	return true;
}
void gtdc_gui::swap() {
	SDL_GL_SwapBuffers();
}
void gtdc_gui::do_key(SDL_KeyboardEvent key,bool down) {

	switch(key.keysym.sym) {
		case SDLK_UP: case 'w':
			input_go_forward=down;
			input_menu_up=down;
			return;
		case SDLK_DOWN: case 's':
			input_go_back=down;
			input_menu_down=down;
			return;
		case SDLK_LEFT: case 'a':
			input_go_left=down;
			return;
		case SDLK_RIGHT: case 'd':
			input_go_right=down;
			return;
		case SDLK_SPACE:
			input_interact=down;
		case SDLK_RETURN:
			input_menu_enter=down;
			return;
		case 'e':
			input_use=down;
			return;
		case '1':
			input_1=down;
			return;
		case '2':
			input_2=down;
			return;
		case '3':
			input_3=down;
			return;
		case SDLK_ESCAPE:
			input_esc=down;
			return;
	}
}
void gtdc_gui::do_mouse(int key,bool down) {

	switch(key) {
	case 1: key=0; break;	//left
	case 3: key=1; break;	//right
	case 2: key=2; break;	//middle
	}

	if(key>2) return;

	input_mouse_down[key]=down;
	input_mouse_clicked[key]=down;
}

void gtdc_gui::check_events() {

	for(int i=0;i<2;i++)
		input_mouse_clicked[i]=false;

	input_use=false;
	input_esc=false;
	input_menu_up=false;
	input_menu_down=false;
	input_menu_enter=false;
	resized=false;

	SDL_Event event;
	while(SDL_PollEvent(&event)) {

		switch(event.type) {
		/*
		case SDL_MOUSEMOTION:
			msg->type=DWINDOW_MESSAGE_MOUSE_MOVE;
			msg->x=event.motion.x;
			msg->y=event.motion.y;
			break;
			*/
		case SDL_MOUSEBUTTONDOWN:
			do_mouse(event.button.button,true);
			break;
		case SDL_MOUSEBUTTONUP:
			do_mouse(event.button.button,false);
			break;
		case SDL_KEYDOWN:
			do_key(event.key,true);
			break;
		case SDL_KEYUP:
			do_key(event.key,false);
			break;

		case SDL_QUIT:
			input_quit=true;
			break;
	    case SDL_VIDEORESIZE:
	    	printf("resize to %d %d\n",event.resize.w,event.resize.h);
	    	width=event.resize.w;
	    	height=event.resize.h;
	    	resized=true;

	        SDL_SetVideoMode(width,height,32,SDL_RESIZABLE | SDL_HWSURFACE | SDL_OPENGL);

	    	break;
		default:
			break;
		}
	}

	if(wrap_mouse) {
		int MouseX,MouseY;
		int HalfScreenX=width/2;
		int HalfScreenY=height/2;
		SDL_GetMouseState(&MouseX,&MouseY);
		input_mouse_dx = MouseX - HalfScreenX;
		input_mouse_dy = MouseY - HalfScreenY;
		SDL_WarpMouse(HalfScreenX,HalfScreenY);
	}

}



