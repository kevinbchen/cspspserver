
#include "TileMap.h"

#ifdef _WIN32
#else
#define stricmp strcasecmp
#endif

//------------------------------------------------------------------------------------------------
TileMap::TileMap(Gun guns[], std::vector<GunObject*>* gunobjects)
{
	//mEngine = JGECreate();
	loaded = false;
	for (int i=0; i<28; i++) {
		mGuns[i] = guns[i];
	}
	mGunObjects = gunobjects;
}

//------------------------------------------------------------------------------------------------
TileMap::~TileMap()
{
	if (loaded) {
		Unload();
	}

	//JGERelease();
}

//------------------------------------------------------------------------------------------------
bool TileMap::Load(char *mapFile, int &gameType)
{
	bool ctf = false; //hack
	bool ffa = false;

	char fileName[80];
	/*strcpy(fileName,"maps/");
	strcat(fileName,mapFile);
	strcat(fileName,"/tile.png");

	mTexture = mEngine->LoadTexture(fileName, true);
	//mTiles = new JQuad*[mTexture->mTexWidth/32*mTexture->mTexHeight/32];
	for (int i=0;i<mTexture->mTexHeight/32;i++) {
		for (int j=0;j<mTexture->mTexWidth/32;j++) {
			mTiles.push_back(new JQuad(mTexture,j*32.0f,i*32.0f,32.0f,32.0f));
		}
	}*/

	strcpy(fileName,"maps/");
	strcat(fileName,mapFile);
	strcat(fileName,"/map.txt");

    FILE *file;

    file = fopen(fileName, "r"); 

	if (file == NULL) return false;

    int w = 0;
	int h = 0;
    char line[4096]; 
	char* s;
    fgets(line,4096,file);
    sscanf(line,"%d,%d",&w,&h);
	if (w != 0 && h != 0) {
		if (gameType != TEAM) {
			fclose(file);
			return false;
		}
		mCols = w;
		mRows = h;
		//gMap = new int*[w];
		/**for (int i=0;i<w;i++) {
			gMap.push_back(gMap[i].push_back(new int[h]));
		}**/
		s = line; 
		int index;
		for (int y=0;y<h;y++) 
		{ 
			if (!fgets(line,4096,file)) break; // read error, you should handle this properly 
			s = line; // This was what the problem was! 
			while (s) {
				sscanf(s,"%d", &index);
				gMap.push_back(index);
				s = strchr(s,','); 
				if (s != NULL) {
					s += 1;
				}
			}
		} 
		
		int x, y; 
		fgets(line,4096,file);

		mNumPoints = 0;
		/**s = line; 
		while (s) {
			s = strchr(s+1,'('); 
			mNumPoints++;
		}**/
		//mCollisionPoints = new Vector2D*[mNumPoints];
		s = line;
		while (s) {
			sscanf(s,"(%d,%d)", &x, &y);
			CollisionPoint point = {x,y,true,true};
			mCollisionPoints.push_back(point);
			s = (strchr(s,','))+1;
			s = (strchr(s,','));
			if (s != NULL) {
				s += 1; 
			}
			mNumPoints++;
		} 
		
		fgets(line,4096,file);
		mNumCTs = 0;
		/**s = line; 
		while (s) {
			s = strchr(s+1,'('); 
			mNumCTs++;
		}**/
		s = line;
		while (s) {
			sscanf(s,"(%d,%d)", &x, &y);
			mCTSpawns.push_back(new Vector2D((float)x,(float)y));
			s = (strchr(s,','))+1;
			s = (strchr(s,','));
			if (s != NULL) {
				s += 1; 
			}
			mNumCTs++;
		} 

		fgets(line,4096,file); 
		mNumTs = 0;
		/**s = line; 
		while (s) {
			s = strchr(s+1,'('); 
			mNumTs++;
		}**/
		s = line;
		while (s) {
			sscanf(s,"(%d,%d)", &x, &y);
			mTSpawns.push_back(new Vector2D((float)x,(float)y));
			s = (strchr(s,','))+1;
			s = (strchr(s,','));
			if (s != NULL) {
				s += 1; 
			}
			mNumTs++;
		} 

		fgets(line,4096,file);
		//int numPoints = 0;
		/**s = line; 
		while (s) {
			s = strchr(s+1,'('); 
			numPoints++;
		}**/
		s = line;
		while (s) {
			sscanf(s,"(%d,%d)", &x, &y);
			
			//mNodes.push_back(new Node(x,y));
			s = (strchr(s,','))+1;
			s = (strchr(s,','));
			if (s != NULL) {
				s += 1; 
			}
			//numPoints++;
		} 


		fgets(line,4096,file);
		//int numGuns = 0;
		/**s = line; 
		while (s) {
			s = strchr(s+1,':'); 
			numGuns++;
		}
		numGuns--;**/
		s = line;
		while (s) {
			sscanf(s,"%d:(%d,%d)", &index, &x, &y);

			if (index == 0) {
				index = 17;
			}
			else if (index == 1) {
				index = 18;
			}
			else if (index == 2) {
				index = 11;
			}
			else if (index == 3) {
				index = 13;
			}
			else if (index == 4) {
				index = 23;
			}
			else if (index == 5) {
				index = 24;
			}
			else if (index == 6) {
				index = 1;
			}
			else if (index == 7) {
				index = 2;
			}
			else if (index == 8) {
				index = 0;
			}

			Gun *gun = &mGuns[index];
			GunObject *gunobject = new GunObject(gun,gun->mClip,gun->mClip*(gun->mNumClips-1));
			gunobject->SetPosition(x,y);
			mGunObjectsSpawn.push_back(gunobject);
			s = (strchr(s,','))+1;
			s = (strchr(s,','));
			if (s != NULL) {
				s += 1; 
			}
		}
	}
	else {
		fclose(file);
		file = fopen(fileName, "r"); 
		char section[4096];
		int index, x, y;
		while(fgets(line,4096,file)) {
			if (sscanf(line,"%s %*s",section) != EOF) {
				if (strcmp(section,"tiles") == 0) {
					mRows = 0;
					mCols = 0;
					while (strncmp(fgets(line,4096,file),"}",1) != 0) {
						s = line;
						while (s) {
							sscanf(s,"%d", &index);
							gMap.push_back(index);
							s = strchr(s,','); 
							if (s != NULL) {
								s += 1;
							}
							mCols++;
						}
						mRows++;
					}
					mCols /= mRows;
				}
				else if (strcmp(section,"collision") == 0) {
					mNumPoints = 0;
					while (strncmp(fgets(line,4096,file),"}",1) != 0) {
						s = line;
						while (s) {
							sscanf(s,"(%d,%d)", &x, &y);
							CollisionPoint point = {x,y,true,true};
							mCollisionPoints.push_back(point);
							s = (strchr(s,','))+1;
							s = (strchr(s,','));
							if (s != NULL) {
								s += 1; 
							}
							mNumPoints++;
						} 
						CollisionPoint point = {-1,-1,true,true};
						mCollisionPoints.push_back(point);
						mNumPoints++;
					}
				}
				else if (stricmp(section,"collisionPeople") == 0) {
					mNumPoints = 0;
					while (strncmp(fgets(line,4096,file),"}",1) != 0) {
						s = line;
						while (s) {
							sscanf(s,"(%d,%d)", &x, &y);
							CollisionPoint point = {x,y,false,true};
							mCollisionPoints.push_back(point);
							s = (strchr(s,','))+1;
							s = (strchr(s,','));
							if (s != NULL) {
								s += 1; 
							}
							mNumPoints++;
						} 
						CollisionPoint point = {-1,-1,true,true};
						mCollisionPoints.push_back(point);
						mNumPoints++;
					}
				}
				else if (strcmp(section,"waypoints") == 0) {
					while (strncmp(fgets(line,4096,file),"}",1) != 0) {
						s = line;
						while (s) {
							sscanf(s,"(%d,%d)", &x, &y);
							//mNodes.push_back(new Node(x,y));
							s = strchr(s,';'); 
							if (s != NULL) {
								s += 1;
							}
						} 
					}
				}
				else if (strcmp(section,"spawns") == 0) {
					while (strncmp(fgets(line,4096,file),"}",1) != 0) {
						s = line;
						while (s) {
							sscanf(s,"(%d,%d,%d)", &index, &x, &y);
							if (gameType == FFA) {
								if (index == 2) {
									mCTSpawns.push_back(new Vector2D((float)x,(float)y));
									ffa = true;
								}
							}
							else {
								if (index == 0) {
									mCTSpawns.push_back(new Vector2D((float)x,(float)y));
								}
								else if (index == 1) {
									mTSpawns.push_back(new Vector2D((float)x,(float)y));
								}
							}
							
							s = strchr(s,';'); 
							if (s != NULL) {
								s += 1;
							}
						} 
					}
					mNumCTs = mCTSpawns.size();
					mNumTs = mTSpawns.size();
				}		
				else if (strcmp(section,"buyzones") == 0) {
					while (strncmp(fgets(line,4096,file),"}",1) != 0) {
						s = line;
						while (s) {
							BuyZone buyzone = BuyZone();
							int x2, y2;
							sscanf(s,"(%d,%d,%d,%d,%d)", &index, &x, &y, &x2, &y2);
							buyzone.x1 = x;
							buyzone.y1 = y;
							buyzone.x2 = x2;
							buyzone.y2 = y2;
							if (gameType == FFA) {
								if (index == 2) {
									mCTBuyZones.push_back(buyzone);
								}
							}
							else {
								if (index == 0) {
									mCTBuyZones.push_back(buyzone);
								}
								else if (index == 1) {
									mTBuyZones.push_back(buyzone);
								}
							}
							
							s = strchr(s,';'); 
							if (s != NULL) {
								s += 1;
							}
						} 
					}
				}
				else {
					char gunsSection[32] = "guns";
					if (gameType == FFA) {
						strcpy(gunsSection,"guns_ffa");
					}
					if (strcmp(section,gunsSection) == 0) {
						while (strncmp(fgets(line,4096,file),"}",1) != 0) {
							s = line;
							while (s) {
								sscanf(s,"(%d,%d,%d)", &index, &x, &y);
								Gun *gun = &mGuns[index];
								GunObject *gunobject = new GunObject(gun,gun->mClip,gun->mClip*(gun->mNumClips-1));
								gunobject->SetPosition(x,y);
								mGunObjectsSpawn.push_back(gunobject);
								s = strchr(s,';');
								if (s != NULL) {
									s += 1; 
								}
							} 
						}
					}

					if (gameType == CTF) {
						if (strcmp(section,"flags") == 0) {
							while (strncmp(fgets(line,4096,file),"}",1) != 0) {
								s = line;
								while (s) {
									sscanf(s,"(%d,%d,%d)", &index, &x, &y);
									if (index == 0) {
										mFlagSpawn[CT].x = x;
										mFlagSpawn[CT].y = y;
										ctf = true;
									}
									else if (index == 1) {
										mFlagSpawn[T].x = x;
										mFlagSpawn[T].y = y;
										ctf = true;
									}
									s = strchr(s,';');
									if (s != NULL) {
										s += 1; 
									}
								} 
							}
						}
					}
				}
			}
		}
	}

	fclose(file);

	if (gameType == CTF && !ctf) {
		gameType = TEAM;
	}
	if (gameType == FFA && !ffa) {
		gameType = TEAM;
	}

	for (unsigned int i=0; i<mCollisionPoints.size()-1; i++) {
		if (mCollisionPoints[i].x == -1 || mCollisionPoints[i+1].x == -1) continue;
		if (fabs(mCollisionPoints[i].x-mCollisionPoints[i+1].x) < EPSILON && fabs(mCollisionPoints[i].y-mCollisionPoints[i+1].y)  < EPSILON) continue;

		CollisionLine collisionLine = CollisionLine(Line(mCollisionPoints[i].x,mCollisionPoints[i].y,mCollisionPoints[i+1].x,mCollisionPoints[i+1].y));
		//collisionLine->line = Line(mMap->mCollisionPoints[i].x,mMap->mCollisionPoints[i].y,mMap->mCollisionPoints[i+1].x,mMap->mCollisionPoints[i+1].y);
		collisionLine.people = mCollisionPoints[i].people;
		collisionLine.bullets = mCollisionPoints[i].bullets;
		mCollisionLines.push_back(collisionLine);
	}


	if (mCTBuyZones.size() == 0 && mTBuyZones.size() == 0) {
		for (unsigned int i=0; i<mCTSpawns.size(); i++) {
			BuyZone buyzone = BuyZone();
			buyzone.x1 = mCTSpawns[i]->x-100;
			buyzone.y1 = mCTSpawns[i]->y-100;
			buyzone.x2 = mCTSpawns[i]->x+100;
			buyzone.y2 = mCTSpawns[i]->y+100;
			mCTBuyZones.push_back(buyzone);
		}
		for (unsigned int i=0; i<mTSpawns.size(); i++) {
			BuyZone buyzone = BuyZone();
			buyzone.x1 = mTSpawns[i]->x-100;
			buyzone.y1 = mTSpawns[i]->y-100;
			buyzone.x2 = mTSpawns[i]->x+100;
			buyzone.y2 = mTSpawns[i]->y+100;
			mTBuyZones.push_back(buyzone);
		}
	}
	/*for (unsigned int i=0;i<mNodes.size();i++) 
	{ 
		for (unsigned int j=0;j<mNodes.size();j++) {
			if (mNodes[i] == mNodes[j]) continue;
			bool isConnected = true;
			Vector2D A(mNodes[i]->mX,mNodes[i]->mY);
			Vector2D B(mNodes[j]->mX,mNodes[j]->mY);
			for (int k=0;k<mNumPoints-1;k++) {
				if (mCollisionPoints[k]->x == -1 || mCollisionPoints[k+1]->x == -1) continue;
				Vector2D C(mCollisionPoints[k]->x,mCollisionPoints[k]->y);
				Vector2D D(mCollisionPoints[k+1]->x,mCollisionPoints[k+1]->y);

				float divisor = (B.x-A.x)*(D.y-C.y)-(B.y-A.y)*(D.x-C.x);
				if (divisor != 0) {
					float r = ((A.y-C.y)*(D.x-C.x)-(A.x-C.x)*(D.y-C.y))/divisor;
					float s = ((A.y-C.y)*(B.x-A.x)-(A.x-C.x)*(B.y-A.y))/divisor;
					if ((0 <= r) && (r <= 1) && (0 <= s) && (s <= 1)) {
						isConnected = false;
					}
				}
			}
			if (isConnected) {
				mNodes[i]->mConnections.push_back(mNodes[j]);
			}
		}
	} */
	loaded = true;
	return true;
}

