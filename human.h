#ifndef _HUMAN_H_
#define _HUMAN_H_

#include "chipmunk.h"
#include <AL/al.h>

#include <vector>

using namespace std;

class Human {
public:

	cpBody* body;
	cpShape* shape;

	float angle_x;
	float angle_y;
	float speed;
	bool alive;
	int texture_alive;
	int texture_dead;

	float player_distance;
	bool player_visible;

	float health;
	float attack_cooloff;

	cpVect heading;

	cpVect interest_point;
	float interest_level;	//0-1, affects walking speed

	bool shape_inserted;

	/*
	ALuint source_wander;
	ALuint source_pain;
	ALuint source_death;
	*/

	vector<int> pickups;

	Human();
	~Human();	//remove body from space before calling

	//x=[-1,1]
	void goForward(float x);
	void goSide(float x);
	void die();
	void hit(float damage);
};

#endif
