#include "human.h"

#include <stdio.h>

#include "audio.hpp"

float human_mass=100;

Human::Human() {
	angle_x=angle_y=0;
	body=cpBodyNew(human_mass,100);

	body->data=this;

	speed=0.01;
	heading=cpvzero;
	alive=true;

	health=100;
	attack_cooloff=0;
	player_visible=false;
	shape_inserted=false;
	interest_level=0;
	player_distance=10000;
}
Human::~Human() {
	cpBodyDestroy(body);
}

void Human::hit(float damage) {
	health-=damage;
	if(health<0) {
		die();
		health=0;
	}
}
void Human::die() {
	alive=false;
	speed=0;
	shape->collision_type=4;
}

void Human::goForward(float x) {
	cpVect vec=cpvforangle((angle_x+90)/57.325);
	heading=cpvadd(heading,cpvmult(vec,x));
}
void Human::goSide(float x) {
	cpVect vec=cpvforangle((angle_x)/57.325);
	heading=cpvadd(heading,cpvmult(vec,x));
}