//------------------------------------------------------------------------------------------------
void TileMap::Unload()
{
	/*if (mTexture)
		mEngine->FreeTexture(mTexture);

	for (unsigned int i=0;i<mTiles.size();i++) {
		delete mTiles[i];
	}
	mTiles.clear();	*/

	gMap.clear();	

	/*for (unsigned int i=0;i<mCollisionPoints.size();i++) {
		delete mCollisionPoints[i];
	}*/
	mCollisionPoints.clear();	
	mCollisionLines.clear();

	for (unsigned int i=0;i<mCTSpawns.size();i++) {
		delete mCTSpawns[i];
	}
	mCTSpawns.clear();	

	for (unsigned int i=0;i<mTSpawns.size();i++) {
		delete mTSpawns[i];
	}
	mTSpawns.clear();	

	mCTBuyZones.clear();
	mTBuyZones.clear();
	/*for (unsigned int i=0;i<mNodes.size();i++) {
		delete mNodes[i];
	}
	mNodes.clear();	*/
	
	for (unsigned int i=0;i<mGunObjectsSpawn.size();i++) {
		delete mGunObjectsSpawn[i];
	}
	mGunObjectsSpawn.clear();
	loaded = false;

	mFlagSpawn[T].x = 0.0f;
	mFlagSpawn[T].y = 0.0f;
	mFlagSpawn[CT].x = 0.0f;
	mFlagSpawn[CT].y = 0.0f;
}

