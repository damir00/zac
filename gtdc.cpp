//game
#include "gui.h"
#include "human.h"
#include "world.h"
#include "audio.hpp"

#include "renderer.h"

//libs
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <vector>
#include <string>

#include <unistd.h>
#include <sys/time.h>

//rendering
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define ILUT_USE_OPENGL
#include "il.h"
#include "ilu.h"
#include "ilut.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
//#include <cstdio>

using namespace std;

int window_width=800;
int window_height=600;
float world_size=1000;
float mouse_sensitivity=0.2;
cpVect world_p1;
cpVect world_p2;

cpVect station_start;
cpVect station_end;

//resources
int texture_ground;
int texture_zombie[2];
int texture_zombie_corpse[2];
int texture_corpse[4];
int texture_wall[5];
int texture_door[5];
int texture_station;
int texture_legend;
int texture_map;
int texture_can;
//3 foods, 3 guns
int texture_pickups[6];
int texture_gun[3];
int texture_bullet[3];
int texture_flash;

int texture_tree[3];
int texture_sign;
int texture_plank;
int texture_logo;

Font* font;

//sound
ALuint buffer_zombie_pain;
ALuint buffer_zombie_scream;
ALuint buffer_zombie_death;
ALuint buffer_zombie_wander[2];
ALuint buffer_door_damage;

ALuint buffer_gun;
ALuint buffer_pain[2];
ALuint buffer_death;
ALuint buffer_explosion;

ALuint source_pain[2];
ALuint source_death;

vector<ALuint> sources_free;
vector<ALuint> sources_playing;

struct Wall {
	bool inserted;
	bool vbo_inited;
	cpShape* shape;
	cpVect start;
	cpVect end;
	float z_start;
	float z_end;
	float repeat_x;
	float repeat_y;
	int texture;
	float color;	//fake shading

	GLuint vbo;
	GLuint vbo_coord;
};
struct Ceil {
	cpVect start,end;
	int texture;
	float z;
	float repeat_x,repeat_y;
};

struct Door : Wall {
	bool inserted;
	cpBody* body;
	cpShape* shape;
	cpConstraint* constraint;
	cpConstraint* constraint2;
	float size;

	bool locked;
	float lock_health;
	float closed_angle;	//rad
};

struct Pickup {
	int type;
	cpBody* body;
	cpShape* shape;
	float player_dist;
};
struct Noise {
	cpVect pos;
	cpVect start,end;
	float range;
};
struct Grenade {
	cpBody* body;
	cpShape* shape;
	float time;
	float z;
	float z_vel;
};
struct Explosion {
	cpVect pos;
	float z;
	float radius;
	float life;
};

struct PickupHud {
	int type;
	float anim;
};


static float pickup_dist=3.5;	//sqrt'ed
static float zombie_dist=3.0;
static float zombie_damage=10;
static float zombie_cooloff=0.7;
static float zombie_size=3;	//height

static float zombie_see_dist=30*30;
static float zombie_see_dist_inside=10*10; //when player in buildings
static float zombie_hear_dist=100*100;		//guns, explosions

//vars
World* world;
Human* player;
cpConstraint* constraint_door;
vector<Human*> zombies;
//vector<Human*> corpses;
//vector<Wall*> walls;
//vector<Door*> doors;
vector<Ceil*> ceils;
//vector<Pickup*> pickups;
vector<Noise*> noises;
vector<Grenade*> grenades;
vector<Explosion*> explosions;

vector<PickupHud*> pickups_hud;

int zombies_alive;

int level;
int difficulty;

int main_menu_selection=0;
bool menu_on;
bool menu_interact;	//did user press space?
float menu_anim;	//transitions
//0: main
//1: difficulty select
//2: pause/death
//3: story
int menu_screen;

bool game_run;

bool player_inside;
bool game_won;
bool switch_level;
float won_fadeout;
int game_mode;	//0=story, 1=hunt

float weapon_damage[]={51,150,400};
float weapon_kick[]={1,2,3};
float weapon_noise[]={50,100,200};

int weapon_ammo[3];
int weapon_max_ammo[]={60,30,10};
int weapon_ammo_gain[]={7,4,1};

int current_weapon;
float flash_fadeout;

float shake_anim;
float shake_angle;	//rad
float player_height;

float fts;	//timestep for current frame in seconds

int map_size=64;
unsigned char* texture_map_data;	//just map
unsigned char* texture_map_data2;	//pointed locations

glm::mat4 matrix;

//some vbos
GLuint vbo_texcoord_normal;
GLuint vbo_pos_zombie;

//shaders
int shader_normal=0;
const char shader_normal_src[]=""
"uniform sampler2D tex;\n"
"uniform vec4 color;\n"

"void main() {\n"
"	vec4 c=texture2D(tex,gl_TexCoord[0].xy);\n"
"	float mult=min(0.8,gl_FragCoord.w*20.0);\n"
"	//float mult=1/gl_FragCoord.w;\n"
"   gl_FragColor=vec4(c.rgb*mult,c.a);\n"
"}";

char story_1[]=""
"The zombie outbreak spreads across the city.\n"
"As your refrigerator is running out of food,\n"
"you decide it's time to go out and face the zombies.\n"
"There is a safehouse deep in the woods,\n"
"that's where you'll be going.\n"
"But first you need to find a way out of the city.\n"
"The subway station is your only chance.\n\n[SPACE]";
char story_2[]=""
"After an hour of riding the rain you arrive close\n"
"to the safehouse. You leave the train and head\n"
"into the dark forest. It is filled with all too\n"
"familiar growling sounds.\n"
"There are signs on the trees, at least you're close\n"
"to your destination.\n\n[SPACE]";
char story_end[]=""
"After finally reaching the safehouse, you are greeted\n"
"by other survivors. They don't have much, but at least\n"
"they can offer shelter.\n"
"Every night the growling returns.\n"
"You know why they're coming back.\n"
"The zombies are coming for you.\n\n";


char story_hunt1[]=""
"The zombie outbreak spreads across the city.\n"
"As your refrigerator is running out of beer,\n"
"you decide it's time to go out and blast some zombies.\n"
"You lock&load your favorite guns and set off to waste\n"
"every last one of them."
".\n\n[SPACE]";
char story_hunt2[]=""
"After clearing the city you realize it's far from over.\n"
"You stock up on ammo and beer and continue your carnage.\n\n[SPACE]";
char story_hunt_end[]=""
"After hours of carnage you are the last man standing.\n"
"Congratulations!"
"\n[SPACE]";

char credits[]=""
"Zombies Are Coming\n\n"
"By Damir\nGraphics by Tashy\n\n[SPACE]";

float render_range_zombies=300;
float render_range_walls=500;
float render_range_pickups=50;

#ifdef __linux__
	#define sleep_ms(ms) usleep((ms)*1000)
#else
	#define sleep_ms(ms) Sleep(ms)
#endif

long get_ticks() {
#ifdef __linux__
	struct timeval mtime;
	gettimeofday(&mtime, NULL);
	return (mtime.tv_sec*1000000+mtime.tv_usec)/1000;
#else
	return GetTickCount();
#endif
}


ALuint audio_source_create(ALuint buffer,bool loop) {
	ALuint source;
	alGenSources(1, &source);

	alSourcef(source, AL_PITCH, 1);
	alSource3f(source, AL_POSITION, 0, 0, 0);
	alSource3f(source, AL_VELOCITY, 0, 0, 0);
	alSourcei(source, AL_LOOPING, loop);
	alSourcei(source, AL_BUFFER, buffer);
	return source;
}
void audio_source_play(ALuint source) {
	alSourcePlay(source);
}
bool audio_source_is_playing(ALuint source) {
	ALint playing;
	alGetSourcei(source, AL_SOURCE_STATE, &playing);
	return (playing==AL_PLAYING);
}

void audio_source_set(ALuint source,cpVect pos,cpVect vel) {
	alSource3f(source, AL_POSITION, pos.x, 0, pos.y);
	alSource3f(source, AL_VELOCITY, vel.x, 0, vel.y);
}
void audio_source_set_buffer(ALuint source,ALuint buffer) {
	alSourcei(source, AL_BUFFER, buffer);
}

void init_audio() {
	ALCdevice* device = alcOpenDevice(NULL);
	ALCcontext* context = alcCreateContext(device, NULL);
	alcMakeContextCurrent(context);
	int argc=0;
	alutInit(0,0);

	buffer_zombie_pain=alutCreateBufferFromFile("media/zombie_pain.wav");
	buffer_zombie_scream=alutCreateBufferFromFile("media/zombie_scream.wav");
	buffer_zombie_death=alutCreateBufferFromFile("media/zombie_death.wav");
	buffer_zombie_wander[0]=alutCreateBufferFromFile("media/zombie1.wav");
	buffer_zombie_wander[1]=alutCreateBufferFromFile("media/zombie2.wav");
	buffer_door_damage=alutCreateBufferFromFile("media/door_damage.wav");

	buffer_gun=alutCreateBufferFromFile("media/gun.wav");
	buffer_pain[0]=alutCreateBufferFromFile("media/pain1.wav");
	buffer_pain[1]=alutCreateBufferFromFile("media/pain2.wav");
	buffer_death=alutCreateBufferFromFile("media/death.wav");
	buffer_explosion=alutCreateBufferFromFile("media/explosion.wav");

	buffer_door_damage=alutCreateBufferFromFile("media/door_damage.wav");

	/*
	source_gun=audio_source_create(buffer_gun,false);
	source_pain[0]=audio_source_create(buffer_pain,false);
	source_pain[1]=audio_source_create(buffer_pain2,false);
	source_death=audio_source_create(buffer_death,false);
	*/

	for(int i=0;i<50;i++) {
		sources_free.push_back(audio_source_create(0,false));
	}
}

void audio_pull_source(ALuint buffer,cpVect pos,int gain=3) {
	if(sources_free.size()==0) return;
	ALuint src=sources_free.back();
	sources_free.pop_back();

	audio_source_set_buffer(src,buffer);
	audio_source_set(src,pos,cpvzero);
	audio_source_play(src);

	alSourcef(src, AL_GAIN, gain);

	sources_playing.push_back(src);

	//printf("free sources %d\n",sources_free.size());
}



float capf(float min,float max,float x) {
	if(x>max) return max;
	if(x<min) return min;
	return x;
}
float randInt(int low,int high) {
	if(low==high) return low;
	return (rand()%(high-low))+low;
}
float randFloat(float low,float high) {
	return (((float)rand())/RAND_MAX)*(high-low)+low;
}
cpVect randVect(cpVect start,cpVect end) {
	return cpv(
			randFloat(start.x,end.x),
			randFloat(start.y,end.y));
}
cpVect randIntVect(cpVect start,cpVect end) {
	return cpv(
			randInt(start.x,end.x),
			randInt(start.y,end.y));
}
bool pointInside(cpVect start,cpVect end,cpVect p) {
	return (p.x>=start.x && p.x<=end.x && p.y>=start.y && p.y<=end.y);
}
bool quadInside(cpVect start1,cpVect end1,cpVect start2,cpVect end2) {
	return (start1.x<end2.x && end1.x>start2.x &&
			start1.y<end2.y && end1.y>start2.y);

//		if(c->start.x<p2.x && c->end.x>p1.x &&
//					c->start.y<p2.y && c->end.y>p1.y)
}
bool pointInBuilding(cpVect p) {
	for(int i=0;i<ceils.size();i++) {
		if(pointInside(ceils[i]->start,ceils[i]->end,p)) return true;
	}
	return false;
}
bool quadInBuilding(cpVect p1,cpVect p2) {
	for(int i=0;i<ceils.size();i++) {
		Ceil* c=ceils[i];
	//	if(c->start.x<p2.x && c->end.x>p1.x &&
	//		c->start.y<p2.y && c->end.y>p1.y) return true;
		if(quadInside(c->start,c->end,p1,p2)) return true;


	}
	return false;
}



