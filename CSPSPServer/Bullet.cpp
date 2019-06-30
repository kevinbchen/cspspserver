#include "Bullet.h"
#include "Person.h"

//------------------------------------------------------------------------------------------------
Bullet::Bullet(float x, float y, float px, float py, float angle, float speed, int damage, Person *parent)
{
	dead = false;
	mX = x;
	mY = y;
	pX = px;
	pY = py;
	mAngle = angle;
	mSpeed = speed;
	mDamage = damage;
	mParent = parent;
	mParentGun = parent->mGuns[parent->mGunIndex]->mGun;
	mId = -1;
	mIsFirstUpdate = true;
	mType = TYPE_BULLET;

	cosAngle = cosf(mAngle);
	sinAngle = sinf(mAngle);
}

//------------------------------------------------------------------------------------------------
Bullet::~Bullet()
{
}


//------------------------------------------------------------------------------------------------
void Bullet::Update(float dt)
{
	if (mIsFirstUpdate) {
		mIsFirstUpdate = false;
	}
	else {
		pX = mX;
		pY = mY;
	}
	//use a ray to prevent missing a collision? (nvm fixed)
	mX += cosf(mAngle)*mSpeed*dt;
	mY += sinf(mAngle)*mSpeed*dt;
}

//------------------------------------------------------------------------------------------------
void Bullet::SetAngle(float angle)
{
	mAngle = angle;
	cosAngle = cosf(mAngle);
	sinAngle = sinf(mAngle);
}

//------------------------------------------------------------------------------------------------
void Bullet::AddLatency(float latency)
{
	mX += mSpeed*cosAngle*latency;
	mY += mSpeed*sinAngle*latency;
}
