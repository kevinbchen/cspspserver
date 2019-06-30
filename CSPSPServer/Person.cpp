#include "Person.h"

//------------------------------------------------------------------------------------------------
Person::Person(std::vector<GunObject*>* gunobjects, std::vector<Bullet*>* bullets, UdpManager* udpmanager, char* name)
{
	mIsFiring = false;
	mIsReloading = false;
	mHasFired = false;
	mX = 0;
	mY = 0;
	mOldX = mX;
	mOldY = mY;
	mSpeed = 0;
	mMaxSpeed = 0;
	mState = DEAD;
	mStateTime = 0;
	mAngle = 0;
	mRecoilAngle = 0.0f;
	mFacingAngle = M_PI_2;
	mLastFireAngle = 0.0f;
	mNumDryFire = 0;
	mMoveState = NOTMOVING;
	mGunObjects = gunobjects;
	mBullets = bullets;
	mMoney = 800;
	mHealth = 100;

	for (int i=0; i<5; i++) {
		mGuns[i] = NULL;
	}
	mGunIndex = KNIFE;
	mTeam = NONE;
	mType = 0;

	mUdpManager = udpmanager;
	mIsActive = true;
	mNumKills = 0;
	mNumDeaths = 0;

	strncpy(mName,name,31);
	mName[31] = '\0';

	mCurrentTime = 0.0f;

	mLastFireTime = 0.0f;

	mIsPositionSafe = false;

	mIsFlashed = false;
	mFlashTime = 0.0f;
	mFlashIntensity = 0.0f;

	mIsInBuyZone = false;
	mSendInput = false;

	mRespawnTime = 5000.0f;
	mInvincibleTime = 0.0f;

	mHasFlag = false;

	mIsInCaptureZone = false;
}

//------------------------------------------------------------------------------------------------
Person::~Person()
{
	for (int i=0; i<5; i++) {
		if (mGuns[i]) {
			delete mGuns[i];
		}
	}
}