//very simple octree
int octree_max_level=7;
class Octree {
public:
	cpVect start;
	cpVect end;
	cpVect center;
	int level;

	cpVect p1,p2,p3,p4;

	vector<Wall*> walls;
	vector<Human*> humans;
	vector<Pickup*> pickups;
	vector<Door*> doors;
	vector<Ceil*> ceils;

	Octree *parent;
	Octree *children[4];
	Octree(Octree* _parent,cpVect _start,cpVect _end) {
		children[0]=children[1]=children[2]=children[3]=NULL;
		start=_start;
		end=_end;
		center=cpvlerp(start,end,0.5);

		p1=cpv(center.x,start.y);
		p2=cpv(end.x,center.y);
		p3=cpv(start.x,center.y);
		p4=cpv(center.x,end.y);
		parent=_parent;
	}
	~Octree() {
		delete(children[0]);
		delete(children[1]);
		delete(children[2]);
		delete(children[3]);
	}

	void add(cpVect pos,Wall* w) {
		cpVect p1,p2;
		if(level>=octree_max_level) {
			walls.push_back(w);
			return;
		}
		if(pointInside(start,center,pos)) {
			if(!children[0]) children[0]=new Octree(this,start,center);
			children[0]->level=level+1;
			children[0]->add(pos,w);
			return;
		}
		p1=cpv(center.x,start.y);
		p2=cpv(end.x,center.y);
		if(pointInside(p1,p2,pos)) {
			if(!children[1]) children[1]=new Octree(this,p1,p2);
			children[1]->level=level+1;
			children[1]->add(pos,w);
			return;
		}
		p1=cpv(start.x,center.y);
		p2=cpv(center.x,end.y);
		if(pointInside(p1,p2,pos)) {
			if(!children[2]) children[2]=new Octree(this,p1,p2);
			children[2]->level=level+1;
			children[2]->add(pos,w);
			return;
		}
		if(pointInside(center,end,pos)) {
			if(!children[3]) children[3]=new Octree(this,center,end);
			children[3]->level=level+1;
			children[3]->add(pos,w);
			return;
		}
	}
	void add(cpVect pos,Human* w) {
		cpVect p1,p2;
		if(level>=octree_max_level) {
			humans.push_back(w);
			return;
		}
		if(pointInside(start,center,pos)) {
			if(!children[0]) children[0]=new Octree(this,start,center);
			children[0]->level=level+1;
			children[0]->add(pos,w);
			return;
		}
		p1=cpv(center.x,start.y);
		p2=cpv(end.x,center.y);
		if(pointInside(p1,p2,pos)) {
			if(!children[1]) children[1]=new Octree(this,p1,p2);
			children[1]->level=level+1;
			children[1]->add(pos,w);
			return;
		}
		p1=cpv(start.x,center.y);
		p2=cpv(center.x,end.y);
		if(pointInside(p1,p2,pos)) {
			if(!children[2]) children[2]=new Octree(this,p1,p2);
			children[2]->level=level+1;
			children[2]->add(pos,w);
			return;
		}
		if(pointInside(center,end,pos)) {
			if(!children[3]) children[3]=new Octree(this,center,end);
			children[3]->level=level+1;
			children[3]->add(pos,w);
			return;
		}
	}

	void add(cpVect pos,Pickup* w) {
		if(level>=octree_max_level) {
			pickups.push_back(w);
			return;
		}
		if(pointInside(start,center,pos)) {
			if(!children[0]) children[0]=new Octree(this,start,center);
			children[0]->level=level+1;
			children[0]->add(pos,w);
			return;
		}
		if(pointInside(p1,p2,pos)) {
			if(!children[1]) children[1]=new Octree(this,p1,p2);
			children[1]->level=level+1;
			children[1]->add(pos,w);
			return;
		}
		if(pointInside(p3,p4,pos)) {
			if(!children[2]) children[2]=new Octree(this,p3,p4);
			children[2]->level=level+1;
			children[2]->add(pos,w);
			return;
		}
		if(pointInside(center,end,pos)) {
			if(!children[3]) children[3]=new Octree(this,center,end);
			children[3]->level=level+1;
			children[3]->add(pos,w);
			return;
		}
	}
	void add(cpVect pos,Door* w) {
		if(level>=octree_max_level) {
			doors.push_back(w);
			return;
		}
		if(pointInside(start,center,pos)) {
			if(!children[0]) children[0]=new Octree(this,start,center);
			children[0]->level=level+1;
			children[0]->add(pos,w);
			return;
		}
		if(pointInside(p1,p2,pos)) {
			if(!children[1]) children[1]=new Octree(this,p1,p2);
			children[1]->level=level+1;
			children[1]->add(pos,w);
			return;
		}
		if(pointInside(p3,p4,pos)) {
			if(!children[2]) children[2]=new Octree(this,p3,p4);
			children[2]->level=level+1;
			children[2]->add(pos,w);
			return;
		}
		if(pointInside(center,end,pos)) {
			if(!children[3]) children[3]=new Octree(this,center,end);
			children[3]->level=level+1;
			children[3]->add(pos,w);
			return;
		}
	}
	void add(cpVect pos,Ceil* w) {
		if(level>=octree_max_level) {
			ceils.push_back(w);
			return;
		}
		if(pointInside(start,center,pos)) {
			if(!children[0]) children[0]=new Octree(this,start,center);
			children[0]->level=level+1;
			children[0]->add(pos,w);
			return;
		}
		if(pointInside(p1,p2,pos)) {
			if(!children[1]) children[1]=new Octree(this,p1,p2);
			children[1]->level=level+1;
			children[1]->add(pos,w);
			return;
		}
		if(pointInside(p3,p4,pos)) {
			if(!children[2]) children[2]=new Octree(this,p3,p4);
			children[2]->level=level+1;
			children[2]->add(pos,w);
			return;
		}
		if(pointInside(center,end,pos)) {
			if(!children[3]) children[3]=new Octree(this,center,end);
			children[3]->level=level+1;
			children[3]->add(pos,w);
			return;
		}
	}
	void clear_zombies() {
		humans.clear();
		for(int i=0;i<4;i++) {
			if(children[i]) children[i]->clear_zombies();
		}
	}
	bool remove_pickup(Pickup* p) {
		for(int i=0;i<pickups.size();i++) {
			if(pickups[i]==p) {
				pickups.erase(pickups.begin()+i);
				return true;
			}
		}
		for(int i=0;i<4;i++) {
			if(children[i])
				if(children[i]->remove_pickup(p)) return true;
		}
		return false;
	}

	bool reinsert_zombie(Human* h,Octree* child) {
		if(!pointInside(start,end,h->body->p)) {
			if(parent) {
				if(parent==child) return false;
				return parent->reinsert_zombie(h,this);
			}
		}
		if(level>=octree_max_level) {
			humans.push_back(h);
			return true;
		}
		for(int i=0;i<4;i++) {
			if(children[i] && children[i]!=child) {
				if(children[i]->reinsert_zombie(h,this)) {
					return true;
				}
			}
		}
		return false;
	}

	void iterate_zombies(cpVect p1,cpVect p2,void (*update_func)(Human* h) ) {
		if(!quadInside(start,end,p1,p2)) return;
		for(int i=0;i<humans.size();i++) {
			update_func(humans[i]);

			if(!pointInside(start,end,humans[i]->body->p)) {
				if(parent) {
					if(parent->reinsert_zombie(humans[i],this))
						humans.erase(humans.begin()+i);
				}
			}

		}
		for(int i=0;i<4;i++) {
			if(children[i]) children[i]->iterate_zombies(p1,p2,update_func);
		}
	}
	void iterate_pickups(cpVect p1,cpVect p2,bool (*update_func)(Pickup* h) ) {
		if(!quadInside(start,end,p1,p2)) return;

		for(int i=0;i<pickups.size();i++) {
			if(update_func(pickups[i])) {
				pickups.erase(pickups.begin()+i);
				i--;
			}
		}
		for(int i=0;i<4;i++) {
			if(children[i]) children[i]->iterate_pickups(p1,p2,update_func);
		}
	}
	void iterate_doors(cpVect p1,cpVect p2,bool (*update_func)(Door* h) ) {
		if(!quadInside(start,end,p1,p2)) return;

		for(int i=0;i<doors.size();i++) {
			if(update_func(doors[i])) {
				doors.erase(doors.begin()+i);
				i--;
			}
		}
		for(int i=0;i<4;i++) {
			if(children[i]) children[i]->iterate_doors(p1,p2,update_func);
		}
	}
	void iterate_walls(cpVect p1,cpVect p2,void (*update_func)(Wall* h) ) {
		if(!quadInside(start,end,p1,p2)) return;

		for(int i=0;i<walls.size();i++) {
			update_func(walls[i]);
		}
		for(int i=0;i<4;i++) {
			if(children[i]) children[i]->iterate_walls(p1,p2,update_func);
		}
	}
	void iterate_ceils(cpVect p1,cpVect p2,void (*update_func)(Ceil* h) ) {
		if(!quadInside(start,end,p1,p2)) return;

		for(int i=0;i<ceils.size();i++) {
			update_func(ceils[i]);
		}
		for(int i=0;i<4;i++) {
			if(children[i]) children[i]->iterate_ceils(p1,p2,update_func);
		}
	}
};
Octree* octree=NULL;

//init for world
void octreeInit() {
	delete(octree);
	octree=new Octree(NULL,world_p1,world_p2);
	octree->level=0;
}


cpShape* queryShape(cpVect start,cpVect end) {
	return cpSpaceSegmentQueryFirst(world->space,start,end,
			CP_ALL_LAYERS,CP_NO_GROUP,NULL);
}

void addNoise(cpVect pos,float range) {
	Noise* n=new Noise();
	n->pos=pos;
	n->range=range;
	n->start=cpvsub(n->pos,cpv(range,range));
	n->end=cpvadd(n->pos,cpv(range,range));
	noises.push_back(n);
}

unsigned char* image_random(int w,int h,int depth) {
	int pixels=w*h;
	int bytes=pixels*depth;
	unsigned char* data=new unsigned char[bytes];

	for(int i=0;i<bytes;i+=3) {
		unsigned char f=randFloat(0.5,0.8)*256;
		data[i+0]=f;
		data[i+1]=f;
		data[i+2]=f;
	}
	return data;
}
unsigned char* image_random_green(int w,int h,int depth) {
	int pixels=w*h;
	int bytes=pixels*depth;
	unsigned char* data=new unsigned char[bytes];

	for(int i=0;i<bytes;i+=3) {
		unsigned char f=randFloat(0.3,0.5)*256;
		data[i+0]=0;
		data[i+1]=f;
		data[i+2]=0;
	}
	return data;
}

