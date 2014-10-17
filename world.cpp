#include "world.h"

#include <stdio.h>

float human_size=0.5;

World::World(cpVect size) {
	space=cpSpaceNew();

	static_body=cpBodyNewStatic();

	printf("new space\n"); fflush(stdout);

	cpVect s2=cpvmult(size,0.5);

	cpVect c1=s2;
	cpVect c2=cpv(s2.x,-s2.y);
	cpVect c3=cpv(-s2.x,-s2.y);
	cpVect c4=cpv(-s2.x,s2.y);

	printf("foo space\n"); fflush(stdout);

	cpBodyInitStatic(space->staticBody);

	addStaticLine(c1,c2);
	addStaticLine(c2,c3);
	addStaticLine(c3,c4);
	addStaticLine(c4,c1);

	printf("space done\n"); fflush(stdout);
}
World::~World() {
	cpSpaceDestroy(space);
}

void World::update(long dt) {
	cpSpaceStep(space,dt);
}

Human* World::addHuman(float size) {
	Human* h=new Human();

	cpShape* shape=cpCircleShapeNew(h->body,size,cpvzero);

	shape->e=0.5;
	shape->u=0.2;

	h->shape=shape;

	humans.push_back(h);
	return h;
}
void World::removeHuman(Human* h) {
	cpSpaceRemoveShape(space,h->shape);
	cpSpaceRemoveBody(space,h->body);

	for(int i=0;i<humans.size();i++) {
		if(h==humans[i]) {
			humans.erase(humans.begin()+i);
			break;
		}
	}
}

void World::addStaticLine(cpVect start,cpVect end) {
	cpShape* s=cpSegmentShapeNew(static_body,start,end,0.1);
	cpSpaceAddStaticShape(space,s);
}
void World::addStaticCircle(cpVect center,float r) {
	cpShape* s=cpCircleShapeNew(space->staticBody,r,center);
	cpSpaceAddShape(space,s);
}


