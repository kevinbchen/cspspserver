#include "Gun.h"
#include "String.h"

//------------------------------------------------------------------------------------------------
Gun::Gun(int id, int delay, int damage, float spread, int clip, int numclips, int reloaddelay, float speed, int cost, int type, char* name)
{
	mId = id;
	mDelay = delay;
	mDamage = damage;
	mSpread = spread;
	mClip = clip;
	mNumClips = numclips;
	mReloadDelay = reloaddelay;
	mSpeed = speed;
	mCost = cost;
	mType = type;
	strcpy(mName,name);

}

//------------------------------------------------------------------------------------------------
Gun::~Gun()
{

}