int texture_create(int width,int height,int depth,unsigned char* data) {
	GLuint id;
	glGenTextures(1,&id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

	GLint internal_formats[4]={GL_LUMINANCE,GL_LUMINANCE,GL_RGB,GL_RGBA8};
	GLenum formats[4]={GL_RGB,GL_RGB,GL_RGB,GL_RGBA};
	depth--;
	glTexImage2D(GL_TEXTURE_2D, 0, internal_formats[depth], width, height,0,
			formats[depth], GL_UNSIGNED_BYTE, data);

	return id;
}
void texture_update(int texture,int width,int height,int depth,unsigned char* data) {
	glBindTexture(GL_TEXTURE_2D, texture);
	GLint internal_formats[4]={GL_LUMINANCE,GL_LUMINANCE,GL_RGB,GL_RGBA8};
	GLenum formats[4]={GL_RGB,GL_RGB,GL_RGB,GL_RGBA};
	depth--;
	glTexImage2D(GL_TEXTURE_2D, 0, internal_formats[depth], width, height,0,
			formats[depth], GL_UNSIGNED_BYTE, data);
}

void put_pixel(unsigned char* buffer,int width,int height,int x,int y,char r,char g,char b) {
	if(x<0 || x>=width || y<0 || y>=height) return;
	int i=(y*height+x)*3;
	buffer[i+0]=r;
	buffer[i+1]=g;
	buffer[i+2]=b;
}
void draw_cross(unsigned char* buffer,int width,int height,int x,int y,char r,char g,char b) {
	put_pixel(buffer,width,height,x,y,r,g,b);
	put_pixel(buffer,width,height,x-1,y-1,r,g,b);
	put_pixel(buffer,width,height,x-1,y+1,r,g,b);
	put_pixel(buffer,width,height,x+1,y+1,r,g,b);
	put_pixel(buffer,width,height,x+1,y-1,r,g,b);
}

void update_map() {
	memcpy(texture_map_data2,texture_map_data,map_size*map_size*3);

	cpVect station_pos=cpvlerp(station_start,station_end,0.5);

	int px=(player->body->p.x+world_size/2)/world_size*map_size;
	int py=(player->body->p.y+world_size/2)/world_size*map_size;
	int sx=(station_pos.x+world_size/2)/world_size*map_size;
	int sy=(station_pos.y+world_size/2)/world_size*map_size;

	draw_cross(texture_map_data2,map_size,map_size,px,py,255,0,0);
	draw_cross(texture_map_data2,map_size,map_size,sx,sy,0,0,255);

	texture_update(texture_map,map_size,map_size,3,texture_map_data2);
}

void damage_player(float damage) {
	if(!player->alive) {
		game_won=true;
		return;
	}
	player->hit(damage);

	if(player->alive) {
		audio_source_play(source_pain[rand()%2]);
	}
	else {
		audio_source_play(source_death);
	}
}
void damage_door(Door* d,float damage) {
	d->lock_health-=damage;

	if(d->lock_health<=0) {
		//printf("door destroyed!\n");
		/*
		d->locked=false;
		cpRotaryLimitJoint* j=(cpRotaryLimitJoint*)d->constraint2;
		j->min=d->body->a -90/57.325;
		j->max=d->body->a + 0/57.325;
		*/
	}
}

bool fire_weapon_hit;
cpVect fire_weapon_end;
float fire_weapon_end_z;
float fire_weapon_static_t;
struct fire_weapon_s {
	float t;
	cpShape* s;
};
vector<fire_weapon_s> fire_weapon_shapes;

//t not ordered!!
void fire_weapon_it(cpShape *shape, cpFloat t, cpVect n, void *data) {

//	printf("weapon hit t %f\n",t);

	if(fire_weapon_hit || shape->body==player->body) return;
	if(!shape->body->data) {
		//fire_weapon_hit=true;
	//	printf("weapon hit static\n");
		return;	//static shapes probably
	}

	//gun doesn't pass trough
	if(current_weapon==0) {
		//fire_weapon_hit=true;
	}

	//calculate height..
	float z=cpflerp(player_height,fire_weapon_end_z,t);
	//printf("hit something %f? hit z %f\n",t,z);

	if(z<0 || z>zombie_size) return;

	if(shape->collision_type==2) {
		Door* d=(Door*) shape->body->data;
		d->lock_health-=weapon_damage[current_weapon]/5;
		return;
	}

	Human* h=(Human*)shape->body->data;
	bool alive=h->alive;
	h->hit(weapon_damage[current_weapon]);
	if(h->alive) {
		audio_pull_source(buffer_zombie_pain,h->body->p);
	}
	else if(alive) {
		audio_pull_source(buffer_zombie_death,h->body->p);
		zombies_alive--;
	}
	cpVect p2=player->body->p;
	cpVect p1=h->body->p;
	cpBodyApplyImpulse(h->body,cpvsub(p1,cpvlerpconst(p1,p2,weapon_kick[current_weapon])),cpvzero);

	/*
	world->removeHuman(h);

	for(int i=0;i<zombies.size();i++) {
		if(h==zombies[i]) {
			zombies.erase(zombies.begin()+i);
			break;
		}
	}
	delete(h);
	*/
}

void addGrenade();

void fire_weapon() {
	if(weapon_ammo[current_weapon]<=0) return;

	weapon_ammo[current_weapon]--;

	if(current_weapon==2) {
		addGrenade();
		return;
	}

	fire_weapon_hit=false;
	fire_weapon_shapes.clear();

	float range=100;

	fire_weapon_end=cpvadd(player->body->p,cpvmult(cpvforangle((player->angle_x+90)/57.325),range));
	fire_weapon_end_z=player_height-sin(player->angle_y/57.325)*range;

	/*
	printf("player height %f, angle %f, sin(angle) %f, end z %f\n",
			player_height,player->angle_y,
			sin(player->angle_y/57.325),
			fire_weapon_end_z);
	 */

	cpSpaceSegmentQuery(world->space,player->body->p,fire_weapon_end,
			CP_ALL_LAYERS,CP_NO_GROUP,fire_weapon_it,NULL);

	/*
	cpSegmentQueryInfo info;
	cpSpaceSegmentQueryFirst(world->space,player->body->p,fire_weapon_end,
			CP_ALL_LAYERS,CP_NO_GROUP,&info);
	if(info.shape && info.shape->body->data) {
		float z=cpflerp(player_height,fire_weapon_end_z,info.t);
			if(z>0 && z<zombie_size) {
			if(info.shape->collision_type==2) {
				Door* d=(Door*) info.shape->body->data;
				d->lock_health-=weapon_damage[current_weapon]/5;
			}
			else {
				Human* h=(Human*)info.shape->body->data;
				h->hit(weapon_damage[current_weapon]);
				cpVect p2=player->body->p;
				cpVect p1=h->body->p;
				cpBodyApplyImpulse(h->body,cpvsub(p1,cpvlerpconst(p1,p2,weapon_kick[current_weapon])),cpvzero);
			}
		}
	}
	*/

	flash_fadeout=0.1;
	bool inside=pointInBuilding(player->body->p);
	addNoise(player->body->p,
			inside ? weapon_noise[current_weapon]*0.5 : weapon_noise[current_weapon]);

	//audio_source_play(source_gun);
	audio_pull_source(buffer_gun,player->body->p);
}

cpVect fire_explosion_pos;
float fire_explosion_radius2;

void damage_human(Human* h) {
	float d=cpvdistsq(h->body->p,fire_explosion_pos);
	if(d>fire_explosion_radius2) {
		printf("dist %f, radius %f\n",d,fire_explosion_radius2);
		return;
	}

	float damage=(1-d/fire_explosion_radius2)*400;

	cpVect norm=cpvsub(h->body->p,fire_explosion_pos);

	cpBodyApplyImpulse(h->body,cpvmult(norm,damage/600),cpvzero);

	printf("explosion, damage %f\n",damage);

	bool alive=h->alive;
	h->hit(damage);
	if(h->alive) {
		audio_pull_source(buffer_zombie_pain,h->body->p);
	}
	else if(alive) {
		audio_pull_source(buffer_zombie_death,h->body->p);
		zombies_alive--;
	}
}

void range_bb(cpVect pos,float range,cpVect* p1,cpVect* p2);

void fire_explosion(cpVect pos,float z,float radius) {
	Explosion* e=new Explosion;
	e->pos=pos;
	e->z=z;
	e->radius=radius;
	e->life=1;

	cpVect p1,p2;
	range_bb(pos,radius,&p1,&p2);

	fire_explosion_pos=pos;
	fire_explosion_radius2=radius*radius;

	printf("explosion range %f\n",radius);

	octree->iterate_zombies(p1,p2,damage_human);

	explosions.push_back(e);
	audio_pull_source(buffer_explosion,pos,50);
	addNoise(pos,100);
}

void player_pickup(int type) {
	PickupHud* h=new PickupHud();
	h->anim=2;
	h->type=type;
	pickups_hud.push_back(h);

	if(type<3) player->health+=10;
	else {
		int t=type-3;
		weapon_ammo[t]=min(weapon_max_ammo[t],weapon_ammo[t]+
				weapon_ammo_gain[t]);
	}
}

void human_check_insert_shape(Human* h) {
	if(!h->shape_inserted) {
		cpSpaceAddBody(world->space,h->body);
		cpSpaceAddShape(world->space,h->shape);
		h->shape_inserted=true;
	}
}

bool update_pickup(Pickup* p) {
	p->player_dist=cpvdistsq(p->body->p,player->body->p);
	if(p->player_dist<pickup_dist) {
		player_pickup(p->type);
		delete(p);
		return true;
	}
	return false;
}
bool update_door(Door* d) {
	if(d->lock_health<0) {
		cpSpaceRemoveConstraint(world->space,d->constraint);
		cpSpaceRemoveConstraint(world->space,d->constraint2);
		cpSpaceRemoveShape(world->space,d->shape);
		cpSpaceRemoveBody(world->space,d->body);
		delete(d);
		return true;
	}

	if(!d->inserted) {
		cpSpaceAddBody(world->space,d->body);
		cpSpaceAddShape(world->space,d->shape);
		cpSpaceAddConstraint(world->space,d->constraint);
		cpSpaceAddConstraint(world->space,d->constraint2);
		d->inserted=true;
	}

	return false;
}

void update_human(Human* h) {
	cpBody* b=h->body;
	cpVect force=cpvmult(
					cpvsub(
							cpvmult(cpvclamp(h->heading,1),h->speed),
							b->v),
					0.3);
	cpBodyApplyForce(b,force,cpvzero);
}
void update_human_post(Human* h) {
	cpBodyResetForces(h->body);
	h->heading=cpvzero;
}

void update_zombie(Human* h) {
	human_check_insert_shape(h);

	if(!h->alive) {
		update_human(h);
		return;
	}

	cpVect dif=cpvsub(player->body->p,h->body->p);
	h->player_distance=cpvlengthsq(dif);

	bool prev_player_visible=h->player_visible;
	h->player_visible=false;

	//zombie attack logic
	if(!player_inside) {
		if(h->player_distance<zombie_see_dist) {

			//also check line of sight
			if(queryShape(h->body->p,player->body->p)==player->shape) {
				h->interest_point=player->body->p;
				h->interest_level=1;
				h->player_visible=true;
				//printf("zombie sees you\n");
			}
		}
	}
	else {
		if(h->player_distance<zombie_see_dist_inside) {
			//also check line of sight
			if(queryShape(h->body->p,player->body->p) == player->shape) {
				//printf("zombie sees you\n");
				h->interest_point=player->body->p;
				h->interest_level=1;
				h->player_visible=true;
			}

		}
	}

	bool following_sound=false;

	if(h->interest_level<0.9) {
		for(int i=0;i<noises.size();i++) {
			Noise* n=noises[i];
			if(pointInside(n->start,n->end,h->body->p)) {
				h->interest_level=0.8;
				h->interest_point=n->pos;
				//printf("Zombie coming to check out noise\n");
				following_sound=true;
			}
		}
	}

	if(!following_sound && !h->player_visible && prev_player_visible) {
		cpVect interest_dif=cpvsub(h->interest_point,h->body->p);
		h->interest_point=cpvadd(h->interest_point,cpvmult(cpvnormalize(interest_dif),3));
	}

	/*
	if( (player_inside && h->player_distance<zombie_see_dist_inside) ||
		(!player_inside && h->player_distance<zombie_see_dist) ) {
		h->interest_point=player->body->p;
		h->interest_level=1;
	}
	*/

	h->interest_level-=fts*0.1;

	if(h->interest_level<0) {
		h->interest_point=randVect(world_p1,world_p2);
		h->interest_level=0.5;
	}

	cpVect interest_dif=cpvsub(h->interest_point,h->body->p);
	float angle=atan2(interest_dif.y,interest_dif.x);

	h->angle_x=angle*57.325-90;
	h->goForward(cpfmax(0.5,h->interest_level));

	h->attack_cooloff-=fts;

	if(h->player_distance<zombie_dist) {
		//printf("zombie close\n");
		if(h->attack_cooloff<0 && queryShape(h->body->p,player->body->p)==player->shape) {
			//printf("zombie attacking, player %f\n",player->health);
			damage_player(zombie_damage);
			h->attack_cooloff=zombie_cooloff;
		}
	}

	if(h->player_distance<250) {
		if(rand()%100==3) {
			audio_pull_source(buffer_zombie_wander[rand()%2],h->body->p);
		}
	}
	update_human(h);
}

void range_bb(cpVect pos,float range,cpVect* p1,cpVect* p2) {
	range/=2;
	*p1=cpvsub(pos,cpv(range,range));
	*p2=cpvadd(pos,cpv(range,range));
}

void update(long ts) {

	fts=(float)ts/1000;

	if(game_won) {
		won_fadeout+=fts;

		if(won_fadeout>1) {
			if(player->alive)
				switch_level=true;
			else {
				menu_on=true;
				menu_screen=2;
				main_menu_selection=1;
			}
		}

		return;
	}

	if(game_mode==0) {
		if(pointInside(station_start,station_end,player->body->p)) {
			game_won=true;
			return;
		}
	}
	else {
		if(zombies_alive==0) {
			game_won=true;
			return;
		}
	}

	if(gtdc_gui::input_esc) {
		menu_on=true;
		menu_screen=2;
		main_menu_selection=0;
	}

	if(level==0) {
		update_map();
	}

	bool walking=false;

	if(player->alive) {
		player->angle_x+=gtdc_gui::input_mouse_dx*mouse_sensitivity;
		player->angle_y=capf(-90,90,player->angle_y+gtdc_gui::input_mouse_dy*mouse_sensitivity);

		if(gtdc_gui::input_1) {
			current_weapon=0;
		}
		else if(gtdc_gui::input_2) {
			current_weapon=1;
		}
		else if(gtdc_gui::input_3) {
			current_weapon=2;
		}

		if(gtdc_gui::input_go_forward) {
			player->goForward(1);
			walking=true;
		}
		if(gtdc_gui::input_go_back) {
			player->goForward(-1);
			walking=true;
		}
		if(gtdc_gui::input_go_left) {
			player->goSide(1);
			walking=true;
		}
		if(gtdc_gui::input_go_right) {
			player->goSide(-1);
			walking=true;
		}

		if(gtdc_gui::input_mouse_clicked[0]) {
			fire_weapon();
		}

		if(gtdc_gui::input_interact) {
			if(!constraint_door) {

				cpVect end=cpvadd(player->body->p,cpvmult(cpvforangle((player->angle_x+90)/57.325),3));
				cpShape* shape=queryShape(player->body->p,end);
				if(shape) {
					constraint_door=cpPinJointNew(player->body,shape->body,
							cpvzero,cpv(2,0));
					constraint_door->maxForce=100;
					cpSpaceAddConstraint(world->space,constraint_door);
				}
			}
		}
		else {
			if(constraint_door) {
				cpSpaceRemoveConstraint(world->space,constraint_door);
				cpConstraintDestroy(constraint_door);
				constraint_door=NULL;
			}
		}

		if(gtdc_gui::input_use) {
			cpShape* s=queryShape(player->body->p,
					cpvadd(player->body->p,
							cpvmult(cpvforangle((player->angle_x+90)/57.325),3)));

			if(s) {
				if(s->collision_type==2) {
					Door* d=(Door*)s->body->data;

					if(!d->locked && cpfabs(d->body->a-d->closed_angle)<10/57.325) {
						d->locked=true;

						cpRotaryLimitJoint* r=(cpRotaryLimitJoint*)d->constraint2;
						r->max=d->closed_angle;
						r->min=d->closed_angle;
					}
				}
				else if(s->collision_type==4) {
					Human* h=(Human*)s->body->data;
					if(h->pickups.size()>0) {
						player_pickup(h->pickups.back());
						h->pickups.pop_back();
					}
				}
			}
		}
	}

	float shake_mult=1;
	shake_anim+=fts;

	if(player->health>30) {
		if(walking) shake_anim+=fts*2.5;
		player_height=2;
		shake_angle=sin(shake_anim)*1.5*shake_mult;
		player->speed=0.01;
	}
	else {
		if(walking) shake_anim+=fts*1.5;
		player->speed=0.002;
		player_height=0.5;
		shake_angle=5*shake_mult + sin(shake_anim)*5*shake_mult;
	}
	if(!player->alive) {
		shake_angle=70;
	}


	flash_fadeout-=fts;

	/*
	printf("station start %f %f end %f %f, player %f %f\n",
			station_start.x,station_start.y,
			station_end.x,station_end.y,
			player->body->p.x,player->body->p.y);
			*/

	//printf("space %d\n",gtdc_gui::input_interact);

	//printf("player rot %f\n",player->body->a);

	/*
	if(constraint_door) {
		cpPinJoint* pin=(cpPinJoint*)constraint_door;
		float dist=cpvdist(pin->anchr1,pin->anchr2);

		printf("dist %f, r1 %f %f r2 %f %f, a %f %f\n",
				dist,
				pin->r1.x,pin->r1.y,
				pin->r2.x,pin->r2.y,
				pin->constraint.a->a,
				pin->constraint.b->a
				);
	}
	*/

	player_inside=pointInBuilding(player->body->p);

	cpVect it_p1,it_p2;
	cpVect z_p1,z_p2;

	range_bb(player->body->p,300,&z_p1,&z_p2);
	octree->iterate_zombies(z_p1,z_p2,update_zombie);
	update_human(player);

	range_bb(player->body->p,5,&it_p1,&it_p2);
	octree->iterate_pickups(it_p1,it_p2,update_pickup);

	range_bb(player->body->p,50,&it_p1,&it_p2);
	octree->iterate_doors(it_p1,it_p2,update_door);

	for(int i=0;i<noises.size();i++) {
		delete(noises[i]);
	}
	noises.clear();

	world->update(ts);

	octree->iterate_zombies(z_p1,z_p2,update_human_post);
	update_human_post(player);

	/*
	octree->clear_zombies();
	for(int i=0;i<zombies.size();i++) {
		octree->add(zombies[i]->body->p,zombies[i]);
	}
	*/

	for(int i=0;i<pickups_hud.size();i++) {
		PickupHud* h=pickups_hud[i];
		h->anim-=fts;
		if(h->anim<0) {
			pickups_hud.erase(pickups_hud.begin()+i);
			i--;
			delete(h);
		}
	}

	float gravity=-10;
	for(int i=0;i<grenades.size();i++) {
		Grenade* g=grenades[i];
		g->z_vel+=gravity*fts;
		g->z+=g->z_vel*fts;
		if(g->z<0) {
			g->z=0;
			if(g->z_vel<0) {
				g->z_vel*=-0.5;
			}
		}
		g->time-=fts;
		if(g->time<0) {
			grenades.erase(grenades.begin()+i);

			cpSpaceRemoveShape(world->space,g->shape);
			cpSpaceRemoveBody(world->space,g->body);

			fire_explosion(g->body->p,g->z,30);

			cpBodyDestroy(g->body);
			cpShapeDestroy(g->shape);

			i--;
			delete(g);
		}
	}
	for(int i=0;i<explosions.size();i++) {
		Explosion* e=explosions[i];
		e->life-=fts*4;
		if(e->life<0) {
			explosions.erase(explosions.begin()+i);
			delete(e);
			i--;
		}
	}

	//printf("%d sources playing\n",sources_playing.size());

	for(int i=0;i<sources_playing.size();i++) {
		//printf("source %d playing?\n",i);
		if(!audio_source_is_playing(sources_playing[i])) {
			sources_free.push_back(sources_playing[i]);
			sources_playing.erase(sources_playing.begin()+i);
			i--;
		}
	}

	alListener3f(AL_POSITION, player->body->p.x, 0,player->body->p.y);
	alListener3f(AL_VELOCITY, player->body->v.x, 0,player->body->v.y);
	alListener3f(AL_ORIENTATION, player->body->rot.x, 0, player->body->rot.y);

	/*
	audio_source_set(source_gun,player->body->p,player->body->v);
	audio_source_set(source_pain[0],player->body->p,player->body->v);
	audio_source_set(source_pain[1],player->body->p,player->body->v);
	audio_source_set(source_death,player->body->p,player->body->v);
	*/
}

void render_texture_quad(cpVect p1,cpVect p2,int texture) {
	glBindTexture(GL_TEXTURE_2D,texture);
	glBegin(GL_QUADS);
	glTexCoord2f(0,0);	glVertex2f(p1.x,p1.y);
	glTexCoord2f(0,1);	glVertex2f(p1.x,p2.y);
	glTexCoord2f(1,1);	glVertex2f(p2.x,p2.y);
	glTexCoord2f(1,0);	glVertex2f(p2.x,p1.y);
	glEnd();
}

void render_zombie(Human* z) {

	cpVect dif=cpvsub(player->body->p,z->body->p);

	if(z->player_distance>10000) return;

	float angle=atan2(dif.y,dif.x);
	float s=zombie_size/2;

	glPushMatrix();
	glTranslatef(z->body->p.x,s,z->body->p.y);
	glRotatef(270-angle*57.325,0,1,0);

	if(z->alive) glBindTexture(GL_TEXTURE_2D,z->texture_alive);
	else {
		glBindTexture(GL_TEXTURE_2D,z->texture_dead);
	//	glTranslatef(0,-1,0);
	//	glRotatef(90,0,0,1);
	}

	vbo_use_coord(vbo_texcoord_normal);
	vbo_use_pos(vbo_pos_zombie);
	vbo_draw(4);

	glPopMatrix();
}
void render_wall(Wall* w) {
	glBindTexture(GL_TEXTURE_2D,w->texture);


//	glColor3f(c,c,c);
	/*
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(rx,0);		glVertex3f(w->start.x,w->z_start,w->start.y);
	glTexCoord2f(rx,ry);	glVertex3f(w->start.x,w->z_end,w->start.y);
	glTexCoord2f(0,0);		glVertex3f(w->end.x,w->z_start,w->end.y);
	glTexCoord2f(0,ry);		glVertex3f(w->end.x,w->z_end,w->end.y);
	glEnd();
	*/

	if(!w->vbo_inited) {
		float rx=w->repeat_x;
		float ry=w->repeat_y;
		float c=w->color;
		float data[]={
				w->start.x,w->z_start,w->start.y,
				w->start.x,w->z_end,w->start.y,
				w->end.x,w->z_start,w->end.y,
				w->end.x,w->z_end,w->end.y
		};
		float data_coord[]={
				w->repeat_x,0,0,
				w->repeat_x,w->repeat_y,0,
				0,0,0,
				0,w->repeat_y,0
		};

		w->vbo=vbo_create(4,data);
		w->vbo_coord=vbo_create(4,data_coord);
		w->vbo_inited=true;
	}

	vbo_use_coord(w->vbo_coord);
	vbo_use_pos(w->vbo);
	vbo_draw(4);
}
bool render_door(Door* w) {

	float rx=w->repeat_x;
	float ry=w->repeat_x;
	float c=w->color;

	cpVect p1=cpv(w->body->p.x,w->body->p.y);
	cpVect p2=cpvadd(p1,cpvmult(cpvforangle(w->body->a),w->size));

	glBindTexture(GL_TEXTURE_2D,w->texture);
	glBegin(GL_TRIANGLE_STRIP);
	glColor3f(c,c,c);

	glTexCoord2f(0,0); 		glVertex3f(p1.x,w->z_start,p1.y);
	glTexCoord2f(0,ry); 	glVertex3f(p1.x,w->z_end,p1.y);
	glTexCoord2f(rx,0);		glVertex3f(p2.x,w->z_start,p2.y);
	glTexCoord2f(rx,ry);	glVertex3f(p2.x,w->z_end,p2.y);
	glEnd();

	if(w->locked) {
		cpVect center=cpvlerp(p1,p2,0.5);
		float center_z=cpflerp(w->z_start,w->z_end,0.5);
		cpVect size=cpv(2.2,0.5);

		float z1=cpflerp(w->z_start,w->z_end,0.4);
		float z2=cpflerp(w->z_start,w->z_end,0.6);

		cpVect offset=cpvmult(cpvforangle(w->body->a+90/57.325),-0.01);
		p1=cpvlerpconst(cpvadd(p1,offset),p2,-0.25);
		p2=cpvlerpconst(cpvadd(p2,offset),p1,-0.25);

		glBindTexture(GL_TEXTURE_2D,texture_plank);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0); 		glVertex3f(p1.x,z1,p1.y);
		glTexCoord2f(0,ry); 	glVertex3f(p1.x,z2,p1.y);
		glTexCoord2f(rx,ry);	glVertex3f(p2.x,z2,p2.y);
		glTexCoord2f(rx,0);		glVertex3f(p2.x,z1,p2.y);
		glEnd();

		glPopMatrix();
	}
	return false;
}
void render_ceil(Ceil* c) {

	float rx=c->repeat_x;
	float ry=c->repeat_y;

	glBindTexture(GL_TEXTURE_2D,c->texture);

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(0,ry);		glVertex3f(c->start.x,c->z,c->start.y);
	glTexCoord2f(rx,0);		glVertex3f(c->start.x,c->z,c->end.y);
	glTexCoord2f(rx,ry);	glVertex3f(c->end.x,c->z,c->start.y);
	glTexCoord2f(0,0);		glVertex3f(c->end.x,c->z,c->end.y);
	glEnd();
}
bool render_pickup(Pickup* p) {

	if(p->player_dist>400) return false;

	float s=0.5;
	cpVect dif=cpvsub(player->body->p,p->body->p);

	float angle=atan2(dif.y,dif.x);

	glBindTexture(GL_TEXTURE_2D,texture_pickups[p->type]);
	glPushMatrix();

	glTranslatef(p->body->p.x,s,p->body->p.y);
	glRotatef(270-angle*57.325,0,1,0);

	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(1,1);	glVertex3f(s,s,0);
	glTexCoord2f(0,1);	glVertex3f(-s,s,0);
	glTexCoord2f(1,0);	glVertex3f(s,-s,0);
	glTexCoord2f(0,0);	glVertex3f(-s,-s,0);
	glEnd();

	glPopMatrix();

	return false;
}

