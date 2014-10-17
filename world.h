#ifndef _WORLD_H_
#define _WORLD_H_

#include "human.h"

#include "chipmunk.h"

#include <vector>

using namespace std;

class World {

public:
	cpSpace* space;
	vector<Human*> humans;
	cpBody* static_body;

	World(cpVect size);
	~World();
	void update(long ts);
	Human* addHuman(float size=0.5);
	void removeHuman(Human* h);
	void addStaticLine(cpVect start,cpVect end);
	void addStaticCircle(cpVect center,float r);

};

#endif