//------------------------------------------------------------------------------------------------
void Person::Update(float dt)
{
	mStateTime += dt;
	if (mState == DEAD) {
		mRespawnTime -= dt;
		/*if (mStateTime >= 2000) {
			mFadeTime -= dt;
			if (mFadeTime < 0) {
				mFadeTime = 0;
			}
		}*/
		return;
	}

	mIsPositionSafe = true;
	if (mIsActive) {
		if (mMoveState == NOTMOVING) {
			mSpeed -= .0005f*dt;
			if (mSpeed < 0) {
				mSpeed = 0.0f;
			}
		}
		else if (mMoveState == MOVING) {
			mSpeed += .0005f*dt;
			if (mSpeed > mMaxSpeed) {
				mSpeed = mMaxSpeed;
			}
			if (mRecoilAngle < mGuns[mGunIndex]->mGun->mSpread*0.5f) {
				mRecoilAngle += mGuns[mGunIndex]->mGun->mSpread/50.0f*dt;
				if (mRecoilAngle > mGuns[mGunIndex]->mGun->mSpread*0.5f) {
					mRecoilAngle = mGuns[mGunIndex]->mGun->mSpread*0.5f;
				}
			}
		}
	}

	/*mOldX = mX;
	mOldY = mY;

	mX += mSpeed*cosf(mAngle)*dt;
	mY += mSpeed*sinf(mAngle)*dt;*/

	if (mMoveState == MOVING) {
		if (mRecoilAngle < mGuns[mGunIndex]->mGun->mSpread*0.5f) {
			mRecoilAngle += mGuns[mGunIndex]->mGun->mSpread/50.0f*dt;
			if (mRecoilAngle > mGuns[mGunIndex]->mGun->mSpread*0.5f) {
				mRecoilAngle = mGuns[mGunIndex]->mGun->mSpread*0.5f;
			}
		}
	}

	if (mGuns[mGunIndex]->mGun->mId == 7 || mGuns[mGunIndex]->mGun->mId == 8) {
		mRecoilAngle = mGuns[mGunIndex]->mGun->mSpread;
	}

	mLastFireAngle = mFacingAngle;

	if (mState == NORMAL) {
		if (mGuns[mGunIndex]->mGun->mId != 7 && mGuns[mGunIndex]->mGun->mId != 8) {
			mRecoilAngle -= mGuns[mGunIndex]->mGun->mSpread/100.0f*dt;
			if (mRecoilAngle < 0) {
				mRecoilAngle = 0.0f;
			}
		}
	}
	else if (mState == ATTACKING) {
		if (mMoveState == NOTMOVING) {
			mRecoilAngle += mGuns[mGunIndex]->mGun->mSpread/500.0f*dt;
		}
		else if (mMoveState == MOVING) {
			mRecoilAngle += mGuns[mGunIndex]->mGun->mSpread/50.0f*dt;
		}

		if  (mGuns[mGunIndex]->mGun->mId == 16 || mGuns[mGunIndex]->mGun->mId == 21 || mGuns[mGunIndex]->mGun->mId == 22 || mGuns[mGunIndex]->mGun->mId == 23) {
			mRecoilAngle = mGuns[mGunIndex]->mGun->mSpread;
		}
		if (mRecoilAngle > mGuns[mGunIndex]->mGun->mSpread) {
			mRecoilAngle = mGuns[mGunIndex]->mGun->mSpread;
		}
		if (mRecoilAngle*500.0f >= 0.1f && mStateTime < 100) {
			mLastFireAngle += (rand()%(int)ceilf(mRecoilAngle*500.0f))/1000.0f-(mRecoilAngle/4.0f);
		}

		if (mStateTime >= mGuns[mGunIndex]->mGun->mDelay) {
			if (mGunIndex == GRENADE) {
				mStateTime = mGuns[mGunIndex]->mGun->mDelay;
			}
			else {
				SetState(NORMAL);
			}
		}

	}
	else if (mState == RELOADING) {
		if (mStateTime >= mGuns[mGunIndex]->mGun->mReloadDelay) {
			mGuns[mGunIndex]->mRemainingAmmo -= (mGuns[mGunIndex]->mGun->mClip-mGuns[mGunIndex]->mClipAmmo);
			mGuns[mGunIndex]->mClipAmmo = mGuns[mGunIndex]->mGun->mClip;
			if (mGuns[mGunIndex]->mRemainingAmmo < 0) {
				mGuns[mGunIndex]->mClipAmmo = mGuns[mGunIndex]->mGun->mClip + mGuns[mGunIndex]->mRemainingAmmo ;
				mGuns[mGunIndex]->mRemainingAmmo = 0;
			}
			SetState(NORMAL);
			
			Packet sendpacket = Packet();
			sendpacket.WriteInt8(ENDRELOAD);
			sendpacket.WriteInt8(mId);
			sendpacket.WriteInt16(mGunIndex);
			sendpacket.WriteInt16(mGuns[mGunIndex]->mClipAmmo);
			sendpacket.WriteInt16(mGuns[mGunIndex]->mRemainingAmmo);
			for (int i=0; i<mUdpManager->mConnections.size(); i++) {
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
			}
		}
		if (mGuns[mGunIndex]->mGun->mId != 7 && mGuns[mGunIndex]->mGun->mId != 8) {
			mRecoilAngle -= mGuns[mGunIndex]->mGun->mSpread/100.0f*dt;
			if (mRecoilAngle < 0) {
				mRecoilAngle = 0.0f;
			}
		}
	}
	else if (mState == DRYFIRING) {
		if (mGunIndex == PRIMARY) {
			if (mStateTime >= 250) {
				SetState(NORMAL);
				mNumDryFire++;
			}
		}
		else if (mGunIndex == SECONDARY) {
			SetState(NORMAL);
			mNumDryFire++;
		}
		else if (mGunIndex == KNIFE) {
			if (mStateTime >= mGuns[mGunIndex]->mGun->mDelay) {
				SetState(NORMAL);
			}
		}
	}
	else if (mState == SWITCHING) {
		if (mGunIndex == PRIMARY || mGunIndex == SECONDARY) {
			if (mStateTime >= mGuns[mGunIndex]->mGun->mDelay*0.75f) {
				SetState(NORMAL);
			}
		}
		else {
			SetState(NORMAL);
		}
	}

	if (mIsFlashed) {
		mFlashTime -= dt/mFlashIntensity;
		if (mFlashTime < 0.0f) {
			mFlashTime = 0.0f;
			mIsFlashed = false;
		}
	}

	mInvincibleTime -= dt;
	if (mInvincibleTime < 0.0f) {
		mInvincibleTime = 0.0f;
	}
}

