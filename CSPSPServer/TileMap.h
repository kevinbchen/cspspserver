#ifndef _TILEMAP_H_
#define _TILEMAP_H_

#include "GunObject.h"
#include <vector>
#include "Vector2D.h"
#include "Collision.h"

#define TEAM 0
#define FFA 1
#define CTF 2

struct CollisionPoint {
	float x;
	float y;
	bool bullets;
	bool people;
};

struct CollisionLine
{
	Line line;
	bool bullets;
	bool people;

	CollisionLine(Line _line): line(_line) {}
	~CollisionLine() {}
};

struct BuyZone {
	float x1;
	float y1;
	float x2;
	float y2;
};

//------------------------------------------------------------------------------------------------
class TileMap
{
private:
	//std::vector<JQuad*> mTiles;
	std::vector<int> gMap;
	//JTexture *mTexture;
	bool loaded;

protected:

public:
	char mName[20];
	int mCols;
	int mRows;

	std::vector<CollisionLine> mCollisionLines;
	std::vector<CollisionPoint> mCollisionPoints;

	std::vector<Vector2D*> mCTSpawns;
	std::vector<Vector2D*> mTSpawns;
	std::vector<BuyZone> mCTBuyZones;
	std::vector<BuyZone> mTBuyZones;
	//std::vector<Node*> mNodes;
	int mNumPoints;
	int mNumCTs;
	int mNumTs;
	std::vector<GunObject*>* mGunObjects;
	std::vector<GunObject*> mGunObjectsSpawn;
	Gun mGuns[28];

	Vector2D mFlagSpawn[2];

	TileMap(Gun guns[], std::vector<GunObject*>* gunobjects);
	~TileMap();
	bool Load(char *mapFile, int &gameType);
	void Unload();
	void Update(float dt);
	//void Render(float x, float y);
	void Reset(int &guncounter);
};

#endif