void renderOctree(Octree* tree,cpVect start,cpVect end) {
	if(!quadInside(tree->start,tree->end,start,end)) return;

	glColor3f(0.5,0.5,0.5);

	for(int i=0;i<4;i++) {
		if(tree->children[i]) renderOctree(tree->children[i],start,end);
	}
}

void render_text(Font* f,cpVect pos,char* text,float size) {
	glBindTexture(GL_TEXTURE_2D,f->texture);

	float x=pos.x;
	int rows=10;

	glEnable(GL_ALPHA_TEST);
	//glBlendFunc(GL_ONE,GL_ONE);

	for(int i=0;text[i]!=0;i++) {
		int chr=text[i]-32;
		if(text[i]==' ') {
			x+=size/3;
			continue;
		}
		if(text[i]=='\n') {
			x=pos.x;
			pos.y-=size;
			continue;
		}

		float spacing=f->spacings[chr-1] * size;
		float offset=f->offsets[chr-1]* size;

		/*
		cpVect c1=
				cpv(chr%rows,
						(chr-1)/rows+1)*
				(1/(float)rows);

		cpVect c2=c1+(cpv(-1,-1)* (1/(float)rows));
		*/
		cpVect c1,c2;

		/*
		c2.x=(chr%rows)/(float)rows;
		c1.y=((chr-1)/rows+1)/(float)rows;
		c1.x=c2.x+(1/(float)rows);
		c2.y=(c1.y-(1/(float)rows));
		float t=1-c2.y;
		c2.y=1-c1.y;
		c1.y=t;
		*/

		float frows=rows;
		float cd=1/frows;

		chr--;

		c2.x=(chr%rows)/frows;
		c1.x=c2.x+cd;

		c1.y=1-(chr/rows)/frows;
		c2.y=c1.y-cd;


		cpVect p1=cpv(x,pos.y);
		cpVect p2=p1+cpv(size,size);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(c2.x,c2.y); glVertex2f(p1.x,p1.y);
		glTexCoord2f(c1.x,c2.y); glVertex2f(p2.x,p1.y);
		glTexCoord2f(c2.x,c1.y); glVertex2f(p1.x,p2.y);
		glTexCoord2f(c1.x,c1.y); glVertex2f(p2.x,p2.y);
		glEnd();

		x+=spacing;
	}
}
cpVect text_bounds(Font* f,char* text,float size) {
	cpVect pos=cpvzero;
	float x=0;

	float max_x=0;

	for(int i=0;text[i]!=0;i++) {
		int chr=text[i]-32;
		if(text[i]==' ') {
			x+=size/3;
			continue;
		}
		if(text[i]=='\n') {
			max_x=cpfmax(x,max_x);
			x=pos.x;
			pos.y+=size;
		}

		float spacing=f->spacings[chr-1] * size;
		float offset=f->offsets[chr-1]* size;
		x+=spacing;
	}
	max_x=cpfmax(x,max_x);
	return cpv(max_x,-(pos.y+size));
}

