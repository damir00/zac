#include "renderer.h"

#include <stdio.h>
#include <string.h>

#define CHAR_TYPE char

int texture_load(string filename,bool repeat) {


	ILubyte *data;
	int width,height,depth;
	ILenum type;
	unsigned char* buffer;
	//unsigned char* buffer_end;
	const CHAR_TYPE* filename_c=(CHAR_TYPE*)filename.c_str();

	ILuint image_name;

	ilGenImages(1,&image_name);
	ilBindImage(image_name);

	type=ilDetermineType(filename_c);
	if(!ilLoad(type,filename_c)) {

		ILenum il_error=ilGetError();
//		if(il_error!=IL_NO_ERROR) {
//		cout<<"il image "<<filename<<": error loading, "<<iluErrorString(il_error)<<endl;
		ilDeleteImages(1,&image_name);
		return NULL;
	}

	data=ilGetData();

	if(!data) {
//		cout<<"il image: no data\n";
		ilDeleteImages(1,&image_name);
		return NULL;
	}

	width=ilGetInteger(IL_IMAGE_WIDTH);
	height=ilGetInteger(IL_IMAGE_HEIGHT);
	depth=ilGetInteger(IL_IMAGE_BPP);

	GLuint texture;
	glGenTextures(1,&texture);

	buffer=new unsigned char[width*height*4];

//	cr_buffer=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,width,height);

//	buffer=cairo_image_surface_get_data(cr_buffer);

	ilCopyPixels( 0,0,0, width,height,1, IL_RGBA, IL_UNSIGNED_BYTE, buffer);
	ilDeleteImages(1,&image_name);

	glBindTexture(GL_TEXTURE_2D,texture);

	unsigned char* buffer2=new unsigned char[width*height*4];

	for(int i=0;i<height;i++) {
		memcpy(buffer2+i*width*4,buffer+(height-i-1)*width*4,width*4);
	}

	glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, width, height,0,
			GL_RGBA, GL_UNSIGNED_BYTE, buffer2);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	if(!repeat) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	}

	delete[] buffer;
	delete[] buffer2;

//	cout<<"image loaded\n";

	return texture;

	/*
	int texture_id=ilutGLLoadImage((char*)filename.c_str());
	glBindTexture(GL_TEXTURE_2D,texture_id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	if(!repeat) {
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
	}

	printf("texture %s id %d\n",filename.c_str(),texture_id);
	return texture_id;
	*/
}

bool shader_supported() {
	return true;
}
void shader_use(int program) {
	glUseProgramObjectARB(program);
}

int shader_fragment_compile(const char* source) {

	int p=glCreateProgramObjectARB();
	int fs=glCreateShaderObjectARB(GL_FRAGMENT_SHADER);
	int vs=0;

	int i;
	glShaderSource(fs,1,(const char**)&source,NULL);
	glCompileShader(fs);

	GLint compile_status;
	glGetObjectParameterivARB(fs,GL_OBJECT_COMPILE_STATUS_ARB,&compile_status);
	if(compile_status==GL_FALSE) {
		glGetObjectParameterivARB(fs,GL_OBJECT_COMPILE_STATUS_ARB,&i);
		char* s=new char[32768];
		glGetInfoLogARB(fs,32768,NULL,s);
		printf("Shader compilation failed: %s\n", s);
		delete(s);
		p=0;
		fs=0;
		return false;
	}

	glAttachObjectARB(p,fs);
	glLinkProgram(p);
	glUseProgramObjectARB(p);

	/*
	char name_buf[]="tex32";
	for(int i=0;i<32;i++) {
		if(i<10) {
			name_buf[3]='0'+i;
			name_buf[4]=0;
		}
		else {
			name_buf[3]='0'+(i/10);
			name_buf[4]='0'+(i%10);
		}
		glUniform1iARB(glGetUniformLocationARB(p,name_buf),i);
	}
	*/

	glUseProgramObjectARB(0);
	return p;
}

GLuint vbo_create(int num_vertices,float* data) {
	GLuint id=0;
	glGenBuffers(1,&id);
	glBindBuffer(GL_ARRAY_BUFFER,id);
	glBufferData(GL_ARRAY_BUFFER,num_vertices*3*sizeof(float),data,GL_STATIC_DRAW);
	return id;
}
void vbo_use_pos(GLuint id) {
	glBindBuffer(GL_ARRAY_BUFFER,id);
	glVertexPointer(3,GL_FLOAT,0,(char *)NULL);
}
void vbo_use_coord(GLuint id) {
	glBindBuffer(GL_ARRAY_BUFFER,id);
	glTexCoordPointer(3,GL_FLOAT,0,(char *)NULL);
}
void vbo_use_pos_own(float* data) {
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glVertexPointer(3,GL_FLOAT,0,data);
}
void vbo_use_coord_own(float* data) {
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glTexCoordPointer(3,GL_FLOAT,0,data);
}

void vbo_draw(int vertices) {
	glDrawArrays(GL_TRIANGLE_STRIP,0,vertices);
}
void vbo_delete(GLuint vbo) {
	glDeleteBuffers(1,&vbo);
}

Font* font_load(char* map,char* spacings) {
	Font* font=new Font();

	font->texture=texture_load(map,false);

	FILE* s_file=fopen(spacings,"r");
	if(s_file) {
		for(int i=0;i<256;i++) {
			float d,o;
			if(fscanf(s_file,"%f %f",&d,&o)!=2) break;
			font->spacings[i]=d;
			font->offsets[i]=o;
		}
		fclose(s_file);
	}
	return font;
}

