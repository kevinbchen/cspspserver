#pragma once

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <errno.h>
#include <math.h>
#include <sstream>

#ifdef _WIN32
#include <winsock.h>

#define socklen_t int
#else
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "Packet.h"
#include "Person.h"
#include "Vector2D.h"
#include "TileMap.h"
#include "UdpManager.h"
#include "Collision.h"
#include "Bullet.h"
#include "HttpManager.h"
#include "Grid.h"

#define VERSION 1.51f
#define NETVERSION 9

#define NONE -1
#define T 0
#define CT 1
#define TIE 2
#define ON 0
#define OFF 1

#define FREEZETIME 0
#define STARTED 1

#define MAXFILESIZE 400

class UdpManager;

struct MapInfo {
	char name[256];
	int type;
};

class GameServer
{
public:
	// Callback functions invoked when mPeople or mBannedPeople is updated, respectively
	std::function<void()> mOnPlayerListUpdate;
	std::function<void()> mOnBanListUpdate;

	bool mHasError;
	//string mOutput;
	std::stringstream mOutStream;

#ifdef _WIN32
	WSADATA wsaData;	// Windows socket
#endif
	int sock, length;
	socklen_t fromlen;
	struct sockaddr_in server;
	struct sockaddr_in from;
	char buffer[4096];

	//char mHTTPBuffer[4096];

	HttpManager* mHttpManager;

	//int websock;
	//struct sockaddr_in webserver;
	std::string ipaddress;
	int mPort;
	
	std::string mInput;
	float mCursorTimer;

	std::vector<MapInfo> mMapCycle;
	int mMapIndex;
	float mMapTime;
	float mMapTimer;
	int mMapTextSize;
	int mMapImageSize;
	int mMapOverviewSize;
	FILE *mMapTextFile;
	FILE *mMapImageFile;
	FILE *mMapOverviewFile;

	int mGameType;

	int mRoundFreezeTime;
	int mRoundTime;
	int mRoundEndTime;
	int mBuyTime;
	int mRespawnTime;

	int mSpawnGunIndex;
	int mInvincibleTime;

	float mLastRoundTime;

	float mRoundTimer;
	float mRoundEndTimer;
	float mBuyTimer;
	int mRoundState;

	int mNumRounds;
	int mNumCTWins;
	int mNumTWins;

	int mNumFlags[2];
	float mFlagX[2];
	float mFlagY[2];
	bool mIsFlagHome[2];
	Person* mFlagHolder[2];

	int mRoundBit;

	int mNumGuns;

	UdpManager* mUdpManager;

	std::vector<Person*> mPeople;
	std::vector<GunObject*> mGunObjects;
	std::vector<Bullet*> mBullets;

	//Person* mPlayer;
	TileMap* mMap;
	Grid* mGrid;
	Gun mGuns[28];

	int mNumCTs;
	int mNumTs;

	int mNumRemainingCTs;
	int mNumRemainingTs;

	int mNumPlayers;
	int mWinner;

	float mTime;

	int mPlayerCounter;
	int mGunCounter;
	int mBulletCounter;
	int mChatCounter;
	int ackcounter;

	float mPingTimer;

	bool mUpdating;
	float mUpdateTimer;

	float mSendMovementTimer;

	char* mName;
	char mMapName[32];
	int mNumMaxPlayers;
	int mFriendlyFire;
	int mAutoBalance;
	int mAllTalk;

	float mTimeMultiplier;

	std::vector<char*> mBannedPeople;
	std::vector<char*> mAdmins;

	GameServer();
	~GameServer();
	void Init();

	void Update(float dt);
	void CheckCollisions();
	void CheckPlayerCollisions(Person* player);
	void HandlePacket(Packet &packet, Connection* connection, sockaddr_in from, bool sendack = true);
	void ResetRound(bool fullreset = false);
	void RespawnPlayer(Person* player, int x, int y);
	void UpdateScores(Person* attacker, Person* victim, Gun* weapon);

	void CleanUp();
	int Register();

	char* GetConfig(const char *location, char searchstr[], int length = 32);

	Person* GetPerson(int id);
	int GetPlayerId();
	int GetGunId();

	void Buy(Person* player, int index);
	void Explode(Grenade* grenade);
	void HandleInput(char* input, bool remote = false);
	bool LoadMap(char* mapname, int maptype);
	void RemovePerson(Person* player);

	std::string GetPersonName(Person* player);
	//bool ReadHTTP(char* string);

	char* DecodeText(char* buffer, char* text);

	void Kick(char* name);
	void Ban(char* name);
	void Unban(char* name);

	void Hash();
};
