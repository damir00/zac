#ifndef _GUI_H_
#define _GUI_G_

#include "SDL.h"

class gtdc_gui {
	//static SDL_Surface* window;

	static void do_key(SDL_KeyboardEvent key,bool down);
	static void do_mouse(int key,bool down);
public:
	static bool open(int width,int height,bool fullscreen);
	static void swap();
	static void check_events();

	static int width,height;

	static bool wrap_mouse;

	static bool resized;

	static bool input_quit;
	static bool input_go_forward;
	static bool input_go_back;
	static bool input_go_left;
	static bool input_go_right;
	static bool input_interact;
	static bool input_use;	//only on press
	static bool input_1;
	static bool input_2;
	static bool input_3;
	static bool input_esc;
	static int input_mouse_dx;
	static int input_mouse_dy;
	static bool input_mouse_clicked[2];
	static bool input_mouse_down[2];

	static bool input_menu_enter;
	static bool input_menu_up;
	static bool input_menu_down;
};

#endif
