#pragma once

#include <math.h>
#include "GunObject.h"

#define TYPE_BULLET 0
#define TYPE_GRENADE 1
//------------------------------------------------------------------------------------------------
class Person;

class Bullet
{
private:
	bool mIsFirstUpdate;

protected:

public:
	float cosAngle;
	float sinAngle;

	float mX;
	float mY;
	float pX;
	float pY;
	float mAngle;
	float mSpeed;
	int mDamage;
	bool dead;
	Person* mParent;
	Gun* mParentGun;
	int mId;
	int mType;

	Bullet(float x, float y, float px, float py, float angle, float speed, int damage, Person *parent);
	~Bullet();

	virtual void Update(float dt);
	void SetAngle(float angle);

	void AddLatency(float latency);

};