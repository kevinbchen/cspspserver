#include "Grenade.h"

//------------------------------------------------------------------------------------------------
Grenade::Grenade(float x, float y, float px, float py, float angle, float speed, Person *parent, int type) : Bullet(x,y,px,py,angle,speed,0,parent)
{
	mType = TYPE_GRENADE;
	mGrenadeType = type;
	mTimer = 1500.0f;
}

//------------------------------------------------------------------------------------------------
Grenade::~Grenade()
{
}


//------------------------------------------------------------------------------------------------
void Grenade::Update(float dt)
{
	Bullet::Update(dt);
	mTimer -= dt;
	if (mTimer < 0.0f) {
		dead = true;
	}
}