//------------------------------------------------------------------------------------------------
void TileMap::Update(float dt)
{
	
}



//------------------------------------------------------------------------------------------------
/*void TileMap::Render(float x, float y)
{	

	int mapX = (int)x-SCREEN_WIDTH_2;
	int mapY = (int)y-SCREEN_HEIGHT_2;


	int col = mapX/32;
	float xoffset = -(float)(mapX%32);
	int row = mapY/32;
	float yy = -(float)(mapY%32);

	int start = mCols * row + col;
	
	float xx;
	int currCol;
	int n, index;
	while (yy < SCREEN_HEIGHT_F && row < mRows)
	{
		xx = xoffset;
		currCol = col;
		index = start;
		while (xx < SCREEN_WIDTH_F && currCol< mCols)
		{
			if (currCol < 0 || row < 0) {
				n = -1;
			}
			else {
				n = (int)gMap[currCol+row*mCols];
			}
			if (n >= 0)
			{
				mEngine->RenderQuad(mTiles[n],xx,yy,0.0f,1.0f,1.0f);
			}
			xx += 32;
			currCol++;
			
		}

		yy += 32;
		start += mCols;
		row++;
	}
	
}*/

//------------------------------------------------------------------------------------------------
void TileMap::Reset(int &guncounter)
{
	for(unsigned int i=0;i<mGunObjectsSpawn.size();i++) {
		GunObject *gunobject = new GunObject(mGunObjectsSpawn[i]->mGun,mGunObjectsSpawn[i]->mClipAmmo,mGunObjectsSpawn[i]->mRemainingAmmo);
		gunobject->SetPosition(mGunObjectsSpawn[i]->mX,mGunObjectsSpawn[i]->mY);

		guncounter++;
		if (guncounter > 32767) {
			for (guncounter=0; guncounter<=32767; guncounter++) {
				bool taken = false;
				for (int i=0; i<mGunObjects->size(); i++) {
					if ((*mGunObjects)[i]->mId == guncounter) {
						taken = true;
					}
				}
				if (!taken) break;
			}
		}

		gunobject->mId = guncounter;
		mGunObjects->push_back(gunobject);
	}
}
