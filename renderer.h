#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#define ILUT_USE_OPENGL
#include "il.h"
#include "ilu.h"
#include "ilut.h"

#include <string>

using namespace std;

struct Font {
	GLuint texture;
	float spacings[256];
	float offsets[256];
};

int texture_load(string filename,bool repeat);

bool shader_supported();
void shader_use(int shader);
int shader_fragment_compile(const char* source);

GLuint vbo_create(int num_vertices,float* data);
void vbo_use_pos(GLuint id);
void vbo_use_coord(GLuint id);
void vbo_use_pos_own(float* data);
void vbo_use_coord_own(float* data);
void vbo_draw(int vertices);
void vbo_delete(GLuint vbo);

Font* font_load(char* map, char* spacings);

#endif