void render() {

//	glLoadMatrixf(glm::value_ptr(matrix));

	shader_use(shader_normal);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glColor3f(1,1,1);

	glRotatef(shake_angle,0,0,1);

	glRotatef(player->angle_y,1,0,0);
	glRotatef(180+player->angle_x,0,1,0);
	glTranslatef(0,-player_height,0);

	float s=100;
	float r=s/5;
	cpVect o1=cpv(0,0)-player->body->p*(r/s*0.5);
	cpVect o2=cpv(r,r)-player->body->p*(r/s*0.5);
	cpVect quad_v1=cpv(s,s);
	cpVect quad_v2=cpv(-s,-s);
	glDisable(GL_ALPHA_TEST);

	for(int x=-1;x<=1;x++) {
		for(int y=-1;y<=1;y++) {
			cpVect offset=cpv(x*s,y*s);
			cpVect p1=cpvadd(quad_v1,offset);
			cpVect p2=cpvadd(quad_v2,offset);

			glBindTexture(GL_TEXTURE_2D,texture_ground);
			glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(o1.x,o1.y);	glVertex3f(p1.x,0,p1.y);
			glTexCoord2f(o2.x,o1.y);	glVertex3f(p2.x,0,p1.y);
			glTexCoord2f(o1.x,o2.y);	glVertex3f(p1.x,0,p2.y);
			glTexCoord2f(o2.x,o2.y);	glVertex3f(p2.x,0,p2.y);
			glEnd();
		}
	}

/*
	glBindTexture(GL_TEXTURE_2D,texture_ground);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2f(o1.x,o1.y);	glVertex3f(s,0,s);
	glTexCoord2f(o2.x,o1.y);	glVertex3f(-s,0,s);
	glTexCoord2f(o1.x,o2.y);	glVertex3f(s,0,-s);
	glTexCoord2f(o2.x,o2.y);	glVertex3f(-s,0,-s);
	glEnd();
*/
	glTranslatef(-player->body->p.x,0,-player->body->p.y);

	/*
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glVertex2f(player->body->p.x,player_height,player->body->p.y);
	glEnd();
	glEnable(GL_TEXTURE_2D);
	*/


//	player->angle_x++;

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.5);

	glColor3f(1,1,1);

	cpVect p1,p2;
	range_bb(player->body->p,render_range_zombies,&p1,&p2);
	octree->iterate_zombies(p1,p2,render_zombie);

	range_bb(player->body->p,render_range_walls,&p1,&p2);

	octree->iterate_walls(p1,p2,render_wall);
	octree->iterate_doors(p1,p2,render_door);

	range_bb(player->body->p,render_range_pickups,&p1,&p2);
	octree->iterate_pickups(p1,p2,render_pickup);
	glColor3f(0.5,0.5,0.5);
	octree->iterate_ceils(p1,p2,render_ceil);

	//renderOctree(octree,p1,p2);

	glColor3f(1,1,1);

	glBindTexture(GL_TEXTURE_2D,texture_bullet[2]);
	for(int i=0;i<grenades.size();i++) {
		Grenade* p=grenades[i];

		cpVect dif=cpvsub(player->body->p,p->body->p);
		float angle=atan2(dif.y,dif.x);

		float s=0.5;

		glPushMatrix();
		glTranslatef(p->body->p.x,s+p->z,p->body->p.y);
		glRotatef(270-angle*57.325,0,1,0);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(1,1);	glVertex3f(s,s,0);
		glTexCoord2f(0,1);	glVertex3f(-s,s,0);
		glTexCoord2f(1,0);	glVertex3f(s,-s,0);
		glTexCoord2f(0,0);	glVertex3f(-s,-s,0);
		glEnd();

		glPopMatrix();
	}

	glDisable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);

	glBindTexture(GL_TEXTURE_2D,texture_flash);
	for(int i=0;i<explosions.size();i++) {
		Explosion* p=explosions[i];

		cpVect dif=cpvsub(player->body->p,p->pos);
		float angle=atan2(dif.y,dif.x);

		float s=p->radius*(1-p->life);

		glPushMatrix();
		glTranslatef(p->pos.x,(1-p->life)*2+p->z,p->pos.y);
		glRotatef(270-angle*57.325,0,1,0);

		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(1,1);	glVertex3f(s,s,0);
		glTexCoord2f(0,1);	glVertex3f(-s,s,0);
		glTexCoord2f(1,0);	glVertex3f(s,-s,0);
		glTexCoord2f(0,0);	glVertex3f(-s,-s,0);
		glEnd();

		glPopMatrix();

	}


	glDisable(GL_BLEND);

	glColor3f(1,1,1);

	glLoadIdentity();
	glTranslatef(0,0,-1);
	glDisable(GL_DEPTH_TEST);

	shader_use(0);

	if(player->alive) {
		float add=0;
		if(flash_fadeout>0 && current_weapon!=2) {
			glEnable(GL_BLEND);
			//flash
			render_texture_quad(cpv(-0.2,-0.2),cpv(0.2,0.2),texture_flash);
			add=flash_fadeout/2;
			glDisable(GL_BLEND);
		}

		glEnable(GL_ALPHA_TEST);
		if(current_weapon!=2) {
			float width2=0.125;
			if(current_weapon==1) {
				width2=0.5;
			}

			p1=cpv(-width2 +1.0/64.0,	-1 +1.0/64.0 +add);
			p2=cpv(width2	 +1.0/64.0,	0 +1.0/64.0 +add);

			//gun
			render_texture_quad(p1,p2,texture_gun[current_weapon]);
		}

		glColor3f(1,1,1);
		//current gun & ammo

		//full screen [-0.5,0.5][-0.4,0.4]

		p1=cpv(
				-0.5,
				-0.4);
		p2=cpv(
				-0.5+0.1,
				-0.4+0.1);
		render_texture_quad(p1,p2,texture_pickups[current_weapon+3]);

		cpVect ammo_size=cpv(0.05,0.05);
		cpVect ammo_pos=cpv(-0.35,-0.4);
		for(int i=0;i<weapon_ammo[current_weapon];i++) {
			cpVect p=ammo_pos+cpv(ammo_size.x*i/4,0);
			render_texture_quad(
					p,cpvadd(p,ammo_size),texture_bullet[current_weapon]);
		}

	}
	else {
		glEnable(GL_ALPHA_TEST);
	}

	cpVect pickups_start=cpv(0.3,-0.4);
	cpVect pickups_size=cpv(0.1,0.1);

	for(int i=0;i<pickups_hud.size();i++) {
		glBindTexture(GL_TEXTURE_2D,texture_pickups[pickups_hud[i]->type]);
		cpVect p1=cpvadd(pickups_start,cpv(0,pickups_size.y*(float)(pickups_hud.size()-i)));
		cpVect p2=cpvadd(p1,pickups_size);

		glBegin(GL_QUADS);
		glTexCoord2f(0,0);	glVertex2f(p1.x,p1.y);
		glTexCoord2f(1,0);	glVertex2f(p2.x,p1.y);
		glTexCoord2f(1,1);	glVertex2f(p2.x,p2.y);
		glTexCoord2f(0,1);	glVertex2f(p1.x,p2.y);
		glEnd();
	}

	char health[100];
	sprintf(health,"Health: %d\n",(int)player->health);
	render_text(font,cpv(-0.55,0.25),health,0.05);
	if(game_mode==1) {
		sprintf(health,"Zombies left: %d\n",zombies_alive);
		render_text(font,cpv(0.2,0.25),health,0.05);
	}

	glDisable(GL_ALPHA_TEST);

	if(game_won || !player->alive) {

		glEnable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);

		if(player->alive) {
			glColor4f(0,0,0,won_fadeout);
		}
		else {
			glColor4f(1,0,0,won_fadeout);
		}

		glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(100,10);
		glVertex2f(-100,10);
		glVertex2f(100,-10);
		glVertex2f(-100,-10);
		glEnd();
	}
	glDisable(GL_BLEND);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}

