#pragma once

#define PRIMARY 0
#define SECONDARY 1
#define KNIFE 2

//------------------------------------------------------------------------------------------------

class Gun
{
private:
protected:
public:
	int mId;
	int mDelay;
	int mDamage;
	float mSpread;
	int mClip;
	int mNumClips;
	int mReloadDelay;
	float mSpeed;
	int mCost;
	int mType;
	char mName[15];

	Gun(int id, int delay, int damage, float spread, int clip, int numclips, int reloaddelay, float speed, int cost, int type, char* name);
	~Gun();

};