//------------------------------------------------------------------------------------------------
void Person::Move(float speed, float angle)
{
	if (!mIsActive) return;
	if (mState == DEAD) return;
	SetMoveState(MOVING);
	mMaxSpeed = speed*mGuns[mGunIndex]->mGun->mSpeed;
	mAngle = angle;
}

//------------------------------------------------------------------------------------------------
std::vector<Bullet*> Person::Fire()
{
	std::vector<Bullet*> bullets;
	if (!mIsActive) return bullets;
	Bullet* bullet;
	//if (!mIsActive) return false;
	if (mState == NORMAL) {
		if (mGunIndex == KNIFE) {
			SetState(ATTACKING);
			return bullets;
		}
		else if (mGunIndex == GRENADE) {
			if (mGuns[mGunIndex]->mClipAmmo != 0)  {
				SetState(ATTACKING);
			}
		}
		else {
			if (mGuns[mGunIndex]->mClipAmmo != 0)  {

				float h = 24*sinf(mFacingAngle);
				float w = 24*cosf(mFacingAngle);
				float theta = mFacingAngle;
				float speed = 0.3f*mGuns[mGunIndex]->mGun->mBulletSpeed;
				if (mGuns[mGunIndex]->mGun->mId == 7) {
					theta -= 0.36f;
					for (int i=0; i<6; i++) {
						theta += (rand()%11)/100.0f-0.05f;
						bullet = new Bullet(mX+w,mY+h,mX,mY,theta,speed,abs(mGuns[mGunIndex]->mGun->mDamage+rand()%17-8),this);
						bullets.push_back(bullet);
						//mBullets->push_back(bullet);
						theta += 0.144f;
					}
				}
				else if (mGuns[mGunIndex]->mGun->mId == 8) {
					theta -= 0.36f;
					for (int i=0; i<4; i++) {
						theta += (rand()%10)/100.0f-0.05f;
						bullet = new Bullet(mX+w,mY+h,mX,mY,theta,speed,abs(mGuns[mGunIndex]->mGun->mDamage+rand()%17-8),this);
						bullets.push_back(bullet);
						//mBullets->push_back(bullet);
						theta += 0.24f;
					}
				}
				else {
					if (mRecoilAngle*1000.0f >= 0.1f) {
						theta += (rand()%(int)ceilf(mRecoilAngle*1000.0f))/1000.0f-(mRecoilAngle*0.5f);
						//theta = mFacingAngle + (rand()%100)/400.0f-0.125f;
					}
					bullet = new Bullet(mX+w,mY+h,mX,mY,theta,speed,abs(mGuns[mGunIndex]->mGun->mDamage+rand()%17-8),this);
					bullets.push_back(bullet);
					//mBullets->push_back(bullet);
				}
				SetState(ATTACKING);
				//mGuns[mGunIndex]->mClipAmmo--;
				//JSample *test = mEngine->LoadSample("sfx/m249.wav");
				//return bullets;
			}
			else {
				SetState(DRYFIRING);
				//return bullets;
			}
		}
	}
	return bullets;
}

//------------------------------------------------------------------------------------------------
std::vector<Bullet*> Person::StopFire()
{
	std::vector<Bullet*> bullets;
	if (!mIsActive) return bullets;

	mIsFiring = false;
	mHasFired = false;

	if (mState == ATTACKING) {
		if (mGunIndex == GRENADE) {
			if (mGuns[mGunIndex]->mClipAmmo != 0)  {
				SetState(NORMAL);
				mGuns[mGunIndex]->mClipAmmo--;

				delete mGuns[mGunIndex];
				mGuns[mGunIndex] = NULL;
				SwitchNext();
			}
		}
	}
	return bullets;
}

//------------------------------------------------------------------------------------------------
void Person::Reload()
{
	if (mGunIndex == KNIFE || mGunIndex == GRENADE) return;
	if (mState != RELOADING && mGuns[mGunIndex]->mClipAmmo != mGuns[mGunIndex]->mGun->mClip && mGuns[mGunIndex]->mRemainingAmmo != 0) {
		SetState(RELOADING);
		mNumDryFire = 0;
	}
}

