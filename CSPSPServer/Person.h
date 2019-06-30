#pragma once

#include <math.h>
#include "GunObject.h"
#include "Bullet.h"
#include "Grenade.h"
#include <iostream>
#include "UdpManager.h"

#define RELATIVE1 0
#define ABSOLUTE1 1

#define M_PI	3.14159265358979323846f
#define M_PI_2	1.57079632679489661923f
#define M_PI_4	0.785398163397448309616f
#define M_1_PI	0.318309886183790671538f
#define M_2_PI	0.636619772367581343076f

#define NONE -1
#define T 0
#define CT 1

#define MOVING 0
#define NOTMOVING 1
#define NORMAL 0
#define DEAD 1
#define ATTACKING 2
#define RELOADING 3
#define DRYFIRING 4
#define SWITCHING 5

#define PRIMARY 0
#define SECONDARY 1
#define KNIFE 2
#define GRENADE 3

struct Input {
	int x;
	int y;
	float facingangle;
};

struct State {
	float x;
	float y;
	float speed;
	float angle;
};

struct Move
{
     float time;
     Input input;
     State state;
};

//------------------------------------------------------------------------------------------------
class Bullet;

class Person
{
private:

protected:

public:
	int mId;
	float mX;
	float mY;
	float mOldX;
	float mOldY;
	int mMoveState;
	float mSpeed;
	float mMaxSpeed;
	float mAngle;
	int mHealth;
	int mMoney;
	float mFacingAngle;
	float mRecoilAngle;
	float mLastFireAngle;
	bool mIsActive;
	int mNumDryFire;
	int mState;
	float mStateTime;
	int mTeam;
	int mType;
	char mName[32];
	char mAccountName[32];
	int mNumKills;
	int mNumDeaths;
	int mMovementStyle;

	std::vector<GunObject*>* mGunObjects;
	std::vector<Bullet*>* mBullets;

	GunObject* mGuns[5];
	int mGunIndex;

	bool mIsFiring; //new
	bool mIsReloading; //new
	bool mHasFired;

	bool mIsFlashed;
	float mFlashTime;
	float mFlashIntensity;

	bool mIsInBuyZone;

	UdpManager* mUdpManager;

	float mCurrentTime;

	float *mTime;
	float mLastFireTime;

	bool mIsPositionSafe;
	bool mSendInput;

	char mIcon[300];

	float mRespawnTime;
	bool mHasFlag;
	bool mIsInCaptureZone;

	float mInvincibleTime;

	Person(std::vector<GunObject*>* gunobjects, std::vector<Bullet*>* bullets, UdpManager* udpmanager, char* name);
	~Person();

	void Update(float dt);
	//virtual void Render(float x, float y);
	void Move(float speed, float angle);
	std::vector<Bullet*> Fire();
	std::vector<Bullet*> StopFire();
	void Reload();
	void Switch(int index);
	void SwitchNext();
	bool PickUp(GunObject* gunobject);
	bool Drop(int index, float speed = 0.35f);

	void RotateFacing(float theta);
	void SetMoveState(int state);
	void SetState(int state);
	void SetTotalRotation(float theta);

	void Die(bool drop = true);
	void Reset();
	void TakeDamage(int damage);
	void ReceiveFlash(float intensity);

	void ReceiveInput(Input input, float time);

	GunObject* GetCurrentGun();
};
