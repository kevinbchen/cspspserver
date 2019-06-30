#include "GunObject.h"

//------------------------------------------------------------------------------------------------
GunObject::GunObject(Gun *gun, int clipammo, int remainingammo)
{
	mId = -1;
	mX = 0;
	mY = 0;
	mOldX = 0;
	mOldY = 0;
	mSpeed = 0;
	mAngle = 0;
	mGun = gun;
	mClipAmmo = clipammo;
	mRemainingAmmo = remainingammo;
	mOnGround = true;
}

//------------------------------------------------------------------------------------------------
GunObject::~GunObject()
{
}

//------------------------------------------------------------------------------------------------
void GunObject::Update(float dt) 
{ 
	if (!mOnGround) {
		mOldX = mX;
		mOldY = mY;

		float speed = mSpeed-0.001f*dt;
		if (speed < 0) {
			speed = 0.0f;
			mOnGround = true;
		}
		mSpeed = speed;

		mX += mSpeed*cosf(mAngle)*dt;
		mY += mSpeed*sinf(mAngle)*dt;		
	}
}

//------------------------------------------------------------------------------------------------
void GunObject::SetPosition(float x, float y) 
{ 
	mX = x;
	mY = y;
	mOldX = x;
	mOldY = y;
}

//------------------------------------------------------------------------------------------------
/**void GunObject::Reset()
{
	if (mSpawned) {
		mClipAmmo = mGun->mClip;
		mRemainingAmmo = mGun->mClip*(mGun->mNumClips-1);
		mX = oldx;
		mY = oldy;
	}
	else {
		delete this;
	}
}**/