//------------------------------------------------------------------------------------------------
void Person::Switch(int index)
{
	if (mGuns[index] != NULL) {
		mGunIndex = index;

		if (mState == RELOADING) {
			//SetState(NORMAL);
		}
		mRecoilAngle = 0.0f;
		mNumDryFire = 0;

		SetState(SWITCHING);
	}
}

//------------------------------------------------------------------------------------------------
void Person::SwitchNext()
{
	if (mState == RELOADING) {
		//SetState(NORMAL);
	}
	else if (mState == ATTACKING) {
		//SetState(NORMAL);
	}

	int gunindex = mGunIndex;
	for (int i=0; i<5; i++) {
		gunindex++;
		if (gunindex > 4) {
			gunindex = 0;
		}
		if (mGuns[gunindex] != NULL) break;
	}

	if (gunindex == mGunIndex) return;

	mGunIndex = gunindex;

	mRecoilAngle = 0.0f;
	mNumDryFire = 0;
	SetState(SWITCHING);
}

//------------------------------------------------------------------------------------------------
bool Person::PickUp(GunObject* gunobject)
{
	if (gunobject->mGun->mType == PRIMARY) {
		if (mGuns[PRIMARY] == NULL) {
			mGuns[PRIMARY] = gunobject;
			gunobject->mOnGround = false;
			/*if (mState == RELOADING) {
				SetState(NORMAL);
			}*/
			//mGunIndex = PRIMARY;
			Switch(PRIMARY);
			return true;
		}
	}
	else if (gunobject->mGun->mType == SECONDARY) {
		if (mGuns[SECONDARY] == NULL) {
			mGuns[SECONDARY] = gunobject;
			gunobject->mOnGround = false;
			if (mGuns[PRIMARY] == NULL) {
				//mGunIndex = SECONDARY;
				Switch(SECONDARY);
			}
			return true;
		}
	}
	else if (gunobject->mGun->mType == GRENADE) {
		if (mGuns[GRENADE] == NULL) {
			mGuns[GRENADE] = gunobject;
			gunobject->mOnGround = false;
			if (mGuns[PRIMARY] == NULL && mGuns[SECONDARY] == NULL) {
				//mGunIndex = GRENADE;
				Switch(GRENADE);
			}
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------------------------
bool Person::Drop(int index, float speed)
{
	if (index >= 5) return false;
	if (index == KNIFE) return false;

	GunObject* gunobject = mGuns[index];
	if (gunobject == NULL) return false;

	/*if (mState == RELOADING) {
		SetState(NORMAL);
	}*/
	gunobject->SetPosition(mX,mY);
	gunobject->mAngle = mFacingAngle;
	gunobject->mSpeed = speed;
	gunobject->mOnGround = false;
	mGunObjects->push_back(gunobject);

	mGuns[index] = NULL;

	if (mGuns[PRIMARY] != NULL) {
		//mGunIndex = PRIMARY;
		Switch(PRIMARY);
	}
	else if (mGuns[SECONDARY] != NULL) {
		//mGunIndex = SECONDARY;
		Switch(SECONDARY);
	}
	else if (mGuns[GRENADE] != NULL) {
		//mGunIndex = GRENADE;
		Switch(GRENADE);
	}
	else if (mGuns[KNIFE] != NULL) {
		//mGunIndex = KNIFE;
		Switch(KNIFE);
	}

	//mRecoilAngle = 0.0f;
	//mNumDryFire = 0;
	return true;
}

//------------------------------------------------------------------------------------------------
void Person::RotateFacing(float theta)
{
	/*float thetaTemp = GetRotation() + theta;
	if (thetaTemp > M_PI*2.0f) {			// angles are in radian, so 2 PI is one full circle
		thetaTemp -= M_PI*2.0f;
	}
	else if (thetaTemp < 0) {
		thetaTemp += M_PI*2.0f;
	}
	SetRotation(thetaTemp);*/
	float thetaTemp = mFacingAngle + theta;
	if (thetaTemp > M_PI*2.0f) {			// angles are in radian, so 2 PI is one full circle
		thetaTemp -= M_PI*2.0f;
	}
	else if (thetaTemp < 0) {
		thetaTemp += M_PI*2.0f;
	}
	mFacingAngle = thetaTemp;
}

//------------------------------------------------------------------------------------------------
void Person::SetMoveState(int state)
{
	if (mMoveState != state) {
		mMoveState = state;
	}
}

//------------------------------------------------------------------------------------------------
void Person::SetState(int state)
{
	if (mState != state) {
		mState = state;
		mStateTime = 0;
	}
}

//------------------------------------------------------------------------------------------------
void Person::SetTotalRotation(float theta)
{
	mFacingAngle = theta;
	mLastFireAngle = theta-M_PI_2;
	//SetRotation(theta-M_PI_2);
	//mAngle = theta;	
}

//------------------------------------------------------------------------------------------------
void Person::Die(bool drop)
{
	if (drop) {
		if (mState == ATTACKING) {
			if (mGunIndex == GRENADE) {
				if (mGuns[mGunIndex]->mClipAmmo != 0)  {
					float h = 24*sinf(mFacingAngle);
					float w = 24*cosf(mFacingAngle);
					float theta = mFacingAngle;

					float speed = 0;
					int type = HE;

					if (mGuns[mGunIndex]->mGun->mId == 25) { //FLASH
						type = FLASH;
					}
					else if (mGuns[mGunIndex]->mGun->mId == 26) { //HE
						type = HE;
					}
					else if (mGuns[mGunIndex]->mGun->mId == 27) { //SMOKE
						type = SMOKE;
					}

					Grenade* grenade = new Grenade(mX+w,mY+h,mX,mY,theta,speed,this,type);
					mBullets->push_back(grenade);

					SetState(NORMAL);
					mGuns[mGunIndex]->mClipAmmo--;

					delete mGuns[mGunIndex];
					mGuns[mGunIndex] = NULL;
					SwitchNext();

					Packet sendpacket = Packet();
					sendpacket.WriteInt8(NEWGRENADE);
					sendpacket.WriteInt16((int)(mX+w));
					sendpacket.WriteInt16((int)(mY+h));
					//sendpacket.WriteInt16((int)(mX));
					//sendpacket.WriteInt16((int)(mY));

					int angle = (int)(theta*(65535/(2*M_PI)))-32768;
					sendpacket.WriteInt16(angle);

					int speedint = (int)(speed*(65535/0.2f))-32768;
					sendpacket.WriteInt16(speedint);

					sendpacket.WriteInt8(mId);
					sendpacket.WriteFloat(*mTime);

					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					}
					sendpacket.Clear();
				}
			}
		}
				
		int index = mGunIndex;
		GunObject* gunobject = mGuns[mGunIndex];
		GunObject* grenadeobject = mGuns[GRENADE];

		if (Drop(mGunIndex,0)) {
			Packet sendpacket = Packet();
			sendpacket.WriteInt8(DROPGUNDEAD);
			sendpacket.WriteInt8(mId);
			sendpacket.WriteInt8(index);
			sendpacket.WriteInt16(gunobject->mId);
			sendpacket.WriteInt8(gunobject->mGun->mType);
			sendpacket.WriteInt16(gunobject->mX);
			sendpacket.WriteInt16(gunobject->mY);

			int angle = (int)(gunobject->mAngle*(255/(2*M_PI)))-128;
			sendpacket.WriteInt8(angle);

			//sendpacket.WriteFloat(*mTime);

			for (int i=0; i<mUdpManager->mConnections.size(); i++) {
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
			}
		}
		if (Drop(GRENADE,0)) {
			Packet sendpacket = Packet();
			sendpacket.WriteInt8(DROPGUNDEAD);
			sendpacket.WriteInt8(mId);
			sendpacket.WriteInt8(GRENADE);
			sendpacket.WriteInt16(grenadeobject->mId);
			sendpacket.WriteInt8(grenadeobject->mGun->mType);
			sendpacket.WriteInt16(grenadeobject->mX);
			sendpacket.WriteInt16(grenadeobject->mY);

			int angle = (int)(grenadeobject->mAngle*(255/(2*M_PI)))-128;
			sendpacket.WriteInt8(angle);

			//sendpacket.WriteFloat(*mTime);

			for (int i=0; i<mUdpManager->mConnections.size(); i++) {
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
			}
		}
	}

	//Drop(mGunIndex,0);
	if (mGuns[PRIMARY] != NULL) {
		delete mGuns[PRIMARY];
		mGuns[PRIMARY] = NULL;
	}
	if (mGuns[SECONDARY] != NULL) {
		delete mGuns[SECONDARY];
		mGuns[SECONDARY] = NULL;
	}
	/*if (mGunIndex != KNIFE) {
		mGuns[mGunIndex] = NULL;
	}*/
	if (mGuns[GRENADE] != NULL) {
		delete mGuns[GRENADE];
		mGuns[GRENADE] = NULL;
	}
	mGunIndex = KNIFE;

	mNumDryFire = 0;
	SetState(DEAD);
	SetMoveState(NOTMOVING);
	mSpeed = 0.0f;

	//mRespawnTime = 5000.0f;
}

//------------------------------------------------------------------------------------------------
void Person::Reset()
{
	SetTotalRotation(M_PI_2);
	SetMoveState(NOTMOVING);
	mSpeed = 0.0f;
	mHealth = 100;
	if (mState == DEAD) {
		SetState(NORMAL);
	}
	if (mState == ATTACKING || mState == DRYFIRING) {
		SetState(NORMAL);
	}
	mIsActive = false;
	mIsFlashed = false;

	mIsFiring = false;
	mHasFired = false;
	mCurrentTime = 0.0f;
	mLastFireTime = 0.0f;
	mHasFlag = false;
}

void Person::TakeDamage(int damage) {
	if (mInvincibleTime > 0.0f) return;
	if (mState != DEAD) {
		mHealth -= damage;

		if (mHealth <= 0) {
			mHealth = 0;
			Die();
		}

		Packet sendpacket = Packet();
		sendpacket.WriteInt8(HIT);
		sendpacket.WriteInt8(mId);
		sendpacket.WriteInt8(mHealth);
		for (int i=0; i<mUdpManager->mConnections.size(); i++) {
			mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i]);
		}
	}
}

