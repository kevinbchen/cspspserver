#pragma once

#include <vector>
#include <math.h>

struct Gun {
	int mId;
	int mDelay;
	int mDamage;
	float mSpread;
	int mClip;
	int mNumClips;
	int mReloadDelay;
	float mSpeed;
	float mBulletSpeed;
	float mViewAngle;
	int mCost;
	int mType;
	char mName[15];
};
//------------------------------------------------------------------------------------------------

class GunObject
{
private:

protected:

public:
	int mId;

	float mX;
	float mY;
	float mOldX;
	float mOldY;
	float mSpeed;
	float mAngle;

	Gun *mGun;
	int mClipAmmo;
	int mRemainingAmmo;
	bool mOnGround;
	//bool mSpawned;

	GunObject(Gun *gun, int clipammo, int remainingammo);
	~GunObject();

	void Update(float dt);
	//void Render(float x, float y);

	void SetTotalRotation(float theta);
	void SetPosition(float x, float y);
	//void Reset();
};