void init_render_screen(int w,int h) {
	glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glm::mat4 mat=glm::perspective<float>(45.0f, (float)w/(float)h, 0.01f, 500.0f);
    glLoadMatrixf(glm::value_ptr(mat));

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

bool init_render() {

	GLenum glew_err = glewInit();
	if(glew_err!=GLEW_OK) {
	  printf("Glew init failed :( %s\n", glewGetErrorString(glew_err));
	  return false;
	}

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0,0.0,0.0,1.0);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	matrix=glm::perspectiveFov<float>(45,window_width,window_height,0.1,100);

	if(shader_supported()) {
		shader_normal=shader_fragment_compile(shader_normal_src);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	float coord_data[]={
			0,0,0,
			1,0,0,
			0,1,0,
			1,1,0
	};
	vbo_texcoord_normal=vbo_create(4,coord_data);

	float s=zombie_size/2;
	float pos_zombie_data[]={
			s,-s,0,
			-s,-s,0,
			s,s,0,
			-s,s,0
	};
	vbo_pos_zombie=vbo_create(4,pos_zombie_data);

	init_render_screen(window_width,window_height);

	return true;
}

bool load_resources() {

	texture_ground=texture_create(32,32,3,NULL);

	texture_zombie[0]=texture_load("media/zombie1.png",false);
	texture_zombie[1]=texture_load("media/zombie2.png",false);
	texture_zombie_corpse[0]=texture_load("media/zombie_corpse1.png",false);
	texture_zombie_corpse[1]=texture_load("media/zombie_corpse2.png",false);

	texture_corpse[0]=texture_load("media/corpse1.png",false);
	texture_corpse[1]=texture_load("media/corpse2.png",false);
	texture_corpse[2]=texture_load("media/corpse3.png",false);
	texture_corpse[3]=texture_load("media/can.png",false);

	texture_gun[0]=texture_load("media/gun.png",false);
	texture_gun[1]=texture_load("media/shotgun.png",false);
	texture_gun[2]=texture_load("media/grenade.png",false);

	texture_bullet[0]=texture_load("media/bullet.png",false);
	texture_bullet[1]=texture_load("media/shell.png",false);
	texture_bullet[2]=texture_gun[2];


	texture_flash=texture_load("media/explosion.png",false);
	texture_sign=texture_load("media/sign.png",false);
	texture_plank=texture_load("media/plank.png",false);
	texture_logo=texture_load("media/logo.png",false);

	texture_door[0]=texture_load("media/door1.png",false);
	texture_legend=texture_load("media/legend.png",false);

	texture_wall[0]=texture_load("media/wall1.png",true);

	//texture_can=texture_load("media/can.png",false);

	texture_wall[1]=texture_wall[0];
	texture_station=texture_load("media/station.png",false);
	texture_door[1]=texture_door[0];

	//level 2
	texture_tree[0]=texture_load("media/tree1.png",false);
	texture_tree[1]=texture_load("media/tree2.png",false);
	texture_tree[2]=texture_load("media/grass.png",false);

	//for(int i=0;i<3;i++) texture_pickups[i]=texture_zombie[0];
	char* pickups_textures[]={
			(char*)"media/burger.png",
			(char*)"media/hotdog.png",
			(char*)"media/meat.png",
			(char*)"media/gun_side.png",
			(char*)"media/shotgun_side.png"
	};
	for(int i=0;i<5;i++) {
		texture_pickups[i]=texture_load(pickups_textures[i],false);
	}
	texture_pickups[5]=texture_gun[2];

	font=font_load((char*)"media/arial.png",(char*)"media/arial.txt");

	return true;
}

void addWall(cpVect start,cpVect end,float z_start,float z_end,
		float repeat_x,float repeat_y,int texture,float color,bool add_shape=false) {
	Wall* w=new Wall();
	w->start=start;
	w->end=end;
	w->z_start=z_start;
	w->z_end=z_end;
	w->repeat_x=repeat_x;
	w->repeat_y=repeat_y;
	w->texture=texture;
	w->color=color;

	w->inserted=false;
	if(add_shape) {
		world->addStaticLine(start,end);
//		cpSegmentShapeNew(&space->staticBody,start,end,0.1);
	}
	else {
		w->shape=NULL;
	}

	w->vbo_inited=false;

	//walls.push_back(w);
	octree->add(cpvlerp(w->start,w->end,0.5),w);
}
void addCeil(cpVect start,cpVect end,float z,float repeat_x,float repeat_y,int texture) {
	Ceil* c=new Ceil();
	c->start=start;
	c->end=end;
	c->z=z;
	c->repeat_x=repeat_x;
	c->repeat_y=repeat_y;
	c->texture=texture;
	octree->add(cpvlerp(c->start,c->end,0.5),c);
	ceils.push_back(c);
}

void addDoor(cpVect start,cpVect end,float z_start,float z_end,int texture) {
	Door* w=new Door();
	w->start=start;
	w->end=end;
	w->z_start=z_start;
	w->z_end=z_end;
	w->repeat_x=1;
	w->repeat_y=1;
	w->texture=texture;
	w->color=1;
	w->size=cpvdist(start,end);
	w->locked=false;
	w->lock_health=30;

	w->body=cpBodyNew(10000,10);
	w->body->p=start;

	cpVect dist=cpvsub(end,start);
	w->body->a=atan2(dist.y,dist.x);

	w->closed_angle=w->body->a;

	w->shape=cpSegmentShapeNew(w->body,cpvzero,cpv(w->size,0),0.1);
	w->shape->collision_type=2;

	w->constraint=cpPivotJointNew2(world->space->staticBody,
			w->body,w->body->p,cpvzero);

//	w->constraint2=cpRotaryLimitJointNew(&world->space->staticBody,
//			w->body,w->body->a,175/57.325+w->body->a);
	w->constraint2=cpRotaryLimitJointNew(world->space->staticBody,
			w->body,w->body->a -90/57.325,w->body->a + 0/57.325);

	/*
	cpSpaceAddBody(world->space,w->body);
	cpSpaceAddShape(world->space,w->shape);
	cpSpaceAddConstraint(world->space,w->constraint);
	cpSpaceAddConstraint(world->space,w->constraint2);
	*/

	w->inserted=false;

	w->body->data=w;

	octree->add(w->body->p,w);
	//doors.push_back(w);
}
void addGrenade() {
	float grenade_size=0.2;

	cpVect rot=cpvforangle((player->angle_x+90)/57.325);

	Grenade* g=new Grenade();
	g->body=cpBodyNew(0.5,INFINITY);
	g->shape=cpCircleShapeNew(g->body,grenade_size,cpvzero);
	g->body->p=player->body->p; //+player->body->rot;
	g->body->v=cpvmult(rot,0.012)+player->body->v;
	g->z=player_height;
	g->z_vel=0;
	g->time=2;

	cpSpaceAddBody(world->space,g->body);
	cpSpaceAddShape(world->space,g->shape);

	grenades.push_back(g);
}

/*
int pickup_touched(cpArbiter *arb,struct cpSpace *space, void *data) {
	printf("pickup\n");
	return 0;
}
*/
int zombie_door_touched(cpArbiter* arb,struct cpSpace* space,void* data) {
	Door* d=(Door*)arb->b_private->body->data;
	Human* h=(Human*)arb->a_private->body->data;

	if(!h->alive) return 1;

	if(d->locked) {
		if(h->attack_cooloff<0) {
			damage_door(d,zombie_damage);
			h->attack_cooloff=1;
		}
	}
	return 1;
}

void addPickup(cpVect pos,int type) {
	Pickup* p=new Pickup();
	p->body=cpBodyNew(1,1);
	p->type=type;

	p->body->p=pos;

//	pickups.push_back(p);
	octree->add(pos,p);
}

void vect_get_space(cpVect p1,cpVect p2,float size,cpVect* b1,cpVect* b2) {
	cpVect c=cpvmult(cpvadd(p1,p2),0.5);
	float size2=size/2;
	*b1=cpvlerpconst(c,p1,size2);
	*b2=cpvlerpconst(c,p2,size2);
}

void generate_building(cpVect center, cpVect size,float angle) {
	cpVect corners[4];
	cpVect size2=cpvmult(size,0.5);

	float height=randInt(5,15)*2 + 4.5*4;

	corners[0]=cpvadd(center,cpv(size2.x, size2.y));
	corners[1]=cpvadd(center,cpv(size2.x,-size2.y));
	corners[2]=cpvadd(center,cpv(-size2.x,-size2.y));
	corners[3]=cpvadd(center,cpv(-size2.x,size2.y));

	cpVect sign_p1,sign_p2;
	float sign_size=2;
	bool sign_ok=false;

	int total_doors=0;

	for(int i=0;i<4;i++) {
		cpVect p1=corners[i];
		cpVect p2=corners[(i+1)%4];

		float wall_len=cpvdist(p1,p2);

		int num_doors=randInt(0,wall_len/25*4); //cpvdist(p1,p2);
		total_doors+=num_doors;

		if(i==3 && num_doors==0) {
			vect_get_space(p1,p2,sign_size,&sign_p1,&sign_p2);
			sign_ok=true;
		}
		if(i==2 && total_doors==0) {
			num_doors=1;
		}

		for(int d=0;d<num_doors;d++) {

			float door_width_norm=2.0/wall_len;
			float min_pos=((float)d)/(float)num_doors;
			float max_pos=(((float)d+1)/(float)num_doors - door_width_norm ) *0.8;
			float pos=randFloat(min_pos,max_pos);

			cpVect pt1=cpvlerp(p1,p2,pos);
			//cpVect pt2=cpvlerp(p1,p2,pos+door_width_norm);
			cpVect pt2=cpvlerpconst(pt1,p2,2);


			addWall(p1,pt1,0,3,0.01,0.01,texture_wall[0],1,true);
			addDoor(pt1,pt2,0,3,texture_door[0]);

			if(i==3 && !sign_ok) {
				if(p1.y-pt1.y>sign_size) {
					vect_get_space(p1,pt1,sign_size,&sign_p1,&sign_p2);
					sign_ok=true;
				}
			}

			p1=pt2;
		}

		addWall(p1,p2,0,3,0.01,0.01,texture_wall[0],1,true);
	}

	addWall(corners[0],corners[1],3,4.5,0.01,0.01,texture_wall[0],1);
	addWall(corners[1],corners[2],3,4.5,0.01,0.01,texture_wall[0],1);
	addWall(corners[2],corners[3],3,4.5,0.01,0.01,texture_wall[0],1);
	addWall(corners[3],corners[0],3,4.5,0.01,0.01,texture_wall[0],1);

	float ry=height/2; //(height-4.5)/2;

	addWall(corners[0],corners[1],4.5,height,size.y/2,ry,texture_wall[0],1);
	addWall(corners[1],corners[2],4.5,height,size.x/2,ry,texture_wall[0],1);
	addWall(corners[2],corners[3],4.5,height,size.y/2,ry,texture_wall[0],1);
	addWall(corners[3],corners[0],4.5,height,size.x/2,ry,texture_wall[0],1);

	if(sign_ok) {
		sign_p1=cpvadd(sign_p1,cpv(0,-0.1));
		sign_p2=cpvadd(sign_p2,cpv(0,-0.1));
		addWall(sign_p2,sign_p1,1,3,1,1,texture_map,1);
		addWall(sign_p1,sign_p2,0.5,1,1,1,texture_legend,1);
	}

	//addCeil(corners[2],corners[0],0.05,size.x,size.y,texture_wall[0]);
	addCeil(corners[2],corners[0],4.5,0.01,0.01,texture_wall[0]);
	//printf("ceil %f %f %f %f\n",corners[0].x,corners[0].y,corners[2].x,corners[2].y);

	int num_pickups=size.x*size.y/30;
	for(int i=0;i<num_pickups;i++) {
		addPickup(randVect(corners[0],corners[2]),rand()%6);
	}
}
void generate_station(cpVect start, cpVect end) {

	cpVect corners[4];
	float height=7;

	corners[0]=cpv(start.x,start.y);
	corners[1]=cpv(start.x,end.y);
	corners[2]=cpv(end.x,end.y);
	corners[3]=cpv(end.x,start.y);

	cpVect normals[4]={
			cpv(-0.1,0),
			cpv(0,0.1),
			cpv(0.1,0),
			cpv(0,-0.1)
	};

	for(int i=0;i<4;i++) {
		cpVect p1=corners[i];
		cpVect p2=corners[(i+1)%4];

		cpVect sign_p1=cpvadd(cpvlerp(p1,p2,0.7),normals[i]);
		cpVect sign_p2=cpvadd(cpvlerp(p1,p2,0.3),normals[i]);
		addWall(sign_p1,sign_p2,3,5,1,1,texture_station,1);

		float wall_len=cpvdist(p1,p2);

		int num_doors=1; // i==0 || i==2 ? 1 : 0;

		for(int d=0;d<num_doors;d++) {

			float door_width_norm=2/wall_len;

			/*
			float min_pos=((float)d)/(float)num_doors;
			float max_pos=(((float)d+1)/(float)num_doors - door_width_norm ) *0.8;
			float pos=(min_pos+max_pos)/2;

			cpVect pt1=cpvlerp(p1,p2,pos);
			cpVect pt2=cpvlerp(p1,p2,pos+door_width_norm);
			*/

			cpVect wall_c=cpvlerp(p1,p2,0.5);
			cpVect pt1=cpvlerpconst(wall_c,p1,1);
			cpVect pt2=cpvlerpconst(wall_c,p2,1);

//			world->addStaticLine(p1,pt1);
			addWall(p1,pt1,0,3,10,10,texture_wall[1],1,true);
			addDoor(pt1,pt2,0,3,texture_door[1]);

			p1=pt2;

			//door
		}

//		world->addStaticLine(p1,p2);
		addWall(p1,p2,0,3,10,10,texture_wall[1],1,true);
	}

	addWall(corners[0],corners[1],3,height,10,10,texture_wall[1],1);
	addWall(corners[1],corners[2],3,height,10,10,texture_wall[1],1);
	addWall(corners[2],corners[3],3,height,10,10,texture_wall[1],1);
	addWall(corners[3],corners[0],3,height,10,10,texture_wall[1],1);

	addCeil(corners[0],corners[2],3,0.01,0.01,texture_wall[1]);
}

void generate_city() {

	/*
	for(int i=0;i<10;i++) {
		cpVect c=randVect(world_p1,world_p2);
		cpVect size=randIntVect(cpv(5,15),cpv(5,30));
		float angle=0; //randFloat(0,360);
		//generate_building(c,size,angle);
	}
	*/

	unsigned char* ground_image=image_random(32,32,3);
	texture_update(texture_ground,32,32,3,ground_image);
	delete[] ground_image;

	texture_map=texture_create(map_size,map_size,3,NULL);

	game_won=false;
	current_weapon=0;
	flash_fadeout=0;
	level=0;

	float field_size=50;
	int grid_size=world_size/field_size;

	int station_x=randInt(0,grid_size);
	int station_y=randInt(0,grid_size); //randInt(0,world_size/field_size);

	printf("world extents %f %f %f %f\n",world_p1.x,world_p1.y,world_p2.x,world_p2.y);

	int building_num=0;
	for(int x=0;x<grid_size;x++) {
		for(int y=0;y<grid_size;y++) {
			if(x==station_x && y==station_y) continue;

			cpVect size=randIntVect(cpv(5,5),cpv(field_size/4,field_size/4));

			size=cpvmult(size,2);

			cpVect loc_pos=randVect(cpv(field_size*0.1,field_size*0.1),
					cpvsub(cpv(field_size*0.9,field_size*0.9),size));

			//cpVect center=cpvadd(loc_pos,cpv(x*field_size,y*field_size));

			float fx=x;
			float fy=y;
			float fgrid_size=grid_size;
			cpVect center=cpv((2*fx+1)/(fgrid_size*2),(2*fy+1)/(fgrid_size*2));
			center=cpvmult(center,world_size);
			center=cpvsub(center,cpv(world_size/2,world_size/2));

			center=cpvadd(center,loc_pos);

			cpvsub(center,cpv(world_size,world_size));

			generate_building(center,size,0);

//			printf("building center %f %f\n",center.x,center.y);

			building_num++;
		}
	}
	printf("Generated %d buildings\n",building_num);

	station_start=cpv((float)station_x*field_size/2,(float)station_y*field_size/2);
	station_end=cpvadd(station_start,cpv(10,30));
	generate_station(station_start,station_end);

	player->body->p=cpvzero;
	player->angle_x=rand()%360;

	/*
	addCeil(cpvlerp(world_p1,world_p2,0.25),
			cpvlerp(world_p1,world_p2,0.75),5,10,10,texture_ground);
	*/

	texture_map_data=new unsigned char[map_size*map_size*3];
	texture_map_data2=new unsigned char[map_size*map_size*3];
	int texture_i=0;
	for(int x=0;x<map_size;x++) {
		for(int y=0;y<map_size;y++) {

			cpVect pos1=cpv(world_size*((float)(x-map_size/2))/(float)map_size,
					world_size*((float)(y-map_size/2))/(float)map_size);
			cpVect pos2=cpv(world_size*((float)(x-map_size/2+1))/(float)map_size,
					world_size*((float)(y-map_size/2+1))/(float)map_size);

			//bool quad=quadInBuilding(pos1,pos2);

			unsigned char c=pointInBuilding(pos1) ? rand()%40 : (rand()%40)+215;
			//unsigned char c=quad ? 0 : 255;
//			printf("pos %f %f inside %d\n",pos.x,pos.y,pointInBuilding(pos));
			texture_map_data[texture_i+0]=c;
			texture_map_data[texture_i+1]=c;
			texture_map_data[texture_i+2]=c;
			texture_i+=3;
		}
	}
	texture_update(texture_map,map_size,map_size,3,texture_map_data);

	render_range_zombies=300;
	render_range_walls=500;
	render_range_pickups=50;
}

void addTree(cpVect pos) {

	float h=7;
	float s=2;

	cpVect p1=cpvadd(pos,cpv(s,s));
	cpVect p2=cpvadd(pos,cpv(-s,-s));

	cpVect p3=cpvadd(pos,cpv(s,-s));
	cpVect p4=cpvadd(pos,cpv(-s,s));

	int t=rand()%2;
	addWall(p1,p2,0,h,1,1,texture_tree[t],1);
	addWall(p3,p4,0,h,1,1,texture_tree[t],1);

	if(rand()%10==1) {
		cpVect station_p=cpvlerp(station_start,station_end,0.5);
		cpVect sign_facing=cpvnormalize(cpvsub(pos,station_p));
		cpVect sign_pos=cpvsub(pos,cpvperp(sign_facing));

		cpVect sign_p1=cpvsub(sign_pos,sign_facing);
		cpVect sign_p2=cpvadd(sign_pos,sign_facing);

		addWall(sign_p1,sign_p2,1,1.5,1,1,texture_sign,1,false);
	}

	world->addStaticCircle(pos,s/3);
}

void generate_forrest() {

	unsigned char* ground_image=image_random_green(32,32,3);
	texture_update(texture_ground,32,32,3,ground_image);
	delete[] ground_image;

	station_start=cpv(100,100);
	station_end=cpv(140,140);

	level=1;

	//trees
	float field_size=10;
	int grid_size=world_size/field_size;

	int station_x=randInt(0,grid_size);
	int station_y=randInt(0,grid_size);

	printf("world extents %f %f %f %f\n",world_p1.x,world_p1.y,world_p2.x,world_p2.y);

	cpVect world_neg_offset=cpv(-world_size/2,-world_size/2);

	cpVect tree_clear_p1=cpvlerpconst(station_start,station_end,-2);
	cpVect tree_clear_p2=cpvlerpconst(station_end,station_start,-2);

	int tree_num=0;
	for(int x=0;x<grid_size;x++) {
		for(int y=0;y<grid_size;y++) {
			if(x==station_x && y==station_y) continue;

			cpVect pos=randVect(cpvzero,cpv(field_size,field_size));
			pos=cpvadd(pos,cpv((float)x*field_size,(float)y*field_size));
			pos=cpvadd(pos,world_neg_offset);

			if(pointInside(tree_clear_p1,tree_clear_p2,pos)) continue;

			addTree(pos);

			//printf("tree at %f %f\n",pos.x,pos.y);

			tree_num++;
		}
	}

	for(int i=0;i<1000;i++) {
		addPickup(randVect(world_p1,world_p2),rand()%3);
	}
	for(int i=0;i<1000;i++) {
		cpVect pos=randVect(world_p1,world_p2);

		float s=randFloat(0.2,0.5);

		cpVect p1=cpvadd(pos,cpv(s,s));
		cpVect p2=cpvadd(pos,cpv(-s,-s));
		cpVect p3=cpvadd(pos,cpv(-s,s));
		cpVect p4=cpvadd(pos,cpv(s,-s));
		s*=2;
		addWall(p1,p2,0,s,1,1,texture_tree[2],1);
		addWall(p3,p4,0,s,1,1,texture_tree[2],1);
	}

	generate_station(station_start,station_end);

	printf("added %d trees\n",tree_num);

	player->body->p=cpvzero; //cpvlerp(station_start,station_end,1.5);

	render_range_zombies=200;
	render_range_walls=200;
	render_range_pickups=50;
}

void clean_octree(Octree* tree) {
	for(int i=0;i<tree->ceils.size();i++) {
		delete(tree->ceils[i]);
	}
	ceils.clear();

	for(int i=0;i<tree->doors.size();i++) {
		Door* d=tree->doors[i];
		if(d->inserted) {
			cpSpaceRemoveConstraint(world->space,d->constraint);
			cpSpaceRemoveConstraint(world->space,d->constraint2);
			cpSpaceRemoveShape(world->space,d->shape);
			cpSpaceRemoveBody(world->space,d->body);
		}
		cpConstraintDestroy(d->constraint);
		cpConstraintDestroy(d->constraint2);
		cpBodyDestroy(d->body);
		delete(d);
	}
	for(int i=0;i<tree->humans.size();i++) {
		Human* h=tree->humans[i];
		if(h->shape_inserted) {
			cpSpaceRemoveShape(world->space,h->shape);
			cpSpaceRemoveBody(world->space,h->body);
		}
		cpShapeDestroy(h->shape);
		cpBodyDestroy(h->body);
		delete(h);
	}
	for(int i=0;i<tree->pickups.size();i++) {
		delete(tree->pickups[i]);
	}
	for(int i=0;i<tree->walls.size();i++) {
		Wall* w=tree->walls[i];
		if(w->vbo_inited) {
			vbo_delete(w->vbo);
			vbo_delete(w->vbo_coord);
		}
		if(w->shape) {
			cpSpaceRemoveShape(world->space,w->shape);
			cpShapeDestroy(w->shape);
		}
		delete(w);
	}
	for(int i=0;i<4;i++) {
		if(tree->children[i]) clean_octree(tree->children[i]);
	}
}


void clean_level() {
	if(octree) {
		clean_octree(octree);
		delete(octree);
		octree=NULL;
	}
	if(world) {
		delete(world);
	}
}
void init_level(int l) {
	clean_level();

	printf("cleanup\n");

	switch_level=false;
	game_won=false;
	won_fadeout=0;


	world=new World(cpv(world_size,world_size));

	printf("world created\n");
	fflush(stdout);

	float w2=world_size/2;
	world_p1=cpv(-w2,-w2);
	world_p2=cpv(w2,w2);

	octreeInit();

	printf("octree\n");
	fflush(stdout);

	player=world->addHuman(0.8);
	human_check_insert_shape(player);
	shake_anim=0;

	printf("generating\n");
	fflush(stdout);

	level=l;
	if(level==0) generate_city();
	else if(level==1) generate_forrest();

	player->shape->collision_type=1;

	float world_area=world_size*world_size;

	int num_zombies=world_area/1000;
	int num_corpses=world_area/1000;

	printf("level inited\n");
	fflush(stdout);

	for(int i=0;i<num_zombies;i++) {
		Human* h=world->addHuman(0.8);
		h->body->p=randVect(world_p1,world_p2);
		h->speed=0.006;
		int t=rand()%2;
		h->texture_alive=texture_zombie[t];
		h->texture_dead=texture_zombie_corpse[t];
		h->shape->collision_type=3;
		octree->add(h->body->p,h);
		//zombies.push_back(h);

		int pick_num=randInt(0,3);
		for(int i=0;i<pick_num;i++) {
			h->pickups.push_back((rand()%3)+3);
		}
	}
	zombies_alive=num_zombies;

	for(int i=0;i<num_corpses;i++) {
		Human* h=world->addHuman(0.8);
		h->body->p=randVect(world_p1,world_p2);
		h->alive=false;
		h->texture_dead=texture_corpse[rand()%4];
		h->shape->collision_type=4;
		octree->add(h->body->p,h);
		//corpses.push_back(h);

		int pick_num=randInt(1,5);
		for(int i=0;i<pick_num;i++) {
			h->pickups.push_back(rand()%6);
		}
	}

	printf("zombies inited\n");
	fflush(stdout);

	cpSpaceAddCollisionHandler(
		world->space,
		2,3,
		zombie_door_touched,
		NULL,NULL,NULL,
		NULL);

	player_height=2;

	if(game_mode==0) {
		weapon_ammo[0]=12;
		weapon_ammo[1]=0;
		weapon_ammo[2]=0;
	}
	else {
		weapon_ammo[0]=weapon_max_ammo[0];
		weapon_ammo[1]=weapon_max_ammo[1];
		weapon_ammo[2]=weapon_max_ammo[2];
	}

	menu_on=true;
	menu_screen=3;
	menu_interact=false;
	menu_anim=0;
}

void update_menu(long ts) {
	float fts=(float)ts/1000;

	if(gtdc_gui::input_menu_enter) {
		menu_interact=true;
	}

	switch(menu_screen) {
	case 0:
		if(gtdc_gui::input_menu_enter) {
			printf("enter, selection %d\n",main_menu_selection);
			if(main_menu_selection<2) {
				printf("loading level\n");
				game_mode=main_menu_selection;
				init_level(0);
			}
			else if(main_menu_selection==2) {
				menu_screen=1;
			}
			else {
				game_run=false;
			}
		}
		if(gtdc_gui::input_menu_up) {
			main_menu_selection=max(0,main_menu_selection-1);
		}
		if(gtdc_gui::input_menu_down) {
			main_menu_selection=min(3,main_menu_selection+1);
		}
		break;
	case 1:	//credits

		if(gtdc_gui::input_menu_enter || gtdc_gui::input_esc) {
			menu_screen=0;
		}
		break;
	case 2:
		if(gtdc_gui::input_esc && player->alive ||
				(gtdc_gui::input_menu_enter && main_menu_selection==0)) {
			menu_on=false;
		}
		if(gtdc_gui::input_menu_up) main_menu_selection=max(
				(player->alive ? 0 : 1),
				main_menu_selection-1);
		if(gtdc_gui::input_menu_down) main_menu_selection=min(2,main_menu_selection+1);
		if(gtdc_gui::input_menu_enter) {
			if(main_menu_selection==1) {
				init_level(level);
			}
			else {
				menu_screen=0;
			}
			main_menu_selection=0;
		}
		break;
	case 3:
		if(menu_interact) {
			menu_anim+=fts;
			if(menu_anim>1) {
				if(level==2) {
					menu_screen=0;
				}
				else {
					menu_on=false;
				}
			}
		}
		break;
	}
}

void render_text_center(Font* f,cpVect pos,char* text,float size) {
	cpVect bounds=text_bounds(f,text,size);
	render_text(f,pos-bounds*0.5,text,size);
}
void render_menu_selection(float size,float y,int selection) {
	cpVect ss=cpv(size,size);
	cpVect sc=cpv(-0.44,y-size*(selection-1));
	render_texture_quad(sc-ss*0.5,sc+ss*0.5,texture_pickups[3]);

}

void render_menu() {

	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	glLoadIdentity();
	glTranslatef(0,0,-1);

	cpVect screen_size2=cpv(0.5,0.4);
	cpVect screen_p1=cpvmult(screen_size2,-1);
	cpVect screen_p2=screen_size2;

	glColor3f(1,1,1);

	switch(menu_screen) {
	case 0: {	//main

		float by=-0.15;
		float bh=0.08;

		render_texture_quad(cpv(-0.5,0),cpv(0.5,0.4),texture_logo);
		render_text_center(font,cpv(0,by-bh*0),(char*)"Story",bh);
		render_text_center(font,cpv(0,by-bh*1),(char*)"Zombie hunt",bh);
		render_text_center(font,cpv(0,by-bh*2),(char*)"Credits",bh);
		render_text_center(font,cpv(0,by-bh*3),(char*)"Exit",bh);

		render_menu_selection(bh,by,main_menu_selection);
		break;
	}
	case 1: {

		render_text_center(font,cpvzero,credits,0.1);

		break;
	}
	case 2: {	//pause/death

		float bh=0.1;
		float by=0.1;

		if(player->alive)
			render_text_center(font,cpv(0,by-bh*0),(char*)"Resume",bh);
		render_text_center(font,cpv(0,by-bh*1),(char*)"Restart",bh);
		render_text_center(font,cpv(0,by-bh*2),(char*)"Main menu",bh);

		render_menu_selection(bh,by,main_menu_selection);
		break;
	}
	case 3:	{	//story
		float size=0.05;

		char* text;

		if(game_mode==0) {
			if(level==0) text=story_1;
			else if(level==1) text=story_2;
			else text=story_end;
		}
		else {
			if(level==0) text=story_hunt1;
			else if(level==1) text=story_hunt2;
			else text=story_hunt_end;
		}

		cpVect bounds=text_bounds(font,text,size);
		float c=1-menu_anim;
		glColor3f(c,c,c);
		render_text(font,cpvmult(bounds,-0.5),text,size);
		break;
	}
	}
}

void init_main_menu() {

	menu_on=true;
	menu_screen=0;
	menu_interact=false;
	menu_anim=0;
	main_menu_selection=0;
}

int main() {

	printf("Welcome to ZAC!\n");

	srand(time(0));

	ilInit();
	iluInit();
	ilutRenderer(ILUT_OPENGL);

	init_audio();

	if(!gtdc_gui::open(window_width,window_height,false)) {
		return 0;
	}
	if(!init_render()) {
		return 0;
	}
	load_resources();

	cpInitChipmunk();

	difficulty=0;

	init_main_menu();

	SDL_ShowCursor(0);

	//DA LOOP

	long last_ts=get_ticks();
	long delta=0;

	long runtime=0;

	long measure_ts=0;
	int measure_fps=0;

	game_run=true;
	for(long i=0;game_run;i++) {

		gtdc_gui::wrap_mouse=!menu_on;
		gtdc_gui::check_events();
		if(gtdc_gui::input_quit) {
			break;
		}
		if(gtdc_gui::resized) {
			init_render_screen(gtdc_gui::width,gtdc_gui::height);
		}

		if(menu_on) {
			update_menu(delta);
			render_menu();
		}
		else {
			update(delta);
			render();
		}

		if(switch_level) {
			init_level(level+1);
		}

		gtdc_gui::swap();

		long new_ts=get_ticks();
		delta=new_ts-last_ts;
		last_ts=new_ts;

		runtime+=delta;

		measure_fps++;
		if(runtime-measure_ts>1000) {
			printf("%d fps\n",measure_fps);
			measure_fps=0;
			measure_ts=runtime;
		}

//		sleep_ms(20);
	}

	return 0;
}