void Person::ReceiveFlash(float intensity) {
	if (mInvincibleTime > 0.0f) return;
	mIsFlashed = true;
	mFlashTime = 15000.0f;
	if (intensity < 0.01f) {
		intensity = 0.01f;
	}
	mFlashIntensity = intensity;

	Packet sendpacket = Packet();
	sendpacket.WriteInt8(RECEIVEFLASH);
	sendpacket.WriteInt8(mId);
	sendpacket.WriteInt8((int)(mFlashIntensity*128));
	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i]);
	}
}

GunObject* Person::GetCurrentGun() {
	return mGuns[mGunIndex];
}

void Person::ReceiveInput(Input input, float time) {
	if (time <= mCurrentTime) return;

	//cout << time;
	//cout << ":";
	//cout << mCurrentTime;
	//cout << "\n";

	float dt = time - mCurrentTime;
	if (dt > 34) {
		//printf("%d %f\n",mId,dt);
	}
	if (mCurrentTime <= 0.0001f) {
		dt = 16.0f;
	}
	
	mCurrentTime = time;
        
	if (mState == DEAD) return;

	int aX = input.x;
	int aY = input.y;
	if (aX >= 20 || aX <= -20 || aY >= 20 || aY <= -20) {
		float angle = atan2f(aX,-aY);
		float speed = (sqrtf(aX*aX + aY*aY)/127.5f)*0.1f;
		if (speed > 0.1f) {
			speed = 0.1f;
		}
		SetMoveState(MOVING);
		mMaxSpeed = speed*mGuns[mGunIndex]->mGun->mSpeed;
		if (mMovementStyle == RELATIVE1) {
			mAngle = input.facingangle+angle;
		}
		else if (mMovementStyle == ABSOLUTE1) {
			mAngle = angle-M_PI_2;
		}
	}
	else {
		SetMoveState(NOTMOVING);
	}


	if (mIsActive) {
		if (mMoveState == NOTMOVING) {
			mSpeed -= .0005f*dt;
			if (mSpeed < 0) {
				mSpeed = 0.0f;
			}
		}
		else if (mMoveState == MOVING) {
			mSpeed += .0005f*dt;
			if (mSpeed > mMaxSpeed) {
				mSpeed = mMaxSpeed;
			}
		}

		if (mIsPositionSafe) {
			mOldX = mX;
			mOldY = mY;
			mIsPositionSafe = false;
		}

		mX += mSpeed*cosf(mAngle)*dt;
		mY += mSpeed*sinf(mAngle)*dt;
	}
	
	mFacingAngle = input.facingangle;

	/*cout << mX;
	cout << ":";
	cout << mY;
	cout << "\n";*/

	mSendInput = true;
}
