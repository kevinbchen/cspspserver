#include "GameServer.h"
#include "Wlan.h"
#include <chrono>

#ifdef _WIN32
#else
#define stricmp strcasecmp
#endif

using std::cout;
using std::string;

void error(char *msg)
{
	if (errno != 0) {
		perror(msg);
		cout << errno;
	}
    //exit(0);
}

GameServer::GameServer()
{
}

void GameServer::Init() {
	WlanInit();

	//strcpy(mHTTPBuffer,"");

	mHasError = false;

	mCursorTimer = 0.0f;

	cout << "CSPSP Server v1.51b\n \n";
	cout << "Type /help for commands\n";

	if (GetConfig("data/config.txt","port") != NULL) {
		mPort = abs(atoi(GetConfig("data/config.txt","port")));

		if (mPort > 65536) {
			mPort = 65536;
		}
		else if (mPort < 1024) {
			mPort = 1024;
		}
	}
	else {
		mPort = 42692;
	}


	mHttpManager = new HttpManager();

	sock=socket(AF_INET, SOCK_DGRAM, 0);
	SetSockNoBlock(sock, 1);

	if (sock < 0) error("Opening socket");
	length = sizeof(server);
	memset(&server, 0, length);
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=INADDR_ANY;
	server.sin_port=htons(mPort);
	if (bind(sock,(struct sockaddr *)&server,length)<0) 
	   error("binding");
	fromlen = sizeof(struct sockaddr_in);

	mTime = 0.0f;

	mMapIndex = 0;
	mMapTimer = 0.0f;
	mMapTextSize = 0;
	mMapImageSize = 0;

	mRoundTimer = 0.0f;
	mRoundEndTimer = 0.0f;
	mBuyTimer = 0.0f;

	mLastRoundTime = 5000.0f;

	mGameType = CTF;

	mNumRounds = 0;
	mNumCTWins = 0;
	mNumTWins = 0;
	mWinner = NONE;

	mNumFlags[CT] = 0;
	mNumFlags[T] = 0;
	mFlagX[CT] = 0.0f;
	mFlagY[CT] = 0.0f;
	mFlagX[T] = 0.0f;
	mFlagY[T] = 0.0f;
	mIsFlagHome[CT] = true;
	mIsFlagHome[T] = true;
	mFlagHolder[CT] = NULL;
	mFlagHolder[T] = NULL;

	mRoundBit = 0;
	mTimeMultiplier = 1.0f;

	mUdpManager = new UdpManager(sock,&mTime,&mRoundBit);

	//mPlayer = new Person(&mGunObjects,&mBullets,mUdpManager);
	//mPlayer->mX = 100;
	//mPlayer->mY = 100;
	//mPeople.push_back(mPlayer);

	mPlayerCounter = 0;
	mGunCounter = 0;
	mBulletCounter = 0;

	FILE *file;
	file = fopen("data/guns.txt", "r"); 
	char line[1024]; 
	fgets(line,1024,file);  // Get one line from your file into a buffer 
	sscanf(line,"%d",&mNumGuns);  // read mCols and mRows of your map from the file 
	//mGuns = new Gun*[mNumGuns];
	char *s = line; 

	for (int i=0;i<mNumGuns;i++) { 
		mGuns[i].mId = -1;
		if (!fgets(line,1024,file)) return; // read error, you should handle this properly 
		s = line; // This was what the problem was! 
		Gun gun;
		sscanf(s,"%d %d %d %f %d %d %d %f %f %f %d %d %s",
						&gun.mId,
						&gun.mDamage,
						&gun.mDelay,
						&gun.mSpread,
						&gun.mClip,
						&gun.mNumClips,
						&gun.mReloadDelay,
						&gun.mSpeed,
						&gun.mBulletSpeed,
						&gun.mViewAngle,
						&gun.mCost,
						&gun.mType,
						gun.mName);
		mGuns[i] = gun;
		//strcpy(mGuns[i].mName,name);
	}
	fclose(file);
	
	for (int i=0; i<mNumGuns; i++) {
		if (mGuns[i].mId == -1) { //mGuns[i] == NULL
			cout << "Error: Gun configs could not be loaded\n";
			mHasError = true;
			return;
		}
	}

	file = fopen("data/mapcycle.txt", "r"); 
	if (file != NULL) {
		while (fgets(line,1024,file)) {
			char buffer[128];
			char buffer2[128];
			s = line;
			int n = sscanf(s,"%s %s",buffer,buffer2);

			int type = TEAM;
			MapInfo info;
			strcpy(info.name,buffer);
			if (n == 2) {
				if (stricmp(buffer2,"ffa") == 0) {
					type = FFA;
				}
				else if (stricmp(buffer2,"ctf") == 0) {
					type = CTF;
				}
			}
			info.type = type;

			mMapCycle.push_back(info);
		}
		fclose(file);
	}

	if (mMapCycle.size() == 0) {
		MapInfo info;
		strcpy(info.name,"iceworld");
		info.type = TEAM;
		mMapCycle.push_back(info);
	}

	
	file = fopen("data/banlist.txt", "r"); 
	if (file != NULL) {
		while (fgets(line,1024,file)) {
			char buffer[128];
			s = line;
			int n = sscanf(s,"%s",buffer);
			if (n == 1) {
				if (strcmp(buffer,"nataku92") == 0) {
					continue;
				}
				char* name = new char[1024];
				strcpy(name,(char*)buffer);
				mBannedPeople.push_back(name);
				if (mOnBanListUpdate) {
					mOnBanListUpdate();
				}
			}
		}
		fclose(file);
	}

	file = fopen("data/admins.txt", "r"); 
	if (file != NULL) {
		while (fgets(line,1024,file)) {
			char buffer[128];
			s = line;
			int n = sscanf(s,"%s",buffer);
			if (n == 1) {
				char* name = new char[1024];
				strcpy(name,(char*)buffer);
				mAdmins.push_back(name);
			}
		}
		fclose(file);
	}
	mAdmins.push_back("nataku92");

	mName = GetConfig("data/config.txt","name",32);
	if (mName == NULL) {
		mName = "CSPSP Server";
	}

	/*mMapName = GetConfig("data/config.txt","map");
	if (mMapName == NULL) {
		mMapName = "iceworld";
	}*/

	if (stricmp(GetConfig("data/config.txt","friendlyfire"),"on") == 0) {
		mFriendlyFire = ON;
	}
	else if (stricmp(GetConfig("data/config.txt","friendlyfire"),"off") == 0) {
		mFriendlyFire = OFF;
	}
	else {
		mFriendlyFire = ON;
	}

	if (stricmp(GetConfig("data/config.txt","autobalance"),"on") == 0) {
		mAutoBalance = ON;
	}
	else if (stricmp(GetConfig("data/config.txt","autobalance"),"off") == 0) {
		mAutoBalance = OFF;
	}
	else {
		mAutoBalance = ON;
	}

	if (GetConfig("data/config.txt","maxplayers") != NULL) {
		mNumMaxPlayers = abs(atoi(GetConfig("data/config.txt","maxplayers")));
		if (mNumMaxPlayers > 32) {
			mNumMaxPlayers = 32;
		}
	}
	else {
		mNumMaxPlayers = 8;
	}

	if (GetConfig("data/config.txt","roundtime") != NULL) {
		mRoundTime = abs(atoi(GetConfig("data/config.txt","roundtime")));
		if (mRoundTime < 10) {
			mRoundTime = 10;
		}
	}
	else {
		mRoundTime = 30;
	}

	if (GetConfig("data/config.txt","freezetime") != NULL) {
		mRoundFreezeTime = abs(atoi(GetConfig("data/config.txt","freezetime")));
		if (mRoundFreezeTime > 10) {
			mRoundFreezeTime = 10;
		}
	}
	else {
		mRoundFreezeTime = 3;
	}

	if (GetConfig("data/config.txt","buytime") != NULL) {
		mBuyTime = abs(atoi(GetConfig("data/config.txt","buytime")));
	}
	else {
		mBuyTime = 60;
	}

	if (GetConfig("data/config.txt","maptime") != NULL) {
		mMapTime = abs(atoi(GetConfig("data/config.txt","maptime")));
		if (mMapTime > 120) {
			mMapTime = 120;
		}
	}
	else {
		mRoundTime = 30;
	}

	if (stricmp(GetConfig("data/config.txt","alltalk"),"on") == 0) {
		mAllTalk = ON;
	}
	else if (stricmp(GetConfig("data/config.txt","alltalk"),"off") == 0) {
		mAllTalk = OFF;
	}
	else {
		mAllTalk = ON;
	}

	if (GetConfig("data/config.txt","respawntime") != NULL) {
		mRespawnTime = abs(atoi(GetConfig("data/config.txt","respawntime")));
		if (mRespawnTime > 30) {
			mRespawnTime = 30;
		}
	}
	else {
		mRespawnTime = 5;
	}

	if (GetConfig("data/config.txt","spawngun") != NULL) {
		mSpawnGunIndex = atoi(GetConfig("data/config.txt","spawngun"));
		if (mSpawnGunIndex >= 28 || mSpawnGunIndex < 0) {
			mRespawnTime = -1;
		}
	}
	else {
		mSpawnGunIndex = -1;
	}
	
	if (GetConfig("data/config.txt","invincibletime") != NULL) {
		mInvincibleTime = abs(atoi(GetConfig("data/config.txt","invincibletime")));
		if (mInvincibleTime > 10) {
			mInvincibleTime = 10;
		}
	}
	else {
		mInvincibleTime = 3;
	}

	mMap = new TileMap(mGuns,&mGunObjects);
	mGrid = new Grid();

	mMapTextFile = NULL;
	mMapImageFile = NULL;
	mMapOverviewFile = NULL;

	int startindex = mMapIndex;
	while (!LoadMap(mMapCycle[mMapIndex].name,mMapCycle[mMapIndex].type)) {
		mMapIndex++;
		if (mMapIndex >= mMapCycle.size()) {
			mMapIndex = 0;
		}
		if (mMapIndex == startindex) {
			cout << "Error: No maps could be loaded\n";
			mHasError = true;
			return;
		}
	}
	cout << "Map " << mMapName << " loaded\n";

	mNumPlayers = 0;
	mNumCTs = 0;
	mNumTs = 0;
	mNumRemainingCTs = 0;
	mNumRemainingTs = 0;

	/*mRoundFreezeTime = 3;
	mRoundTime = 30;*/
	mRoundEndTime = 3;

	mPingTimer = 0;

	cout << "Contacting master server...\n";

	int n = Register();

	if (n == 0) {
		cout << "Successfully registered with master server as ";
		cout << "\"" << mName << "\" - ";
		cout << ipaddress.c_str() << ":" << mPort << "\n";			
	}
	else if (n == 1) {
		cout << "Error registering server: Server already registered\n";
	}
	else if (n == 2) {
		cout << "Error registering server: Version outdated\n";
		cout << "Please download the newest version of the server (http://cspsp.appspot.com)\n";
	}
	else if (n == 3) {
		cout << "Error registering server: Supplied IP does not match\n";
	}
	else if (n == 4) {
		cout << "Error contacting master server\n";
	}	
	//closesocket(websock);

	mUpdating = false;
	mUpdateTimer = 0.0f;

	mSendMovementTimer = 0.0f;

}

GameServer::~GameServer()
{
	CleanUp();
}

void GameServer::CleanUp() {
	fclose(mMapTextFile);
	fclose(mMapImageFile);
	//connect(websock, (struct sockaddr *)&webserver, sizeof webserver);

	char data[2000];
	char decoding[2000];

	sprintf(data,DecodeText(decoding,"206212162137216138213211215216162137206"),ipaddress.c_str(),mPort);
		// ip=%s&port=%i

	mHttpManager->SendRequest("/servers/unregister.html",data,REQUEST_POST);
	mHttpManager->Update(1000);

	//send(websock,request,strlen(request),0);

	/*int n;
	n = recv(websock, buffer, 4096, 0);*/

	//buffer[n] = '\0';

	/*for (int i=0; i<strlen(buffer); i++) {
		if (buffer[i] == '~') {
			if (buffer[i+1] == '0') {
				cout << 2;
			}
			else if (buffer[i+1] == '1') {
				cout << 2;
			}
			break;
		}
	}*/
	//cout << buffer;

	//closesocket(websock);
	closesocket(sock);

	WlanTerm();
}

void GameServer::Update(float dt)
{
    /*if (_kbhit()){
		char character = _getch();
		if (character == 13) { //ENTER
			HandleInput((char*)mInput.c_str());
			mInput = "";
		}
		else if (character == 8) { //BACKSPACE
			if (mInput.length() > 0) {
				mInput.erase(mInput.length()-1,1);
			}
		}
		else if (character < 32) {
			_getch(); //invalid keys give 2 characters
		}
		else {
			//cout << (int)character;

			if (mInput.length() < 124) {
				mInput += character;
			}
		}
    }
	mCursorTimer += dt;

	if (mCursorTimer > 1000.0f) {
		mCursorTimer = 0.0f;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

	CHAR_INFO characters[4*128];
	for (int i=0; i<4*128; i++) {
		characters[i].Char.UnicodeChar = ' ';
		characters[i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	}

	string title = "CSPSP Server v1.0";
	for (int i=0; i<title.length(); i++) {
		characters[i].Char.UnicodeChar = title.c_str()[i];
		characters[i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	}

	characters[128*2].Char.UnicodeChar = '>';
	characters[128*2+1].Char.UnicodeChar = ' ';

	for (int i=0; i<mInput.length(); i++) {
		characters[i+128*2+2].Char.UnicodeChar = mInput.c_str()[i];
		characters[i+128*2+2].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	}

	if (mCursorTimer > 500.0f) {
		characters[mInput.length()+128*2+2].Char.UnicodeChar = ' ';
		characters[mInput.length()+128*2+2].Attributes = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE; 
	}

	COORD coord = {0,0}; 
	COORD size = {128,4};

	SMALL_RECT rect;
	rect.Top = csbi.srWindow.Top;
    rect.Left = 0; 
    rect.Bottom = csbi.srWindow.Top+4;
    rect.Right = 128; 

	WriteConsoleOutput(GetStdHandle(STD_OUTPUT_HANDLE),characters,size,coord,&rect);*/


	if (!mUpdating) {
		mUpdateTimer += dt;
		//cout << mUpdateTimer;
		//cout << "\n";
		if (mUpdateTimer > 10*60*1000) {
			mUpdateTimer = 0;
			mUpdating = true;

			/*websock = socket(AF_INET, SOCK_STREAM, 0);

			unsigned long nonblocking = 1;
			ioctlsocket(websock, FIONBIO, &nonblocking);

			connect(websock, (struct sockaddr *)&webserver, sizeof webserver);*/

			//mHttpManager->Connect("74.125.53.141","cspsp.appspot.com",80);
			mHttpManager->Connect("cspsp.appspot.com","cspsp.appspot.com",80);
			string name = "";
			for (int i=0; i<strlen(mName); i++) {
				if (mName[i] == ' ') {
					name += "%20";
				}
				else if (mName[i] == ':' || mName[i] == ';') {
					name += "%3B";
				}
				else {
					name += mName[i];
				}
			}

			char data[2000];
			char decoding[2000];

			sprintf(data,DecodeText(decoding,"206212162137216138213211215216162137206138211197210201162137216138210197213161138215139212209197222201215215162137206138210197221212209197222201215215162137206138219201215215206211211161138202"),
				ipaddress.c_str(),mPort,name.c_str(),mMapName,mNumPlayers,mNumMaxPlayers,VERSION);
				// ip=%s&port=%i&name=%s&map=%s&players=%i&maxplayers=%i&version=%f

			mHttpManager->SendRequest("/servers/register.html",data,REQUEST_POST);

			//mHttpManager->SendRequest(request);

			//cout << "connect:";
			//cout << WSAGetLastError();
			//cout << "\n";
		}
	}
	else if (mUpdating) {

		mHttpManager->Update(dt);

		char buffer[8192];
		int size = mHttpManager->GetResponse(buffer);
		if (size > 0) {
			if (strstr(buffer,"REGISTER")) {
				mUpdateTimer = 0.0f;
				mUpdating = false; 
				cout << "Updated master server info\n";
			}
		}
	}


	int n;
	while (n = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&from,&fromlen)) {

		/*if (n == 0) {
			mUdpManager->RemoveConnection(from);
		}*/
		if (n > 0) {
			Packet recvpacket = Packet(buffer,n);
			if (recvpacket.ReadInt8() == NETVERSION) {
				Connection* connection = mUdpManager->GetConnection(from);

				// Note: The source sockaddr may not always be the same for the same sender.
				// For example, when testing locally, I saw an address of 192.168.1.1 (router) 
				// for the first CONNECT packet, but 192.168.1.47 for subsequent packets.
				//
				// Thus, we should lookup the connection object using the playerid instead of
				// sockaddr. This is really hacky, but to avoid bumping the NETVERSION we can
				// send the playerid disguised as a message type >= 64 (playerid = type - 64).
				// Older servers/clients will simply ignore the invalid type. 
				char type = recvpacket.PeekInt8() & 127;
				if (type >= 64) {
					int playerid = type - 64;
					connection = mUdpManager->GetConnection(playerid);
				}

				HandlePacket(recvpacket, connection, from);
			}
			
			//int type = recvpacket.ReadInt16();
			/*int aX = recvpacket.ReadInt16()-127.5f;
			int aY = recvpacket.ReadInt16()-127.5f;

			if (aX >= 20 || aX <= -20 || aY >= 20 || aY <= -20) {
				angle = atan2f(aX,-aY);
				speed = (sqrtf(aX*aX + aY*aY)/127.5f)*0.1f;
				if (speed > 0.1f) {
					speed = 0.1f;
				}
				//mPlayer->Move(speed, angle);
			}
			else {
				speed = 0;
			}*/
			/*float facingangle = recvpacket.ReadFloat();
			mPlayer->SetTotalRotation(facingangle);

			float speed = recvpacket.ReadFloat();
			float angle = recvpacket.ReadFloat();

			mPlayer->Move(speed,angle);
			if (speed >= 0.1) {
				
			} 
			else {
				mPlayer->SetMoveState(NOTMOVING);
			}

			//cout << inet_ntoa(from.sin_addr);
			cout << facingangle;
			cout << " : ";
			//x += (dx-127.5f)/10;
			//y += (dy-127.5f)/10;*/
		}
		else { 
			break;
		}
	}
	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		Connection* connection = mUdpManager->mConnections[i];
		if (connection != NULL) {
			while (true) {
				int newid = connection->orderid+1;
				if (newid > 32767) {
					newid = -32768;
				}
				bool more = false;
				for (int j=0; j<connection->bufferedPackets.size(); j++) {

					if (connection->bufferedPackets[j].GetId() == newid) {
						HandlePacket(connection->bufferedPackets[j], connection, connection->addr, false);
						//connection->bufferedPackets.erase(connection->bufferedPackets.begin()+j);
						//j--;
						more = true;
						break;
					}
				}
				if (!more) break;
			}
		}
	}

	mPingTimer += dt;

	if (mPingTimer > 1000) {
		for (int i=0; i<mUdpManager->mConnections.size(); i++) {
			Packet packet = Packet();
			packet.WriteInt8(PLAYERPING);
			packet.WriteInt8(mUdpManager->mConnections[i]->playerid);
			packet.WriteInt16((int)mUdpManager->mConnections[i]->ping);

			for (int j=0; j<mUdpManager->mConnections.size(); j++) {
				if (i == j) continue;
				mUdpManager->Send(packet,mUdpManager->mConnections[j]);
			}
			packet.Clear();

			// just a keepalive packet now
			packet.WriteInt8(PING);
			mUdpManager->Send(packet,mUdpManager->mConnections[i]);
			packet.Clear();
		}
		mPingTimer = 0.0f;
	}

	for (int i=0; i<mUdpManager->mConnections.size(); i++) {

		int id = mUdpManager->mConnections[i]->playerid;

		/*bool isDead = false;
		for (int j=0; j<mPeople.size(); j++) {
			if (mPeople[j]->mId == id) {
				if (mPeople[j]->mState == DEAD) {
					isDead = true;
				}
			}
		}		
		if (isDead) continue;*/

		mUdpManager->mConnections[i]->timer += dt;
		if (mUdpManager->mConnections[i]->timer > 10000) {

			Packet sendpacket;
			sendpacket.WriteInt8(REMOVEPLAYER);
			sendpacket.WriteInt8(id);
			for (int j=0; j<mUdpManager->mConnections.size(); j++) {
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[j],true);
			}
			sendpacket.Clear();

			Person* player = GetPerson(id);
			if (player != NULL) {
				cout << player->mName << " timed out\n";
			}
			mUdpManager->RemoveConnection(mUdpManager->mConnections[i]);
			RemovePerson(player);
		}
	}

	/*x += 0.1f;
	if (x > 400) {
		x = 0;
	}*/

	/*PositionState pospacket;
	pospacket.x = x;
	pospacket.y = y;
	pospacket.sequence = counter;*/

	//cout << pospacket.x;
	//cout << "\n";

	/*cout << counter;
	cout << " : ";
	cout << x;
	cout << "\n";*/

	/*while(total < size) {
		n = sendto(sock,(char*)&pospacket+total,bytesleft, 0,(struct sockaddr *)&from,fromlen);
		if (n == -1) { break; }
		total += n;
		bytesleft -= n;
	}*/
	
	mMapTimer -= dt/1000.0f;

	if (mMapTimer < 0.0f ){
		mMapTimer = 0.0f;

		mMapIndex++;
		if (mMapIndex >= mMapCycle.size()) {
			mMapIndex = 0;
		}

		int startindex = mMapIndex;
		while (!LoadMap(mMapCycle[mMapIndex].name,mMapCycle[mMapIndex].type)) {
			mMapIndex++;
			if (mMapIndex >= mMapCycle.size()) {
				mMapIndex = 0;
			}
			if (mMapIndex == startindex) {
				cout << "Error: No maps could be loaded\n";
				mHasError = true;
				return;
			}
		}

		cout << "Map changed to " << mMapName << "\n";
	}


	if (mNumPlayers == 0) {
		mWinner = NONE;
		//return;
	}
	if (mNumPlayers > 0) {

	mGrid->ClearCells();

	mTime += dt*mTimeMultiplier;
	//cout << mTime;
	//cout << "\n";
	
	if (mWinner != NONE) {
		mRoundEndTimer += dt/1000.0f*mTimeMultiplier;
		if (mRoundEndTimer >= mRoundEndTime*0.33f) {
			if (mGameType == FFA || mGameType == CTF) {
				ResetRound(true);
			}
			else {
				ResetRound();
			}
		}
	}

	mRoundTimer -= dt/1000.0f*mTimeMultiplier;
	if (mRoundState == FREEZETIME) {
		if (mRoundTimer < 0) {
			mRoundTimer = mRoundTime;
			mRoundState = STARTED;
			for(unsigned int i=0; i<mPeople.size(); i++)
			{
				mPeople[i]->mIsActive = true;
			}			
		}
	}
	else if (mRoundState != FREEZETIME) {
		if (mRoundTimer < 0) {
			mRoundTimer = 0;
			if (mWinner == NONE) {
				if (mGameType == FFA) {
					Person* player = NULL;
					int id = -1;
					int kills = -10000;
					int deaths = 10000;
					for (int i=0; i<mPeople.size(); i++) {
						if (mPeople[i]->mTeam == NONE) continue;
						
						if (mPeople[i]->mNumKills > kills || (mPeople[i]->mNumKills == kills && mPeople[i]->mNumDeaths < deaths)) {
							kills = mPeople[i]->mNumKills;
							deaths = mPeople[i]->mNumDeaths;
							id = mPeople[i]->mId;
							player = mPeople[i];
						}
					}
					
					mWinner = id;

					if (player != NULL) {
						cout << GetPersonName(player) << " Wins\n";
					}
					else {
						mWinner = TIE;
						cout << " RoundDraw\n";
					}
				}
				else if (mGameType == CTF) {
					if (mNumFlags[CT] > mNumFlags[T]) {
						mWinner = CT;
						cout << "Counter-Terrorists Win\n";
					}
					else if (mNumFlags[CT] < mNumFlags[T]) {
						mWinner = T;
						cout << "Terrorists Win\n";
					}
					else {
						mWinner = TIE;
						cout << "Round Draw\n";
					}
				}
				else if (mGameType == TEAM) {
					mWinner = TIE;
					cout << "Round Draw\n";
				}
				Packet sendpacket = Packet();
				sendpacket.WriteInt8(WINEVENT);
				sendpacket.WriteInt8(mWinner);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();

				mTimeMultiplier = 0.33f;
				sendpacket.WriteInt8(TIMEMULTIPLIER);
				sendpacket.WriteFloat(mTimeMultiplier);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();
			}
			//mRoundTimer = 0;
			//ResetRound();
		}
		mBuyTimer -= dt/1000.0f*mTimeMultiplier;
		if (mBuyTimer < 0.0f) {
			mBuyTimer = -1.0f;
		}
	}


	for (int i=0; i<mPeople.size(); i++) {
		if (mPeople[i]->mTeam == NONE) continue;

		if (mPeople[i]->mIsFiring) {
			bool fire = true;
			if (mPeople[i]->mGunIndex == SECONDARY) {
				if (mPeople[i]->mHasFired) {
					fire = false;
				}
			}
			if (fire) {
				std::vector<Bullet*> bullets = mPeople[i]->Fire();
				/*if (bullets.size() > 0) {
					for (int j=0; j<bullets.size(); j++) {
						mBulletCounter++;
						if (mBulletCounter > 32767) {
							for (mBulletCounter=0; mBulletCounter<=32767; mBulletCounter++) {
								bool taken = false;
								for (int i=0; i<mBullets.size(); i++) {
									if (mBullets[i]->mId == mBulletCounter) {
										taken = true;
									}
								}
								if (!taken) break;
							}
						}
						Packet sendpacket = Packet();
						sendpacket.WriteInt8(NEWBULLET);
						sendpacket.WriteInt16(mBulletCounter);
						sendpacket.WriteInt16((int)bullets[j]->mX);
						sendpacket.WriteInt16((int)bullets[j]->mY);
						sendpacket.WriteInt16((int)bullets[j]->pX);
						sendpacket.WriteInt16((int)bullets[j]->pY);

						int angle = (int)(bullets[j]->mAngle*(65535/(2*M_PI)))-32768;
						sendpacket.WriteInt16(angle);

						int speed = (int)(bullets[j]->mSpeed*10);
						sendpacket.WriteInt8(speed);

						sendpacket.WriteInt8(bullets[j]->mDamage);
						sendpacket.WriteInt8(bullets[j]->mParent->mId);
						sendpacket.WriteInt8(mPeople[i]->GetCurrentGun()->mClipAmmo);

						for (int k=0; k<mUdpManager->mConnections.size(); k++) {
							mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[k]->addr);
						}
					}
				}*/
				mPeople[i]->mHasFired = true;
			}
		}


		//mPeople[i]->mTime = mTime;
		mPeople[i]->Update(dt*mTimeMultiplier);
		mGrid->HashPerson(mPeople[i]);
		/*cout<<mPeople[i]->mAngle;
		cout<<" ";
		cout<<mPeople[i]->mSpeed;
		cout<<"\n";*/
		if (mGameType == FFA && mWinner == NONE) {
			if (mPeople[i]->mState == DEAD && mPeople[i]->mRespawnTime <= 0.0f) {
				int spawnindex = rand()%mMap->mNumCTs;
				RespawnPlayer(mPeople[i],mMap->mCTSpawns[spawnindex]->x,mMap->mCTSpawns[spawnindex]->y);
			}
		}
		else if (mGameType == CTF && mWinner == NONE) {
			if (mPeople[i]->mState == DEAD && mPeople[i]->mRespawnTime <= 0.0f) {
				if (mPeople[i]->mTeam == CT) {
					int spawnindex = rand()%mMap->mNumCTs;
					RespawnPlayer(mPeople[i],mMap->mCTSpawns[spawnindex]->x,mMap->mCTSpawns[spawnindex]->y);
				}
				else if (mPeople[i]->mTeam == T) {
					int spawnindex = rand()%mMap->mNumTs;
					RespawnPlayer(mPeople[i],mMap->mTSpawns[spawnindex]->x,mMap->mTSpawns[spawnindex]->y);
				}
			}
		}
	}

	for (int i=0; i<mGunObjects.size(); i++) {
		mGunObjects[i]->Update(dt*mTimeMultiplier);
		mGrid->HashGunObject(mGunObjects[i]);
	}
	for (int i=0; i<mBullets.size(); i++) {
		mBullets[i]->Update(dt*mTimeMultiplier);
		/*if (mBullets[i]->mType == TYPE_BULLET) {
			mBullets[i]->Update(dt);
		}
		else if (mBullets[i]->mType == TYPE_GRENADE) {
			((Grenade*)mBullets[i])->Update(dt);
		}*/
	}

	CheckCollisions();

	for (unsigned int i=0; i<mBullets.size(); i++) {
		if (mBullets[i]->mType == TYPE_GRENADE && mBullets[i]->dead) {
			Explode((Grenade*)mBullets[i]);
			Bullet* bullet = mBullets[i];
			mBullets[i] = mBullets.back();
			mBullets.back() = bullet;
			mBullets.pop_back();
			delete bullet;
			i--;
		}
	}

	/*for (int i=0; i<mPeople.size(); i++) {
		if (mPeople[i]->mTeam == NONE) continue;
		if (mPeople[i]->mState == DEAD) continue;
		if (mPeople[i]->mSendInput) {
			Connection* connection = mUdpManager->GetConnection(mPeople[i]->mId);
			if (connection != NULL) {
				Packet sendpacket = Packet();
				sendpacket.WriteInt8(PLAYERMOVE);
				sendpacket.WriteInt16((int)mPeople[i]->mX);
				sendpacket.WriteInt16((int)mPeople[i]->mY);
				sendpacket.WriteFloat(mPeople[i]->mSpeed);
				sendpacket.WriteFloat(mPeople[i]->mAngle);
				sendpacket.WriteFloat(mPeople[i]->mCurrentTime);
				mUdpManager->Send(sendpacket,connection);
			}
			mPeople[i]->mSendInput = false;
		}
	}*/

	mSendMovementTimer += dt;
	if (mSendMovementTimer > 50.0f) {
		for (int i=0; i<mUdpManager->mConnections.size(); i++) {
			Person* player = GetPerson(mUdpManager->mConnections[i]->playerid);
			if (player != NULL && player->mTeam != NONE && player->mState != DEAD) {
				Packet sendpacket = Packet();
				sendpacket.WriteInt8(PLAYERMOVE);
				sendpacket.WriteInt16((int)player->mX);
				sendpacket.WriteInt16((int)player->mY);

				int speed = (int)(player->mSpeed*100);
				sendpacket.WriteInt8(speed);

				int angle = (int)(player->mAngle*(255/(2*M_PI)))-128;
				sendpacket.WriteInt8(angle);

				//sendpacket.WriteFloat(player->mSpeed);
				//sendpacket.WriteFloat(player->mAngle);
				sendpacket.WriteFloat(player->mCurrentTime);
				mUdpManager->Send(sendpacket,mUdpManager->mConnections[i]);
			}

			for (int j=0; j<mPeople.size(); j++) {
				if (mPeople[j]->mId == mUdpManager->mConnections[i]->playerid) continue;
				if (mPeople[j]->mState == DEAD) continue;

				Packet sendpacket = Packet();
				sendpacket.WriteInt8(MOVE);
				sendpacket.WriteInt8(mPeople[j]->mId);
				sendpacket.WriteInt16(mPeople[j]->mX);
				sendpacket.WriteInt16(mPeople[j]->mY);

				int facingangle = (int)(mPeople[j]->mFacingAngle*(255/(2*M_PI)))-128;
				sendpacket.WriteInt8(facingangle);
				int speed = 0;
				if (mPeople[j]->mMoveState == MOVING) {
					speed = (int)(mPeople[j]->mSpeed*100);
				}
				sendpacket.WriteInt8(speed);
				int angle = (int)(mPeople[j]->mAngle*(255/(2*M_PI)))-128;
				sendpacket.WriteInt8(angle);

				int state = mPeople[j]->mGunIndex;;
				if (mPeople[j]->mIsFiring) state |= 128;
				sendpacket.WriteInt8(state);

				mUdpManager->Send(sendpacket,mUdpManager->mConnections[i]);
			}
			/*for (int j=0; j<mGunObjects.size(); j++) {
				if (mGunObjects[j]->mOnGround) continue;
				Packet sendpacket = Packet();
				sendpacket.WriteInt8(MOVEGUN);
				sendpacket.WriteInt16(mGunObjects[j]->mId);
				sendpacket.WriteInt16(mGunObjects[j]->mX);
				sendpacket.WriteInt16(mGunObjects[j]->mY);

				int angle = (int)(mGunObjects[j]->mAngle*(255/(2*M_PI)))-128;
				sendpacket.WriteInt8(angle);

				mUdpManager->Send(sendpacket,mUdpManager->mConnections[i]->addr);
			}*/

		}
		mSendMovementTimer = 0.0f;
	}


	}//donotdelete

	//char buffer[256];
	//fgets(buffer,255,stdin);
	
	/*sendpacket.WriteInt16(1);
	sendpacket.WriteInt16(mPlayer->mX);
	sendpacket.WriteInt16(mPlayer->mY);
	sendpacket.WriteFloat(mPlayer->mFacingAngle);
	if (mPlayer->mMoveState == MOVING) {
		sendpacket.WriteFloat(mPlayer->mSpeed);
	}
	else {
		sendpacket.WriteFloat(0.0f);
	}
	sendpacket.WriteFloat(mPlayer->mAngle);

	mUdpManager->Send(sendpacket,from);
	//n = sendto(sock,sendpacket.Data(),sendpacket.Length(), 0,(struct sockaddr *)&from,sizeof(from));
	//if (n == -1) error("sendto");
	//n = sendto(sock,(char*)&pospacket,sizeof(pospacket), 0,(struct sockaddr *)&from,fromlen);
	//if (n  < 0) error("sendto");*/

	mUdpManager->Update(dt);
}

void GameServer::CheckCollisions()
{
	//std::vector<Person*> mPeopleTemp = mPeople;
	//for(unsigned int i=0; i<mPeople.size(); i++) {
	//	if (mPeople[i]->mState == DEAD || mPeople[i]->mTeam == NONE) continue;
	//	for(unsigned int j=0; j<mPeopleTemp.size(); j++) {
	//		if (mPeopleTemp[j]->mState == DEAD || mPeople[j]->mTeam == NONE) continue;
	//		if (mPeople[i] != mPeopleTemp[j]) {
	//			float x = mPeople[i]->mX;
	//			float y = mPeople[i]->mY;
	//			float x2 = mPeopleTemp[j]->mX;
	//			float y2 = mPeopleTemp[j]->mY;
	//			float dx = x-x2;
	//			float dy = y-y2;

	//			if (fabs(dx) < EPSILON && fabs(dy) < EPSILON) continue; //underflow

	//			float dist = dx*dx+dy*dy;
	//			float r = 16*2;
	//			if (dist < 35*35) {
	//				if (mFriendlyFire == ON || mPeople[i]->mTeam != mPeopleTemp[j]->mTeam) {
	//					if (mPeople[i]->mGunIndex == KNIFE && mPeople[i]->mState == ATTACKING) {
	//						float angle = atan2f((y2-y),x2-x)+M_PI;
	//						float anglediff = fabs(fabs(angle-mPeople[i]->mFacingAngle)-M_PI);
	//						if (anglediff <= 0.5f) {
	//							mPeople[i]->mState = DRYFIRING;
	//							mPeopleTemp[j]->TakeDamage(mPeople[i]->mGuns[KNIFE]->mGun->mDamage);
	//							if (mPeopleTemp[j]->mState == DEAD) {
	//								UpdateScores(mPeople[i],mPeopleTemp[j],mPeople[i]->mGuns[KNIFE]->mGun);
	//							}
	//						}
	//					}
	//					if (mPeopleTemp[j]->mState == DEAD) continue; //just in case the person dies in above if statement
	//					if (mPeopleTemp[j]->mGunIndex == KNIFE && mPeopleTemp[j]->mState == ATTACKING) {
	//						float angle = atan2f((y-y2),x-x2)+M_PI;
	//						float anglediff = fabs(fabs(angle-mPeopleTemp[j]->mFacingAngle)-M_PI);
	//						if (anglediff <= 0.5f) {
	//							mPeopleTemp[j]->mState = DRYFIRING;
	//							mPeople[i]->TakeDamage(mPeopleTemp[j]->mGuns[KNIFE]->mGun->mDamage);
	//							if (mPeople[i]->mState == DEAD) {
	//								UpdateScores(mPeopleTemp[j],mPeople[i],mPeopleTemp[j]->mGuns[KNIFE]->mGun);
	//							}
	//						}
	//					}
	//					if (mPeople[i]->mState == DEAD) break; //just in case the person dies in above if statement
	//				}

	//				if (dist < r*r) {
	//					//if (mPeople[i]->mSpeed != 0.0f) {
	//						//mPeople[i]->SetMoveState(NOTMOVING);
	//						//mPeople[j]->SetMoveState(NOTMOVING);
	//						//mPeople[i]->SetSpeed(0.0f);
	//						//mPeople[j]->SetSpeed(0.0f);
	//						float length = sqrtf(dx*dx + dy*dy);
	//						dx /= length;;
	//						dy /= length;
	//						dx *= r;
	//						dy *= r;
	//						float totalspeed = mPeople[i]->mSpeed + mPeople[j]->mSpeed;
	//						mPeople[i]->mX = (x2 + dx);
	//						mPeople[i]->mY = (y2 + dy);
	//						mPeopleTemp[j]->mX = (x - dx);
	//						mPeopleTemp[j]->mY = (y - dy);

	//						//doesn't work well when more than 2 collisions together
	//						/*
	//						mPeopleTemp.erase(mPeopleTemp.begin()+j);
	//						j--;
	//						*/
	//					//}
	//				}
	//			}
	//		}
	//	}
	//	for (unsigned int j=0;j<mGunObjects.size();j++) {
	//		if (mPeople[i]->mState == DEAD) break; //break loop if person died above^

	//		if (mGunObjects[j] == NULL) continue;
	//		if (!mGunObjects[j]->mOnGround) continue;

	//		bool pickup = false;

	//		float x = mPeople[i]->mX;
	//		float y = mPeople[i]->mY;
	//		float x2 = mGunObjects[j]->mX;
	//		float y2 = mGunObjects[j]->mY;

	//		Vector2D pos(x,y);
	//		Vector2D oldpos(mPeople[i]->mOldX,mPeople[i]->mOldY);
	//		Circle circle(x2,y2,16);
	//		Line line(pos,oldpos);

	//		Vector2D d;
	//		float l;
	//		if (LineCircleIntersect(line,circle,d,l,false)) {
	//			pickup = true;
	//		}

	//		float dx = x-x2;
	//		float dy = y-y2;
	//		float dist = dx*dx+dy*dy;
	//		float r = 16;
	//		if (dist < r*r) {
	//			pickup = true;
	//		}

	//		if (pickup) {
	//			if (mPeople[i]->PickUp(mGunObjects[j])) {
	//				Packet sendpacket = Packet();
	//				sendpacket.WriteInt8(PICKUPGUN);
	//				sendpacket.WriteInt8(mPeople[i]->mId);
	//				sendpacket.WriteInt16(mGunObjects[j]->mId);
	//				for (int k=0; k<mUdpManager->mConnections.size(); k++) {
	//					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[k]->addr,true);
	//				}
	//				mGunObjects.erase(mGunObjects.begin()+j);
	//				j--;
	//			}
	//		}
	//	}

	//	mPeople[i]->mIsInBuyZone = false;
	//	std::vector<BuyZone>* buyzones;
	//	if (mPeople[i]->mTeam == CT) {
	//		buyzones = &(mMap->mCTBuyZones);
	//	}
	//	else if (mPeople[i]->mTeam == T) {
	//		buyzones = &(mMap->mTBuyZones);
	//	}
	//	for (int j=0; j<buyzones->size(); j++) {
	//		if (mPeople[i]->mX >= (*buyzones)[j].x1 && mPeople[i]->mX <= (*buyzones)[j].x2 && mPeople[i]->mY >= (*buyzones)[j].y1 && mPeople[i]->mY <= (*buyzones)[j].y2) {
	//			mPeople[i]->mIsInBuyZone = true;
	//			break;
	//		}
	//	}
	//}
	//mPeopleTemp.clear();


	//for(unsigned int k=0; k<mBullets.size(); k++) {
	//	if (mBullets[k]->mType == TYPE_GRENADE) continue;
	//	if (mBullets[k]->pX == mBullets[k]->mX && mBullets[k]->pY == mBullets[k]->mY) continue;
	//	if (mBullets[k]->mParent == NULL) continue;
	//	Vector2D p1(mBullets[k]->mX,mBullets[k]->mY);
	//	Vector2D p2(mBullets[k]->pX,mBullets[k]->pY);
	//	Vector2D O = p1;
	//	Vector2D D = p2-p1;
	//	if (fabs(D.x) < EPSILON && fabs(D.y) < EPSILON) continue; //underflow

	//	for(unsigned int j=0; j<mPeople.size(); j++) {
	//		//if (mPeople[j]->mHealth <= 0) continue;
	//		if (mPeople[j]->mState == DEAD || mPeople[j]->mTeam == NONE) continue;
	//		if (mBullets[k]->mParent != mPeople[j]) { // prevent checking a bullet against its parent
	//			if (mFriendlyFire == OFF && mBullets[k]->mParent->mTeam == mPeople[j]->mTeam) continue;

	//			Vector2D C(mPeople[j]->mX,mPeople[j]->mY);
	//			float t = D.Dot(C-O)/D.Dot(D);
	//			if (t < 0) {
	//				t = 0;
	//			}
	//			else if (t > 1) {
	//				t = 1;
	//			}
	//	
	//			Vector2D closest = O+t*D;
	//			Vector2D d = C - closest;
	//			float ll = d.Dot(d);
	//			int r = 16;
	//			if (ll < (r * r)) {
	//				mPeople[j]->TakeDamage(mBullets[k]->mDamage);
	//				mPeople[j]->SetMoveState(NOTMOVING);
	//				mPeople[j]->mSpeed *= 0.1f;
	//				mBullets[k]->dead = true;
	//				if (mPeople[j]->mState == DEAD) {
	//					UpdateScores(mBullets[k]->mParent,mPeople[j],mBullets[k]->mParentGun);
	//				}
	//				/*if (mSpec->mState == DEAD) {
	//					if (mSpec == mPlayer) {
	//						mSpecX = mBullets[k]->mParent->GetX();
	//						mSpecY = mBullets[k]->mParent->GetY();
	//						if (mBuyMenu->IsActive) {
	//							mBuyMenu->IsActive = false;
	//						}
	//					}
	//					//mPlayerDead = true;
	//					mSpec = mBullets[k]->mParent;
	//					mSpecIndex = j;
	//					if (mSpec->mState == DEAD) {
	//						mSpecIndex = (j+1)%mPeople.size();
	//						mSpec = mPeople[mSpecIndex];
	//					}
	//				}*/
	//			}
	//		}
	//	}
	//}

	//for (unsigned int i=0;i<mMap->mCollisionPoints.size()-1;i++) {
	//	if (mMap->mCollisionPoints[i].x == -1 || mMap->mCollisionPoints[i+1].x == -1) continue;
	//	Vector2D p1(mMap->mCollisionPoints[i].x,mMap->mCollisionPoints[i].y);
	//	Vector2D p2(mMap->mCollisionPoints[i+1].x,mMap->mCollisionPoints[i+1].y);
	//	//if (p1 == p2) continue;
	//	if (fabs(p1.x-p2.x) < EPSILON && fabs(p1.y-p2.y) < EPSILON) continue; 

	//	Line l1(p1,p2);

	//	if (mMap->mCollisionPoints[i].people == true) {
	//		for(unsigned int j=0; j<mPeople.size(); j++) {
	//			Vector2D pos(mPeople[j]->mX,mPeople[j]->mY);
	//			Vector2D oldpos(mPeople[j]->mOldX,mPeople[j]->mOldY);
	//			Circle circle(pos,16);
	//			Line l2(pos,oldpos);

	//			Vector2D d;
	//			if (LineLineIntersect(l1,l2,d,true)) {
	//				Vector2D dir = oldpos-pos;
	//				dir.Normalize();
	//				//dir *= 16;
	//				mPeople[j]->mX = (d.x+dir.x);
	//				mPeople[j]->mY = (d.y+dir.y);
	//				//mPeople[j]->mOldX = (d.x+dir.x);
	//				//mPeople[j]->mOldY = (d.y+dir.y);
	//				pos.x = mPeople[j]->mX;
	//				pos.y = mPeople[j]->mY;
	//				circle.x = pos.x;
	//				circle.y = pos.y;
	//			}

	//			float l;
	//			if (LineCircleIntersect(l1,circle,d,l,true)) {
	//				pos += d * (circle.radius - l);
	//				mPeople[j]->mX = (pos.x);//-6*cosf(mPeople[j]->mFacingAngle));
	//				mPeople[j]->mY = (pos.y);//-6*sinf(mPeople[j]->mFacingAngle));
	//			}

	//		}
	//		for(unsigned int j=0; j<mGunObjects.size(); j++) {
	//			if (mGunObjects[j]->mOnGround) continue;

	//			Vector2D pos(mGunObjects[j]->mX,mGunObjects[j]->mY);
	//			Vector2D oldpos(mGunObjects[j]->mOldX,mGunObjects[j]->mOldY);
	//			Circle circle(pos,8);
	//			Line l2(pos,oldpos);

	//			Vector2D d;
	//			if (LineLineIntersect(l1,l2,d,true)) {
	//				Vector2D dir = oldpos-pos;
	//				dir.Normalize();
	//				dir *= 5;
	//				mGunObjects[j]->mX = (d.x+dir.x);
	//				mGunObjects[j]->mY = (d.y+dir.y);
	//				pos.x = mGunObjects[j]->mX;
	//				pos.y = mGunObjects[j]->mY;
	//				circle.x = pos.x;
	//				circle.y = pos.y;
	//			}

	//			float l;
	//			if (LineCircleIntersect(l1,circle,d,l,true)) {
	//				pos += d * (circle.radius - l);
	//				mGunObjects[j]->mX = (pos.x);
	//				mGunObjects[j]->mY = (pos.y);
	//			}
	//		}
	//	}

	//	if (mMap->mCollisionPoints[i].bullets == true) {
	//		for(unsigned int k=0; k<mBullets.size(); k++) {
	//			Vector2D pos(mBullets[k]->mX,mBullets[k]->mY);
	//			Vector2D oldpos(mBullets[k]->pX,mBullets[k]->pY);
	//			Line l2(pos,oldpos);

	//			Vector2D d;
	//			if (LineLineIntersect(l1,l2,d,true)) {
	//				if (mBullets[k]->mType == TYPE_BULLET) {
	//					mBullets[k]->dead = true;		
	//				}
	//				else if (mBullets[k]->mType == TYPE_GRENADE) {
	//					/*float wallangle = atan2f(l1.x2-l1.x1,l1.y2-l1.y1);
	//					float diffangle = mBullets[k]->mAngle-wallangle;
	//					mBullets[k]->mAngle = M_PI_2-((M_PI_2-wallangle)+diffangle);
	//					mBullets[k]->mAngle -= M_PI;*/

	//					Vector2D velocity(cosf(mBullets[k]->mAngle),sinf(mBullets[k]->mAngle));
	//					Vector2D normal(-(l1.y2-l1.y1),l1.x2-l1.x1);
	//					normal.Normalize();
	//					
	//					if (velocity.Dot(normal) > 0.0f) {
	//						normal *= -1.0f;
	//					}
	//					Vector2D velocity2 = velocity-2*normal*(normal.Dot(velocity));
	//					mBullets[k]->mAngle = atan2f(velocity2.y,velocity2.x);

	//					mBullets[k]->mX = d.x+cosf(mBullets[k]->mAngle);
	//					mBullets[k]->mY = d.y+sinf(mBullets[k]->mAngle);
	//					mBullets[k]->pX = d.x+cosf(mBullets[k]->mAngle);
	//					mBullets[k]->pY = d.y+sinf(mBullets[k]->mAngle);
	//					//mBullets[k]->dead = true;		
	//				}		
	//			}
	//		}
	//	}
	//}

	//for (unsigned int i=0; i<mBullets.size(); i++) {
	//	if (mBullets[i]->dead) {
	//		Bullet* bullet = mBullets[i];
	//		mBullets[i] = mBullets.back();
	//		mBullets.back() = bullet;
	//		delete bullet;
	//		mBullets.pop_back();
	//		i--;
	//		/*delete mBullets[i];
	//		mBullets.erase(mBullets.begin()+i);
	//		i--; // makes up for the erase to prevent skipping bullets*/
	//	}
	//}
/////////////////////////////////////////////////////////////////////////////////

	for(unsigned int i=0; i<mBullets.size(); i++) {
		Bullet* bullet = mBullets[i];

		if (bullet->pX == bullet->mX && bullet->pY == bullet->mY) continue;
		if (bullet->mParent == NULL) continue;
		
		Vector2D p1(bullet->mX,bullet->mY);
		Vector2D p2(bullet->pX,bullet->pY);
		Line l1(p2,p1);
		if (fabs(p1.x-p2.x) < EPSILON && fabs(p1.y-p2.y) < EPSILON) continue; //underflow

		int gridWidth = mGrid->mWidth;
		int gridHeight = mGrid->mHeight;
		int cellSize = mGrid->mCellSize;

		int cellX = p2.x*mGrid->mConversion;///mGrid->mCellSize;
		int cellY = p2.y*mGrid->mConversion;///mGrid->mCellSize;

		int cellX2 = p1.x*mGrid->mConversion;
		int cellY2 = p1.y*mGrid->mConversion;

		float vX = bullet->cosAngle;
		float vY = bullet->sinAngle;

		int dirX = (vX >= 0.0f) ? 1 : -1;
		int dirY = (vY >= 0.0f) ? 1 : -1;
		float tX = 0.0f;
		float tY = 0.0f;

		float stepX = 0.0f;
		float stepY = 0.0f;

		if (fabs(vX) > EPSILON) {
			if (dirX >= 0.0f) {
				tX = ((cellX+1)*cellSize-p2.x)/vX+1;
			}
			else {
				tX = ((cellX)*cellSize-p2.x)/vX+1;
			}
			stepX = cellSize/fabs(vX);
		}
		else {
			tX = 100000;
			stepX = 100000;
		}

		if (fabs(vY) > EPSILON) {
			if (dirY >= 0.0f) {
				tY = ((cellY+1)*cellSize-p2.y)/vY+1;
			}
			else {
				tY = ((cellY)*cellSize-p2.y)/vY+1;
			}
			stepY = cellSize/fabs(vY);
		}
		else {
			tY = 100000;
			stepY = 100000;
		}

		while (true) {
			if (cellX < 0 || cellX >= gridWidth || cellY < 0 || cellY >= gridHeight) break;

			int cell = cellY*gridWidth + cellX;

			int intersected = 0;
			float maxT = (tX < tY) ? tX*tX : tY*tY;
			Line line(0,0,0,0);
			Vector2D d;

			for (unsigned int j=0; j<mGrid->mCells[cell].mCollisionLines.size(); j++) {
				if (mGrid->mCells[cell].mCollisionLines[j]->bullets != !!true) continue;

				Line l2 = mGrid->mCells[cell].mCollisionLines[j]->line;
				if (fabs(l2.x1-l2.x2) < EPSILON && fabs(l2.y1-l2.y2) < EPSILON) continue;

				Vector2D d2;
				if (LineLineIntersect(l1,l2,d2,true)) {
					float t = (p2-d2).LengthSquared();
					if (t < maxT) {
						maxT = t;
						line = l2;
						d = d2;
						intersected = 1;
						//break;
					}
				}
			}
			

			Person *p = NULL;

			if (bullet->mType == TYPE_BULLET) {
				for (unsigned int j=0; j<mGrid->mCells[cell].mPeople.size(); j++) {
					Person *person = mGrid->mCells[cell].mPeople[j];

					if (person->mState == DEAD) continue;
					if (bullet->mParent == person) continue; // prevent checking a bullet against its parent
					if (mGameType != FFA && mFriendlyFire == OFF && bullet->mParent->mTeam == person->mTeam) continue;

					Circle circle(person->mX,person->mY,16);
					Vector2D d2;
					float l;

					float dx = circle.x-p2.x;
					float dy = circle.y-p2.y;

					if (dx*dx+dy*dy < 16*16) {
						d2.x = person->mX;
						d2.y = person->mY;
						float t = (p2-d2).LengthSquared();
						if (t < maxT) {
							maxT = t;
							d = d2;
							intersected = 2;
							p = person;
							//break;
						}
					}
					else {
						if (LineCircleIntersect2(l1,circle,d2,l)) {
							float t = (p2-d2).LengthSquared();
							if (t < maxT) {
								maxT = t;
								d = d2;
								intersected = 2;
								p = person;
								//break;
							}
						}
					}
				}
			}


			if (intersected == 1) {					
				if (bullet->mType == TYPE_BULLET) {
					bullet->dead = true;
				}
				else if (bullet->mType == TYPE_GRENADE) {
					/*float wallangle = atan2f(l1.x2-l1.x1,l1.y2-l1.y1);
					float diffangle = bullet->mAngle-wallangle;
					bullet->mAngle = M_PI_2-((M_PI_2-wallangle)+diffangle);
					bullet->mAngle -= M_PI;*/

					Vector2D velocity(bullet->cosAngle,bullet->sinAngle);
					Vector2D normal(-(line.y2-line.y1),line.x2-line.x1);
					normal.Normalize();
					
					if (velocity.Dot(normal) > 0.0f) {
						normal *= -1.0f;
					}
					Vector2D velocity2 = velocity-2*normal*(normal.Dot(velocity));
					bullet->SetAngle(atan2f(velocity2.y,velocity2.x));
					//bullet->mAngle = atan2f(velocity2.y,velocity2.x);

					bullet->mX = d.x+cosf(bullet->mAngle);
					bullet->mY = d.y+sinf(bullet->mAngle);
					bullet->pX = d.x+cosf(bullet->mAngle);
					bullet->pY = d.y+sinf(bullet->mAngle);
					//mBullets[k]->dead = true;		
				}
				break;
			}
			else if (intersected == 2) {
				bullet->dead = true;
				if (bullet->mParent != NULL) {
					Packet sendpacket = Packet();
					sendpacket.WriteInt8(HITINDICATOR);
					Connection* connection = mUdpManager->GetConnection(bullet->mParent->mId);
					if (connection != NULL) {
						mUdpManager->SendReliable(sendpacket,connection);
					}
				}
				
				Packet sendpacket = Packet();
				sendpacket.WriteInt8(DAMAGEINDICATOR);
				sendpacket.WriteFloat(bullet->mAngle);
				Connection* connection = mUdpManager->GetConnection(p->mId);
				if (connection != NULL) {
					mUdpManager->SendReliable(sendpacket,connection);
				}

				p->TakeDamage(bullet->mDamage);
				if (p->mState == DEAD) {
					UpdateScores(bullet->mParent,p,bullet->mParentGun);
				}
				break;
			}

			if (cellX == cellX2 && cellY == cellY2) {
				break;
			}
			if (tX < tY) {
				tX += stepX;
				cellX += dirX;
			}
			else {
				tY += stepY;
				cellY += dirY;
			}
		}
	}
	for (unsigned int i=0; i<mBullets.size(); i++) {
		if (mBullets[i]->mType == TYPE_GRENADE) continue; //don't delete grenades until later /explode

		if (mBullets[i]->dead) {
			Bullet* bullet = mBullets[i];
			mBullets[i] = mBullets.back();
			mBullets.back() = bullet;
			/*if (mDeadBullets.size() < 10) {
				mDeadBullets.push_back(bullet);
			}
			else {
				delete bullet;
			}*/
			delete bullet;
			mBullets.pop_back();
			i--;

			/*delete mBullets[i];
			mBullets.erase(mBullets.begin()+i);
			i--;*/
		}
	}

	for(unsigned int i=0; i<mPeople.size(); i++) {
		Person* person1 = mPeople[i];
		if (person1 == NULL) continue;
		if (person1->mState == DEAD || person1->mTeam == NONE) continue;

		int minx = 0;
		int miny = 0;
		int maxx = 0;
		int maxy = 0;

		minx = (int)(mGrid->mConversion*(min2(person1->mX,person1->mOldX) - 16.0f));
		miny = (int)(mGrid->mConversion*(min2(person1->mY,person1->mOldY) - 16.0f));
		maxx = (int)(mGrid->mConversion*(max2(person1->mX,person1->mOldX) + 16.0f));
		maxy = (int)(mGrid->mConversion*(max2(person1->mY,person1->mOldY) + 16.0f));

		minx = min2(max2(0,minx),mGrid->mWidth-1);
		miny = min2(max2(0,miny),mGrid->mHeight-1);
		maxx = min2(max2(0,maxx),mGrid->mWidth-1);
		maxy = min2(max2(0,maxy),mGrid->mHeight-1);

		for (int y=miny; y<=maxy; y++) {
			int celly = y*mGrid->mWidth;
			for (int x=minx; x<=maxx; x++) {
				int cell = celly+x;
				for(unsigned int j=0; j<mGrid->mCells[cell].mPeople.size(); j++) {
					Person* person2 = mGrid->mCells[cell].mPeople[j];
					
					if (person2->mState == DEAD || person2->mTeam == NONE) continue;
					if (person1 == person2) continue;

					float x = person1->mX;
					float y = person1->mY;
					float x2 = person2->mX;
					float y2 = person2->mY;
					float dx = x-x2;
					float dy = y-y2;

					if (fabs(dx) < EPSILON && fabs(dy) < EPSILON) continue; //underflow

					float dist = dx*dx+dy*dy;
					float r = 16*2;
					if (dist < 40*40) {
						if (mGameType == FFA || mFriendlyFire == ON || person2->mTeam != person1->mTeam) {
							if (person1->mGunIndex == KNIFE && person1->mState == ATTACKING) {
								float angle = atan2f((y2-y),x2-x);
								float anglediff = fabs(fabs(angle+M_PI-person1->mFacingAngle)-M_PI);
								if (anglediff <= 0.6f) {
									person1->mState = DRYFIRING;
									person2->TakeDamage(person1->mGuns[KNIFE]->mGun->mDamage);
									if (person2->mState == DEAD) {
										UpdateScores(person1,person2,person1->mGuns[KNIFE]->mGun);
									}
									
									Packet sendpacket = Packet();
									sendpacket.WriteInt8(HITINDICATOR);
									Connection* connection = mUdpManager->GetConnection(person1->mId);
									if (connection != NULL) {
										mUdpManager->SendReliable(sendpacket,connection);
									}

									sendpacket.Clear();
									sendpacket.WriteInt8(DAMAGEINDICATOR);
									sendpacket.WriteFloat(angle);
									connection = mUdpManager->GetConnection(person2->mId);
									if (connection != NULL) {
										mUdpManager->SendReliable(sendpacket,connection);
									}
								}
							}
							if (person2->mState == DEAD) continue; //just in case the person dies in above if statement
							if (person2->mGunIndex == KNIFE && person2->mState == ATTACKING) {
								float angle = atan2f((y-y2),x-x2);
								float anglediff = fabs(fabs(angle+M_PI-person2->mFacingAngle)-M_PI);
								if (anglediff <= 0.6f) {
									person2->mState = DRYFIRING;
									person1->TakeDamage(person2->mGuns[KNIFE]->mGun->mDamage);
									if (person1->mState == DEAD) {
										UpdateScores(person2,person1,person2->mGuns[KNIFE]->mGun);
									}

									Packet sendpacket = Packet();
									sendpacket.WriteInt8(HITINDICATOR);
									Connection* connection = mUdpManager->GetConnection(person2->mId);
									if (connection != NULL) {
										mUdpManager->SendReliable(sendpacket,connection);
									}

									sendpacket.Clear();
									sendpacket.WriteInt8(DAMAGEINDICATOR);
									sendpacket.WriteFloat(angle);
									connection = mUdpManager->GetConnection(person1->mId);
									if (connection != NULL) {
										mUdpManager->SendReliable(sendpacket,connection);
									}
								}
							}
							if (person1->mState == DEAD) break; //just in case the person dies in above if statement
						}
						if (dist < r*r) {
							//if (mPeople[i]->mSpeed != 0.0f) {
								//mPeople[i]->SetMoveState(NOTMOVING);
								//mPeople[j]->SetMoveState(NOTMOVING);
								//mPeople[i]->SetSpeed(0.0f);
								//mPeople[j]->SetSpeed(0.0f);
								float length = sqrtf(dx*dx + dy*dy); //HERE
								dx /= length;
								dy /= length;
								dx *= r;
								dy *= r;
								float totalspeed = person1->mSpeed + person2->mSpeed;
								person1->mX = x2 + dx;
								person1->mY = y2 + dy;
								person2->mX = x - dx;
								person2->mY = y - dy;

								//doesn't work well when more than 2 collisions together
								/*
								mPeopleTemp.erase(mPeopleTemp.begin()+j);
								j--;
								*/
							//}
						}
					}
				}

				if (person1->mState == DEAD) break;//continue loop if person died above^

				
				for (unsigned int j=0; j<mGrid->mCells[cell].mGunObjects.size(); j++) {
					GunObject* gunobject = mGrid->mCells[cell].mGunObjects[j];
					//if (mGunObjects[j] == NULL) continue;
					if (!gunobject->mOnGround) continue;
					float x = person1->mX;
					float y = person1->mY;
					float x2 = gunobject->mX;
					float y2 = gunobject->mY;
					float dx = x-x2;
					float dy = y-y2;
					float dist = dx*dx+dy*dy;
					float r = 16;
					if (dist < r*r) {
						if (person1->PickUp(gunobject)) {
							Packet sendpacket = Packet();
							sendpacket.WriteInt8(PICKUPGUN);
							sendpacket.WriteInt8(person1->mId);
							sendpacket.WriteInt16(gunobject->mId);
							for (int k=0; k<mUdpManager->mConnections.size(); k++) {
								mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[k],true);
							}

							for (unsigned int k=0; k<mGunObjects.size(); k++) {
								if (mGunObjects[k] == gunobject) {
									mGunObjects.erase(mGunObjects.begin()+k);
									break;
								}
							}
							mGrid->mCells[cell].mGunObjects.erase(mGrid->mCells[cell].mGunObjects.begin()+j);
							j--;
						}
					}
				}

				if (mGameType == CTF) {
					int team1 = person1->mTeam;
					int team2 = CT;
					if (team1 == CT) {
						team2 = T;
					}
				
					if (person1 == mFlagHolder[team2]) { //captured
						float dx = person1->mX-mMap->mFlagSpawn[team1].x;
						float dy = person1->mY-mMap->mFlagSpawn[team1].y;
						float dist = dx*dx+dy*dy;
						float r = 16;
						if (dist < r*r) {
							if (mIsFlagHome[team1]) {
								mFlagX[team2] = mMap->mFlagSpawn[team2].x;
								mFlagY[team2] = mMap->mFlagSpawn[team2].y;
								mIsFlagHome[team2] = true;
								mFlagHolder[team2] = NULL;
								mNumFlags[team1] += 1;

								person1->mMoney += 1000;
								if (person1->mMoney > 16000) {
									person1->mMoney = 16000;
								}
								person1->mNumKills += 5;

								cout << GetPersonName(person1) << " captured the enemy flag\n";

								Packet sendpacket = Packet();
								sendpacket.WriteInt8(CAPTUREFLAG);
								sendpacket.WriteInt8(team2);
								sendpacket.WriteInt8(person1->mId);

								for (int k=0; k<mUdpManager->mConnections.size(); k++) {
									mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[k],true);
								}
								sendpacket.Clear();
							}
							else {
								if (person1->mIsInCaptureZone == false) {
									Packet sendpacket = Packet();
									sendpacket.WriteInt8(MESSAGE);
									sendpacket.WriteInt8(4);
									mUdpManager->SendReliable(sendpacket,mUdpManager->GetConnection(person1->mId));
									sendpacket.Clear();
								}
							}
							person1->mIsInCaptureZone = true;
						}
						else {
							person1->mIsInCaptureZone = false;
						}
					}
					if (!mIsFlagHome[team1] && mFlagHolder[team1] == NULL) { //ownflag
						float dx = person1->mX-mFlagX[team1];
						float dy = person1->mY-mFlagY[team1];
						float dist = dx*dx+dy*dy;
						float r = 16;
						if (dist < r*r) {
							mFlagX[team1] = mMap->mFlagSpawn[team1].x;
							mFlagY[team1] = mMap->mFlagSpawn[team1].y;
							mIsFlagHome[team1] = true;

							person1->mMoney += 300;
							if (person1->mMoney > 16000) {
								person1->mMoney = 16000;
							}

							cout << GetPersonName(person1) << " returned the flag\n";

							Packet sendpacket = Packet();
							sendpacket.WriteInt8(RETURNFLAG);
							sendpacket.WriteInt8(team1);
							sendpacket.WriteInt8(person1->mId);

							for (int k=0; k<mUdpManager->mConnections.size(); k++) {
								mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[k],true);
							}
							sendpacket.Clear();
						}
					}
					if (mFlagHolder[team2] == NULL) { //otherflag
						float dx = person1->mX-mFlagX[team2];
						float dy = person1->mY-mFlagY[team2];
						float dist = dx*dx+dy*dy;
						float r = 16;
						if (dist < r*r) {
							mIsFlagHome[team2] = false;
							mFlagHolder[team2] = person1;
							person1->mHasFlag;

							person1->mMoney += 300;
							if (person1->mMoney > 16000) {
								person1->mMoney = 16000;
							}

							cout << GetPersonName(person1) << " has the enemy flag\n";

							Packet sendpacket = Packet();
							sendpacket.WriteInt8(PICKUPFLAG);
							sendpacket.WriteInt8(team2);
							sendpacket.WriteInt8(person1->mId);

							for (int k=0; k<mUdpManager->mConnections.size(); k++) {
								mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[k],true);
							}
							sendpacket.Clear();
						}
					}
				}

				person1->mIsInBuyZone = false;
				if (mGameType == FFA) {
					person1->mIsInBuyZone = true;
				}
				else {
					std::vector<BuyZone>* buyzones = NULL;
					if (person1->mTeam == CT) {
						buyzones = &(mMap->mCTBuyZones);
					}
					else if (person1->mTeam == T) {
						buyzones = &(mMap->mTBuyZones);
					}
					if (buyzones != NULL) {
						for (int j=0; j<buyzones->size(); j++) {
							if (person1->mX >= (*buyzones)[j].x1 && person1->mX <= (*buyzones)[j].x2 && person1->mY >= (*buyzones)[j].y1 && person1->mY <= (*buyzones)[j].y2) {
								person1->mIsInBuyZone = true;
								break;
							}
						}
					}
				}


				for (unsigned int j=0; j<mGrid->mCells[cell].mCollisionLines.size(); j++) {
					if (mGrid->mCells[cell].mCollisionLines[j]->people != true) continue;

					Line l1 = mGrid->mCells[cell].mCollisionLines[j]->line;
					//if (l1.x1 == l1.x2 && l1.y1 == l1.y2) continue;
					if (fabs(l1.x1-l1.x2) < EPSILON && fabs(l1.y1-l1.y2) < EPSILON) continue;

					Vector2D pos(person1->mX,person1->mY);
					Vector2D oldpos(person1->mOldX,person1->mOldY);
					Circle circle(pos,16);
					Line l2(pos,oldpos);

					Vector2D d;
					if (LineLineIntersect(l1,l2,d,true)) {
						Vector2D dir = oldpos-pos;
						dir.Normalize();
						//dir *= 16;
						person1->mX = d.x+dir.x;
						person1->mY = d.y+dir.y;
						//person1->mOldX = (d.x+dir.x);
						//person1->mOldY = (d.y+dir.y);
						pos.x = person1->mX;
						pos.y = person1->mY;
						circle.x = pos.x;
						circle.y = pos.y;
					}

					float l;
					if (LineCircleIntersect(l1,circle,d,l,true)) {
						pos += d * (circle.radius - l);
						person1->mX = pos.x;//-6*cosf(mPeople[j]->mFacingAngle));
						person1->mY = pos.y;//-6*sinf(mPeople[j]->mFacingAngle));
						//mPeople[j]->mOldX = pos.x;
						//mPeople[j]->mOldY = pos.y;
					}
				}

			}
			if (person1->mState == DEAD) break;
		}
	}

	for(unsigned int i=0; i<mGunObjects.size(); i++) {
		GunObject* gunobject = mGunObjects[i];

		if (gunobject == NULL) continue;
		if (gunobject->mOnGround) continue;

		int minx = (int)(mGrid->mConversion*(((gunobject->mX < gunobject->mOldX) ? gunobject->mX:gunobject->mOldX) - 8.0f));
		int miny = (int)(mGrid->mConversion*(((gunobject->mY < gunobject->mOldY) ? gunobject->mY:gunobject->mOldY) - 8.0f));
		int maxx = (int)(mGrid->mConversion*(((gunobject->mX > gunobject->mOldX) ? gunobject->mX:gunobject->mOldX) + 8.0f));
		int maxy = (int)(mGrid->mConversion*(((gunobject->mY > gunobject->mOldY) ? gunobject->mY:gunobject->mOldY) + 8.0f));

		minx = min2(max2(0,minx),mGrid->mWidth-1);
		miny = min2(max2(0,miny),mGrid->mHeight-1);
		maxx = min2(max2(0,maxx),mGrid->mWidth-1);
		maxy = min2(max2(0,maxy),mGrid->mHeight-1);

		for (int y=miny; y<=maxy; y++) {
			int celly = y*mGrid->mWidth;
			for (int x=minx; x<=maxx; x++) {
				int cell = celly+x;
	
				Vector2D pos(gunobject->mX,gunobject->mY);
				Vector2D oldpos(gunobject->mOldX,gunobject->mOldY);
				Circle circle(pos,8);
				Line l2(pos,oldpos);

				for (unsigned int j=0; j<mGrid->mCells[cell].mCollisionLines.size(); j++) {
					if (mGrid->mCells[cell].mCollisionLines[j]->people != true) continue;

					Line l1 = mGrid->mCells[cell].mCollisionLines[j]->line;
					//if (l1.x1 == l1.x2 && l1.y1 == l1.y2) continue;
					if (fabs(l1.x1-l1.x2) < EPSILON && fabs(l1.y1-l1.y2) < EPSILON) continue;

					Vector2D d;
					if (LineLineIntersect(l1,l2,d,true)) {
						Vector2D dir = oldpos-pos;
						dir.Normalize();
						//dir *= 5;
						gunobject->mX = d.x+dir.x;
						gunobject->mY = d.y+dir.y;
						pos.x = gunobject->mX;
						pos.y = gunobject->mY;
						circle.x = pos.x;
						circle.y = pos.y;
					}

					float l;
					if (LineCircleIntersect(l1,circle,d,l,true)) {
						pos += d * (circle.radius - l);
						gunobject->mX = pos.x;
						gunobject->mY = pos.y;
					}
				}
			}
		}
	}
}

void GameServer::CheckPlayerCollisions(Person* player)
{
	if (player->mState == DEAD || player->mTeam == NONE) return;
	for(unsigned int j=0; j<mPeople.size(); j++) {
		if (mPeople[j]->mState == DEAD || mPeople[j]->mTeam == NONE) continue;
		if (player != mPeople[j]) {
			float x = player->mX;
			float y = player->mY;
			float x2 = mPeople[j]->mX;
			float y2 = mPeople[j]->mY;
			float dx = x-x2;
			float dy = y-y2;
			float dist = dx*dx+dy*dy;
			float r = 16*2;
			if (dist < 35*35) {
				if (dist < r*r) {
					//if (mPeople[i]->mSpeed != 0.0f) {
						//mPeople[i]->SetMoveState(NOTMOVING);
						//mPeople[j]->SetMoveState(NOTMOVING);
						//mPeople[i]->SetSpeed(0.0f);
						//mPeople[j]->SetSpeed(0.0f);
						float length = sqrtf(dx*dx + dy*dy);
						dx /= length;;
						dy /= length;
						dx *= r;
						dy *= r;
						float totalspeed = player->mSpeed + mPeople[j]->mSpeed;
						player->mX = (x2 + dx);
						player->mY = (y2 + dy);
						mPeople[j]->mX = (x - dx);
						mPeople[j]->mY = (y - dy);

						//doesn't work well when more than 2 collisions together
						/*
						mPeopleTemp.erase(mPeopleTemp.begin()+j);
						j--;
						*/
					//}
				}
			}
		}
	}

	for (unsigned int j=0;j<mGunObjects.size();j++) {
		if (mGunObjects[j] == NULL) continue;
		if (!mGunObjects[j]->mOnGround) continue;

		bool pickup = false;

		float x = player->mX;
		float y = player->mY;
		float x2 = mGunObjects[j]->mX;
		float y2 = mGunObjects[j]->mY;

		Vector2D pos(x,y);
		Vector2D oldpos(player->mOldX,player->mOldY);
		Circle circle(x2,y2,16);
		Line line(pos,oldpos);

		Vector2D d;
		float l;
		if (LineCircleIntersect(line,circle,d,l,false)) {
			pickup = true;
		}

		float dx = x-x2;
		float dy = y-y2;
		float dist = dx*dx+dy*dy;
		float r = 16;
		if (dist < r*r) {
			pickup = true;
		}

		if (pickup) {
			if (player->PickUp(mGunObjects[j])) {
				Packet sendpacket = Packet();
				sendpacket.WriteInt8(PICKUPGUN);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteInt16(mGunObjects[j]->mId);
				for (int k=0; k<mUdpManager->mConnections.size(); k++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[k],true);
				}
				mGunObjects.erase(mGunObjects.begin()+j);
				j--;
			}
		}
	}

	for (unsigned int i=0;i<mMap->mCollisionPoints.size()-1;i++) {
		if (mMap->mCollisionPoints[i].x == -1 || mMap->mCollisionPoints[i+1].x == -1) continue;
		Vector2D p1(mMap->mCollisionPoints[i].x,mMap->mCollisionPoints[i].y);
		Vector2D p2(mMap->mCollisionPoints[i+1].x,mMap->mCollisionPoints[i+1].y);
		//if (p1 == p2) continue;
		if (fabs(p1.x-p2.x) < EPSILON && fabs(p1.y-p2.y) < EPSILON) continue;

		Line l1(p1,p2);

		if (mMap->mCollisionPoints[i].people == true) {
			Vector2D pos(player->mX,player->mY);
			Vector2D oldpos(player->mOldX,player->mOldY);
			Circle circle(pos,16);
			Line l2(pos,oldpos);

			Vector2D d;
			if (LineLineIntersect(l1,l2,d,true)) {
				Vector2D dir = oldpos-pos;
				dir.Normalize();
				dir *= 16;
				player->mX = (d.x+dir.x);
				player->mY = (d.y+dir.y);
				//mPeople[j]->mOldX = (d.x+dir.x);
				//mPeople[j]->mOldY = (d.y+dir.y);
				pos.x = player->mX;
				pos.y = player->mY;
				circle.x = pos.x;
				circle.y = pos.y;
			}

			float l;
			if (LineCircleIntersect(l1,circle,d,l,true)) {
				pos += d * (circle.radius - l);
				player->mX = (pos.x);//-6*cosf(mPeople[j]->mFacingAngle));
				player->mY = (pos.y);//-6*sinf(mPeople[j]->mFacingAngle));
			}
		}
	}
}

void GameServer::HandlePacket(Packet &packet, Connection* connection, sockaddr_in from, bool sendack) {
	if (connection != NULL) {
		connection->timer = 0;
	}


	int type;
	bool round;
	Packet sendpacket;

	while (packet.Index() < packet.Length()) {
		int startindex = packet.Index();
		char temp = packet.ReadInt8();
		type = temp & 127;
		round = false;
		if ((int)(temp & 128) == (int)mRoundBit) {
			round = true;
		}
		sendpacket = Packet();
		//printf("%s\n",(round)?"1":"0");
	
		if (type == SERVERINFO) {
			int index = packet.ReadInt16();

			sendpacket.WriteInt8(NETVERSION);
			sendpacket.WriteInt8(SERVERINFO);
			sendpacket.WriteInt16(index);
			sendpacket.WriteChar(mName);
			sendpacket.WriteChar(mMapName);
			sendpacket.WriteInt8(mNumPlayers);
			sendpacket.WriteInt8(mNumMaxPlayers);

			int n = sendto(sock,sendpacket.Data(),sendpacket.Length(), 0,(struct sockaddr *)&from,sizeof(from));

			//mUdpManager->Send(sendpacket,from);
			sendpacket.Clear();
			continue;
		}
		else if (type == TIME) {
			float ctime = packet.ReadFloat();
			float time = packet.ReadFloat();

			if (connection != NULL) {
				connection->time = ctime;
				
				float ping = mTime-time;
				if (ping < 10.0f) ping = 10.0f;
				else if (ping > 1000.0f) ping = 1000.0f;
				connection->sping = ping;
			}
			continue;
		}
		else if (type == CONNECT) {
			char name[32];
			packet.ReadChar(name,32);
			char accountname[32];
			packet.ReadChar(accountname,32);
			int ackid = packet.ReadInt16();
		
			bool reconnecting = false;
			if (connection != NULL) {
				Person* player = GetPerson(connection->playerid);
				if (player == NULL || connection->reconnecting == true) {
					reconnecting = true;
				}
			}

			if (connection == NULL || reconnecting == true) {
				if (reconnecting == true) {
					mUdpManager->RemoveConnection(connection);
				}
				connection = mUdpManager->AddConnection(GetPlayerId(), from);

				// send ack and reset the orderid
				connection->orderid = ackid;
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}

				// check if name is availabe; if not, send message
				bool taken = false;
				for (int i=0; i<mPeople.size(); i++) {
					if (strcmp(mPeople[i]->mName,name) == 0) {
						taken = true;
						break;
					}
				}
				if (taken) {
					sendpacket.WriteInt8(ERROR1);
					sendpacket.WriteChar("Error: Try reconnecting after a few seconds.");
					mUdpManager->SendReliable(sendpacket,connection);
					sendpacket.Clear();	
					connection->timer += 9000;
					continue;
				}

				if (mNumPlayers >= mNumMaxPlayers) {
					sendpacket.WriteInt8(ERROR1);
					sendpacket.WriteChar("Error: Server is full");
					mUdpManager->SendReliable(sendpacket,connection);
					sendpacket.Clear();	
					connection->timer += 9000;
					continue;
				}
				bool banned = false;
				for (int i=0; i<mBannedPeople.size(); i++) {
					if (strcmp(accountname,mBannedPeople[i]) == 0) {
						banned = true;
						break;
					}
				}
				if (banned) {
					sendpacket.WriteInt8(ERROR1);
					sendpacket.WriteChar("Error: You have been banned from this server");
					mUdpManager->SendReliable(sendpacket,connection);
					sendpacket.Clear();
					connection->timer += 9000;
					continue;
				}

				sendpacket.WriteInt8(CONNECT);

				bool admin = false;
				for (int i=0; i<mAdmins.size(); i++) {
					if (strcmp(accountname,mAdmins[i]) == 0) {
						admin = true;
						break;
					}
				}

				sendpacket.WriteInt8(admin);
				sendpacket.WriteChar(mMapName);
				sendpacket.WriteInt32(mMapTextSize);
				sendpacket.WriteInt32(mMapImageSize);
				sendpacket.WriteInt32(mMapOverviewSize);
				mUdpManager->SendReliable(sendpacket,connection,true);
				sendpacket.Clear();

				// Send playerid disguised as an invalid message type
				sendpacket.WriteInt8(connection->playerid + 64);
				mUdpManager->Send(sendpacket, connection);
				sendpacket.Clear();
			}
			continue;

		}

		if (connection == NULL) break;

		switch (type) {
			case NEWPLAYER: {
				char name[32];
				packet.ReadChar(name,32);
				char accountname[32];
				packet.ReadChar(accountname,32);
				int movementstyle = packet.ReadInt8();
				char icon[300];
				int datalength = packet.ReadData(icon);
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}
			
				//check if name is availabe; if not, send message
				bool taken = false;
				for (int i=0; i<mPeople.size(); i++) {
					if (strcmp(mPeople[i]->mName,name) == 0) {
						taken = true;
						break;
					}
				}
				if (taken) {
					sendpacket.WriteInt8(ERROR1);
					sendpacket.WriteChar("Error: Try reconnecting after a few seconds.");
					mUdpManager->SendReliable(sendpacket,connection);
					sendpacket.Clear();	
					connection->timer += 9000;
					break;
				}

				if (mNumPlayers >= mNumMaxPlayers) {
					sendpacket.WriteInt8(ERROR1);
					sendpacket.WriteChar("Error: Server is full");
					mUdpManager->SendReliable(sendpacket,connection);
					sendpacket.Clear();	
					connection->timer += 9000;
					break;
				}

				mNumPlayers++;

				Person* player = new Person(&mGunObjects,&mBullets,mUdpManager,name);
				player->mGuns[KNIFE] = new GunObject(&mGuns[0],0,0);
				player->mGunIndex = KNIFE;
				player->mMovementStyle = movementstyle;
				player->mTime = &mTime;
				
				player->mId = connection->playerid;
				player->SetState(DEAD);
				strcpy(player->mAccountName,accountname);
				memcpy(player->mIcon,icon,datalength);

				mPeople.push_back(player);
				if (mOnPlayerListUpdate) {
					mOnPlayerListUpdate();
				}

				sendpacket.WriteInt8(GAMEINFO);
				sendpacket.WriteChar(mName);
				sendpacket.WriteInt8(mGameType);
				//sendpacket.WriteChar(mMapName);
				sendpacket.WriteInt8(mFriendlyFire);
				//sendpacket.WriteInt8(mNumCTs);
				//sendpacket.WriteInt8(mNumTs);
				sendpacket.WriteInt8(mNumRemainingCTs);
				sendpacket.WriteInt8(mNumRemainingTs);
				sendpacket.WriteFloat(mTime);
				sendpacket.WriteInt16(mRoundFreezeTime);
				sendpacket.WriteInt16(mRoundTime);
				sendpacket.WriteInt16(mRoundEndTime);
				sendpacket.WriteInt16(mBuyTime);
				sendpacket.WriteInt16(mRespawnTime);
				sendpacket.WriteInt16(mInvincibleTime);
				sendpacket.WriteInt8(mRoundState);
				sendpacket.WriteFloat(mRoundTimer);
				sendpacket.WriteFloat(mRoundEndTimer);
				sendpacket.WriteFloat(mBuyTimer);
				sendpacket.WriteInt16(mNumRounds);
				sendpacket.WriteInt16(mNumCTWins);
				sendpacket.WriteInt16(mNumTWins);
				sendpacket.WriteInt8(mWinner);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteChar(player->mName);
				sendpacket.WriteFloat(mTimeMultiplier);
				mUdpManager->SendReliable(sendpacket,connection,true);
				sendpacket.Clear();

				for (int i=0; i<mNumGuns; i++) {
					sendpacket.WriteInt8(GUNINFO);
					sendpacket.WriteInt8(mGuns[i].mId);
					sendpacket.WriteInt16(mGuns[i].mDelay);
					sendpacket.WriteInt16(mGuns[i].mDamage);
					sendpacket.WriteFloat(mGuns[i].mSpread);
					sendpacket.WriteInt8(mGuns[i].mClip);
					sendpacket.WriteInt8(mGuns[i].mNumClips);
					sendpacket.WriteInt16(mGuns[i].mReloadDelay);
					sendpacket.WriteFloat(mGuns[i].mSpeed);
					sendpacket.WriteFloat(mGuns[i].mBulletSpeed);
					sendpacket.WriteFloat(mGuns[i].mViewAngle);
					sendpacket.WriteInt16(mGuns[i].mCost);
					sendpacket.WriteInt8(mGuns[i].mType);
					sendpacket.WriteChar(mGuns[i].mName);
					mUdpManager->SendReliable(sendpacket,connection,true);
					sendpacket.Clear();
				}
				/*sendpacket.WriteInt8(NEWMAP);
				sendpacket.WriteChar(mMapName);
				mUdpManager->SendReliable(sendpacket,connection);
				sendpacket.Clear();*/

				sendpacket.WriteInt8(GAMEINFOEND);
				mUdpManager->SendReliable(sendpacket,connection,true);
				sendpacket.Clear();

				for (int i=0; i<mPeople.size(); i++) {
					if (mPeople[i]->mId != player->mId) {
						sendpacket.WriteInt8(NEWPLAYER);
						sendpacket.WriteInt8(mPeople[i]->mId);
						sendpacket.WriteInt16(mPeople[i]->mX);
						sendpacket.WriteInt16(mPeople[i]->mY);
						sendpacket.WriteInt8(mPeople[i]->mTeam);
						sendpacket.WriteInt8(mPeople[i]->mType);
						sendpacket.WriteInt8(mPeople[i]->mState);
						sendpacket.WriteInt16(mPeople[i]->mNumKills);
						sendpacket.WriteInt16(mPeople[i]->mNumDeaths);
						sendpacket.WriteInt8(mPeople[i]->mMovementStyle);
						sendpacket.WriteInt16(mPeople[i]->mMoney);
						sendpacket.WriteChar(mPeople[i]->mName);
						mUdpManager->SendReliable(sendpacket,connection,true);
						sendpacket.Clear();
					}

					for (int j=0; j<5; j++) {
						GunObject* gunobject = mPeople[i]->mGuns[j];
						if (gunobject == NULL) continue;
						if (gunobject->mGun->mType == KNIFE) continue;
						sendpacket.WriteInt8(NEWGUN);
						sendpacket.WriteInt8(mPeople[i]->mId);
						sendpacket.WriteInt16(gunobject->mId);
						sendpacket.WriteInt8(gunobject->mGun->mId);
						sendpacket.WriteInt8(gunobject->mClipAmmo);
						sendpacket.WriteInt16(gunobject->mRemainingAmmo);
						mUdpManager->SendReliable(sendpacket,connection,true);
						sendpacket.Clear();		
					}
					sendpacket.WriteInt8(SWITCHGUN);
					sendpacket.WriteInt8(mPeople[i]->mId);
					sendpacket.WriteInt8(mPeople[i]->mGunIndex);
					mUdpManager->SendReliable(sendpacket,connection,true);
					sendpacket.Clear();
				}

				for (int i=0; i<mGunObjects.size(); i++) {
					sendpacket.WriteInt8(SPAWNGUN);
					sendpacket.WriteInt16(mGunObjects[i]->mId);
					sendpacket.WriteInt8(mGunObjects[i]->mGun->mId);
					sendpacket.WriteInt8(mGunObjects[i]->mClipAmmo);
					sendpacket.WriteInt16(mGunObjects[i]->mRemainingAmmo);
					sendpacket.WriteInt16((int)mGunObjects[i]->mX);
					sendpacket.WriteInt16((int)mGunObjects[i]->mY);

					mUdpManager->SendReliable(sendpacket,connection,true);
					sendpacket.Clear();
				}

				if (mGameType == CTF) {
					sendpacket.WriteInt8(CTFINFO);
					sendpacket.WriteInt16(mNumFlags[CT]);
					sendpacket.WriteInt16(mNumFlags[T]);
					sendpacket.WriteInt16((int)mFlagX[CT]);
					sendpacket.WriteInt16((int)mFlagY[CT]);
					sendpacket.WriteInt16((int)mFlagX[T]);
					sendpacket.WriteInt16((int)mFlagY[T]);
					sendpacket.WriteInt8((mIsFlagHome[CT]) ? 1:0);
					sendpacket.WriteInt8((mIsFlagHome[T]) ? 1:0);
					if (mFlagHolder[CT] == NULL) {
						sendpacket.WriteInt8(-1);
					}
					else {
						sendpacket.WriteInt8(mFlagHolder[CT]->mId);
					}
					if (mFlagHolder[T] == NULL) {
						sendpacket.WriteInt8(-1);
					}
					else {
						sendpacket.WriteInt8(mFlagHolder[T]->mId);
					}
					mUdpManager->SendReliable(sendpacket,connection,true);
					sendpacket.Clear();
				}

				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					if (connection == mUdpManager->mConnections[i]) continue;
					sendpacket.WriteInt8(NEWPLAYER);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt16(0);
					sendpacket.WriteInt16(0);
					sendpacket.WriteInt8(player->mTeam);
					sendpacket.WriteInt8(player->mType);
					sendpacket.WriteInt8(player->mState);
					sendpacket.WriteInt16(player->mNumKills);
					sendpacket.WriteInt16(player->mNumDeaths);
					sendpacket.WriteInt8(player->mMovementStyle);
					sendpacket.WriteInt16(player->mMoney);
					sendpacket.WriteChar(player->mName);
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					sendpacket.Clear();

					for (int j=0; j<5; j++) {
						GunObject* gunobject = player->mGuns[j];
						if (gunobject == NULL) continue;
						if (gunobject->mGun->mType == KNIFE) continue;
						sendpacket.WriteInt8(NEWGUN);
						sendpacket.WriteInt8(player->mId);
						sendpacket.WriteInt16(gunobject->mId);
						sendpacket.WriteInt8(gunobject->mGun->mId);
						sendpacket.WriteInt8(gunobject->mClipAmmo);
						sendpacket.WriteInt16(gunobject->mRemainingAmmo);
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
						sendpacket.Clear();		
					}
				}

				cout << GetPersonName(player) << " connected\n";

				break;
			}
			case PLAYERICON: { // icon request
				int id = packet.ReadInt8();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection);
				}

				Person* player = GetPerson(id);
				if (player == NULL) break;

				sendpacket.WriteInt8(PLAYERICON);
				sendpacket.WriteInt8(id);
				sendpacket.WriteData(player->mIcon,300);
				mUdpManager->SendReliable(sendpacket,connection);
				sendpacket.Clear();
						
				break;
			}
			case REMOVEPLAYER: {
				int id = packet.ReadInt8();
				
				if (connection == NULL) break;
				if (connection->playerid == id) {
					if (id != -1) {
						sendpacket.WriteInt8(REMOVEPLAYER);
						sendpacket.WriteInt8(id);
						for (int i=0; i<mUdpManager->mConnections.size(); i++) {
							if (mUdpManager->mConnections[i] == connection) continue;
							mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
						}
						sendpacket.Clear();
					}

					mUdpManager->RemoveConnection(connection);

					Person* player = GetPerson(id);
					if (player != NULL) {
						cout << player->mName << " disconnected\n";
					}

					RemovePerson(player);
				}

				break;
			}
			case SWITCHTEAM: {
				int id = packet.ReadInt8();
				int team = packet.ReadInt8();
				int type = packet.ReadInt8();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				if (connection->playerid != id) break;

				Person* player = GetPerson(id);
				if (player == NULL) break;
	
				if (player->mTeam == team) break;

				if (mAutoBalance == ON && mGameType == CTF) {
					if (team == CT) {
						if (mNumCTs >= mNumTs+1) {
							team = T;
						}
					}
					else if (team == T) {
						if (mNumTs >= mNumCTs+1) {
							team = CT;
						}
					}
				}

				if (player->mTeam == team) break;

				if (player->mState != DEAD && (player->mTeam == CT || player->mTeam == T)) {
					player->Die();
					player->mRespawnTime = mRespawnTime*1000;
					UpdateScores(player,player,NULL);
				}

				if (team == NONE) {
				}
				else if (team == CT) {
					if (mGameType == FFA) {
						if (mNumCTs+mNumTs == mMap->mNumCTs) { //too many
							sendpacket.WriteInt8(MESSAGE);
							sendpacket.WriteInt8(3);
							mUdpManager->SendReliable(sendpacket,connection);
							sendpacket.Clear();
							break;
						}
					}
					else {
						if (mNumCTs == mMap->mNumCTs) { //too many
							sendpacket.WriteInt8(MESSAGE);
							sendpacket.WriteInt8(3);
							mUdpManager->SendReliable(sendpacket,connection);
							sendpacket.Clear();
							break;
						}
					}
					mNumCTs++;
				}
				else if (team == T) {
					if (mGameType == FFA) {
						if (mNumCTs+mNumTs == mMap->mNumCTs) { //too many
							sendpacket.WriteInt8(MESSAGE);
							sendpacket.WriteInt8(3);
							mUdpManager->SendReliable(sendpacket,connection);
							sendpacket.Clear();
							break;
						}
					}
					else {
						if (mNumTs == mMap->mNumTs) { //too many
							sendpacket.WriteInt8(MESSAGE);
							sendpacket.WriteInt8(3);
							mUdpManager->SendReliable(sendpacket,connection);
							sendpacket.Clear();
							break;
						}
					}
					mNumTs++;
				}

				if (player->mTeam == NONE) {
				}
				else if (player->mTeam == CT) {
					mNumCTs--;
				}
				else if (player->mTeam == T) {
					mNumTs--;
				}

				player->mTeam = team;

				if (type < 0 || type > 3) {
					type = 0;
				}

				player->mType = type;

				sendpacket.WriteInt8(SWITCHTEAM);
				sendpacket.WriteInt8(id);
				sendpacket.WriteInt8(team);
				sendpacket.WriteInt8(type);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					//if (mUdpManager->CompareIP(mUdpManager->mConnections[i]->addr,from)) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();

				if (mGameType == FFA && team != NONE) {
				}
				else {
					cout << GetPersonName(player) << " has joined the ";

					if (team == CT) {
						cout << "Counter-Terrorist Team\n";
					}	
					else if (team == T) {
						cout << "Terrorist Team\n";
					}	
					else if (team == NONE) {
						cout << "Spectators\n";
					}
				}


				if (team != NONE) {
					if (mNumCTs+mNumTs <= 2) {
						mWinner = NONE;
						if (mGameType == FFA || mGameType == CTF) {
							ResetRound(true);
						}
						else {
							ResetRound();
						}
					}
				}

				//cout << "\n";

				break;
			}
 			case PLAYERMOVE: {
				int x = packet.ReadInt8();
				int y = packet.ReadInt8();
				int facingangle = packet.ReadInt8();
				int state = packet.ReadInt8(); // gunindex and isfiring
				float time = packet.ReadFloat();

				Person* player = GetPerson(connection->playerid);
				if (player == NULL) break;
				if (player->mState == DEAD) break;
				//printf("%d : %f %f %f\n",player->mId,player->mCurrentTime,time,mLastRoundTime);
				if (player->mCurrentTime >= time) {
					//printf("\tTESTasdf\n");
					break;
				}
				if (!round) {
					//printf("\tTESTround\n");
					break;
				}

				if (time-mTime > 1000.0f) { // client's time is moving too fast..
					break;
				}

				Input input = {x,y,(float)((facingangle+128)/(255.0f/(2*M_PI)))};
				player->ReceiveInput(input,time);
				
				bool isfiring = (state&128) != 0;
				player->mIsFiring = isfiring;

				int gunindex = state&127;
				if (gunindex < 0 || gunindex >= 5) break;
				if (gunindex != player->mGunIndex) {
					player->Switch(gunindex);
				}
				
				//cout << player->mFacingAngle;
				//cout << "\n";
				break;
			}
			case MOVE: {
				int id = packet.ReadInt8();
				int facingangle = packet.ReadInt8();
				int speed = packet.ReadInt8();
				int angle = packet.ReadInt8();

				if (connection == NULL) break;
				if (connection->playerid != id) {
					break;
				}
				Person* player = GetPerson(id);
				if (player == NULL) break;
				if (player->mState == DEAD) break;

				player->SetTotalRotation((float)((facingangle+128)/(255/(2*M_PI))));

				if (speed >= 1) {
					player->Move((float)(speed/100.0f),(float)((angle+128)/(255/(2*M_PI))));
				} 
				else {
					player->SetMoveState(NOTMOVING);
				}
				//mUdpManager->SendAll();
				//cout<<speed;
				//cout<<"\n";
				break;
			}
			case SWITCHGUN: {
				int gunindex = packet.ReadInt8();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				Person* player = GetPerson(connection->playerid);
				if (player == NULL) break;
				if (player->mState == DEAD) break;
				if (gunindex < 0 || gunindex >= 5) break;

				player->Switch(gunindex);

				sendpacket.WriteInt8(SWITCHGUN);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteInt8(gunindex);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					if (mUdpManager->mConnections[i] == connection) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();
				break;
			}
			case DROPGUN: {
				int gunindex = packet.ReadInt8();
				int gunid = packet.ReadInt16();
				//float time = packet.ReadFloat();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				Person* player = GetPerson(connection->playerid);
				if (player == NULL) break;
				if (player->mState == DEAD) break;

				if (gunindex >= 5) break;
				if (gunindex == KNIFE) break;

				//if (time-mTime > TIMETHRESHOLD) break;

				bool previousround = !round;

				GunObject* gunobject = player->mGuns[gunindex];
				if (gunobject == NULL) break;

				if (gunobject->mId == gunid) {
					if (player->Drop(gunindex)) {
					}
					
					sendpacket.WriteInt8(DROPGUN);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt8(gunindex);
					sendpacket.WriteInt16(gunid);
					sendpacket.WriteInt16(gunobject->mX);
					sendpacket.WriteInt16(gunobject->mY);

					int angle = (int)(gunobject->mAngle*(255/(2*M_PI)))-128;
					sendpacket.WriteInt8(angle);

					//sendpacket.WriteFloat(time);
					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						//if (mUdpManager->CompareIP(mUdpManager->mConnections[i]->addr,from)) continue;
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true,round);
					}
					sendpacket.Clear();

					if (previousround) {
						delete mGunObjects.back();
						mGunObjects.pop_back();
					}
				}
				break;
			}
			case STARTFIRE: {
				int id = packet.ReadInt8();
				//int gunindex = packet.ReadInt16();
				//float angle = packet.ReadInt16();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection);
				}

				Person* player = GetPerson(id);
				if (player == NULL) break;
				if (player->mState == DEAD) break;
				
				player->mIsFiring = true;

				sendpacket.WriteInt8(STARTFIRE);
				sendpacket.WriteInt8(id);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					//if (mUdpManager->CompareIP(mUdpManager->mConnections[i]->addr,from)) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i]);
				}
				sendpacket.Clear();

				break;
			}
			case ENDFIRE: {
				int id = packet.ReadInt8();
				//int gunindex = packet.ReadInt16();
				//float angle = packet.ReadInt16();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection);
				}

				Person* player = GetPerson(id);
				if (player == NULL) break;
				//if (player->mState == DEAD) break;
				
				player->mIsFiring = false;
				player->mHasFired = false;

				sendpacket.WriteInt8(ENDFIRE);
				sendpacket.WriteInt8(id);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					//if (mUdpManager->CompareIP(mUdpManager->mConnections[i]->addr,from)) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i]);
				}
				sendpacket.Clear();
				break;
			}
			case NEWBULLET: {
				//int id = packet.ReadInt16();
				int guntype = packet.ReadInt8();
				int x = packet.ReadInt16();
				int y = packet.ReadInt16();
				//int px = packet.ReadInt16();
				//int py = packet.ReadInt16();
				int angle = packet.ReadInt16();
				//int speed = packet.ReadInt8();
				//int damage = packet.ReadInt8();
				//int parentid = packet.ReadInt8();
				int ammo = packet.ReadInt8();
				float time = packet.ReadFloat();

				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection);
				}

				/*bool exists = false;
				for (int i=0; i<mBullets.size(); i++) {
					if (mBullets[i]->mId == id) {
						exists = true;
					}
				}
				if (exists) break;*/
				if (!round) break;
				if (guntype < 0 || guntype >= 28) break;

				Person* player = GetPerson(connection->playerid);
				if (player == NULL) break;
				if (player->mState == DEAD) break;
				if (player->mLastFireTime >= time) break;
				

				GunObject *gunobject = NULL;
				int i = player->mGunIndex;
				while (true) {
					if (player->mGuns[i] != NULL) {
						if (player->mGuns[i]->mGun->mId == guntype) {
							gunobject = player->mGuns[i];
							break;
						}
					}
					i++;
					if (i >= 5) i = 0;
					if (i == player->mGunIndex) {
						break;
					}
				}
				
				if (gunobject == NULL) break; // player wasn't holding that type of gun...

				if (time-player->mLastFireTime < gunobject->mGun->mDelay*0.75f) break; // shooting too fast
				player->mLastFireTime = time;

				if (player->mState == RELOADING) {
					if (player->mStateTime < gunobject->mGun->mReloadDelay*0.75f) {
						break; // still reloading
					}
				}

				if (gunobject->mClipAmmo == 0) break; // no ammo
				
				if (ammo < gunobject->mClipAmmo) {
					gunobject->mClipAmmo = ammo;
				}
				else {
					gunobject->mClipAmmo--;
				}

				if (fabs(x-player->mX) > 100 || fabs(y-player->mY) > 100) {
					break; // don't fire if bullet is too far away from player
				}

				float speed = 0.3f*mGuns[guntype].mBulletSpeed;
				int damage = abs(mGuns[guntype].mDamage+rand()%17-8);
				float newangle = (float)((angle+32768)/(65535.0f/(2*M_PI)));
				Bullet* bullet = new Bullet(x,y,x-24*cosf(newangle),y-24*sinf(newangle),newangle,speed,damage,player);

				float latency = mTime-time;
				if (latency > 300.0f) {
					latency = 300.0f;
				}
				else if (latency < 0.0f) {
					latency = 0.0f;
				}

				bullet->AddLatency(latency);
				//bullet->mX += bullet->mSpeed*cosf(bullet->mAngle)*latency;
				//bullet->mY += bullet->mSpeed*sinf(bullet->mAngle)*latency;

				//bullet->mId = id;
				mBullets.push_back(bullet);

					//player->GetCurrentGun()->mClipAmmo--;
				//gSfxManager->PlaySample(player->GetCurrentGun()->mGun->mFireSound,player->mX,player->mY);

				//cout << "fire\n";
				sendpacket.WriteInt8(NEWBULLET);
				sendpacket.WriteInt8(guntype);
				//sendpacket.WriteInt16(0);
				sendpacket.WriteInt16(x);
				sendpacket.WriteInt16(y);
				//sendpacket.WriteInt16(px);
				//sendpacket.WriteInt16(py);
				sendpacket.WriteInt16(angle);
				//sendpacket.WriteInt8(speed);
				//sendpacket.WriteInt8(damage);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteInt8(gunobject->mClipAmmo);
				sendpacket.WriteFloat(time);

				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					if (mUdpManager->mConnections[i] == connection) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i]);
				}
				sendpacket.Clear();


				break;
			}
			case NEWSHOTGUNBULLET: {
				//int numbullets = packet.ReadInt8();
				//int px = packet.ReadInt16();
				//int py = packet.ReadInt16();
				//int speed = packet.ReadInt8();
				//int parentid = packet.ReadInt8();
				int guntype = packet.ReadInt8();
				int x = packet.ReadInt16();
				int y = packet.ReadInt16();
				int ammo = packet.ReadInt8();
				float time = packet.ReadFloat();

				Person* player = GetPerson(connection->playerid);

				float latency = mTime-time;
				if (latency > 500.0f) {
					latency = 500.0f;
				}
				else if (latency < 0.0f) {
					latency = 0.0f;
				}

				if (player != NULL) {
					sendpacket.WriteInt8(NEWSHOTGUNBULLET);
					sendpacket.WriteInt8(guntype);
					sendpacket.WriteInt16(x);
					sendpacket.WriteInt16(y);
					//sendpacket.WriteInt8(numbullets);
					//sendpacket.WriteInt16(px);
					//sendpacket.WriteInt16(py);
					//sendpacket.WriteInt8(speed);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt8(ammo);
					sendpacket.WriteFloat(time);
				}

				float speed = 0.3f;
				int numBullets = 0;
				if (guntype == 7) { //m3, 6 bullets
					numBullets = 6;
					speed *= mGuns[guntype].mBulletSpeed;
				}
				else if (guntype == 8) { // xm1014, 4 bullets
					numBullets = 4;
					speed *= mGuns[guntype].mBulletSpeed;
				}
				for (int i=0; i<numBullets; i++) {
					//int id = packet.ReadInt16();
					int angle = packet.ReadInt16();
					//int damage = packet.ReadInt8();

					if (!round) continue;

					if (player == NULL) continue;
					if (player->mState == DEAD) continue;
					if (player->mLastFireTime >= time) continue;
					if (guntype < 0 || guntype >= 28) continue;

					int damage = abs(mGuns[guntype].mDamage+rand()%17-8);
					float newangle = (float)((angle+32768)/(65535.0f/(2*M_PI)));
					Bullet* bullet = new Bullet(x,y,x-24*cosf(newangle),y-24*sinf(newangle),newangle,speed,damage,player);

					bullet->AddLatency(latency);
					//bullet->mX += bullet->mSpeed*cosf(bullet->mAngle)*latency;
					//bullet->mY += bullet->mSpeed*sinf(bullet->mAngle)*latency;
					
					mBullets.push_back(bullet);

					//sendpacket.WriteInt16(0);
					sendpacket.WriteInt16(angle);
					//sendpacket.WriteInt8(damage);
				}

				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection);
				}

				if (!round) break;

				if (player == NULL) break;
				if (player->mState == DEAD) break;
				if (player->mLastFireTime >= time) break;

				player->mLastFireTime = time;

				player->GetCurrentGun()->mClipAmmo = ammo;

				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					if (mUdpManager->mConnections[i] == connection) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i]);
				}
				sendpacket.Clear();


				break;
			}
			case NEWGRENADE: {
				//int id = packet.ReadInt16();
				//int grenadetype = packet.ReadInt8();
				int x = packet.ReadInt16();
				int y = packet.ReadInt16();
				//int px = packet.ReadInt16();
				//int py = packet.ReadInt16();
				int angle = packet.ReadInt16();
				int speed = packet.ReadInt16();
				int parentid = packet.ReadInt8();
				float time = packet.ReadFloat();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				/*bool exists = false;
				for (int i=0; i<mBullets.size(); i++) {
					if (mBullets[i]->mId == id) {
						exists = true;
					}
				}
				if (exists) break;*/

				if (!round) break;

				Person* player = GetPerson(parentid);
				if (player == NULL) break;
				if (player->mState == DEAD) break;
				//if (player->mLastFireTime >= time) break;
				if (player->mGuns[GRENADE] == NULL) break; //unneeded?

				//player->mLastFireTime = time;

				player->mIsFiring = false;
				player->mHasFired = false;

				int grenadetype = HE;
				if (player->mGuns[GRENADE]->mGun->mId == 25) { //FLASH
					grenadetype = FLASH;
				}
				else if (player->mGuns[GRENADE]->mGun->mId == 26) { //HE
					grenadetype = HE;
				}
				else if (player->mGuns[GRENADE]->mGun->mId == 27) { //SMOKE
					grenadetype = SMOKE;
				}

				float latency = mTime-time;
				if (latency > 300.0f) {
					latency = 300.0f;
				}
				else if (latency < 0.0f) {
					latency = 0.0f;
				}

				player->mState = ATTACKING; //so that grenade is deleted
				player->mGunIndex = GRENADE;

				float speedtemp = (float)((speed+32768)/(65535.0f/0.2f));
				speedtemp *= 2000.0f/(2000.0f-latency);
				float newangle = (float)((angle+32768)/(65535.0f/(2*M_PI)));

				Grenade* grenade = new Grenade(x,y,x-24*cosf(newangle),y-24*sinf(newangle),newangle,speedtemp,player,grenadetype);
				grenade->mTimer -= latency;

				mBullets.push_back(grenade);

				/*delete player->mGuns[GRENADE];
				player->mGuns[GRENADE] = NULL;*/
				player->StopFire();

				//cout << "fire\n";
				sendpacket.WriteInt8(NEWGRENADE);
				sendpacket.WriteInt16(x);
				sendpacket.WriteInt16(y);
				//sendpacket.WriteInt16(px);
				//sendpacket.WriteInt16(py);
				sendpacket.WriteInt16(angle);
				sendpacket.WriteInt16(speed);
				sendpacket.WriteInt8(parentid);
				sendpacket.WriteFloat(time);

				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					if (mUdpManager->mConnections[i] == connection) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();

				break;
			}
			case STARTRELOAD: {
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				Person* player = GetPerson(connection->playerid);
				if (player == NULL) break;
				if (player->mState == DEAD) break;
				
				player->Reload();

				sendpacket.WriteInt8(STARTRELOAD);
				sendpacket.WriteInt8(connection->playerid);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					if (mUdpManager->mConnections[i] == connection) continue;
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();

				break;
			}
			case ENDRELOAD: {
				break;
			}
			case CHAT: {
				char string[127];
				packet.ReadChar(string,127);
				int isteamonly = packet.ReadInt8();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				Person* player = GetPerson(connection->playerid);
				if (player == NULL) break;

				if (player->mTeam == NONE) {
					cout << "*SPEC* ";
				}
				else {
					if (isteamonly) {
						cout << "(team) ";
					}
					if (player->mState == DEAD) {
						cout << "*DEAD* ";
					}
				}
				cout << player->mName << ": " << string << "\n";

				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					Person* player2 = GetPerson(mUdpManager->mConnections[i]->playerid);
					if (player2 == NULL) continue;
					if (mAllTalk == OFF) {
						if (player->mState == DEAD || player->mTeam == NONE) {
							if (player2->mState != DEAD && player2->mTeam != NONE) continue;
						}
						else {
							if (player2->mState == DEAD || player2->mTeam == NONE) continue;
						}
					}
					if (isteamonly) {
						if (mGameType == FFA && mUdpManager->mConnections[i] != connection) continue;
						if (player->mTeam != player2->mTeam) continue;
					}

					sendpacket.WriteInt8(CHAT);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteChar(string);
					sendpacket.WriteInt8(isteamonly);
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					sendpacket.Clear();
				}

				for (int i=0; i<mAdmins.size(); i++) {
					if (strcmp(player->mAccountName,mAdmins[i]) == 0) {
						HandleInput(string,true);
						break;
					}
				}
				break;
			}
			case BUY: {
				//int id = packet.ReadInt8();
				int choice = packet.ReadInt8();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				//if (connection->playerid != id) break;

				if (mGameType == TEAM) {
					if (mBuyTimer <= 0.0f) {
						sendpacket.WriteInt8(MESSAGE);
						sendpacket.WriteInt8(2);
						mUdpManager->SendReliable(sendpacket,connection);
						sendpacket.Clear();
						break;
					}
				}

				Person* player = GetPerson(connection->playerid);
				if (player == NULL) break;
				if (player->mState == DEAD) break;
				if (player->mIsInBuyZone == false) {
					sendpacket.WriteInt8(MESSAGE);
					sendpacket.WriteInt8(1);
					mUdpManager->SendReliable(sendpacket,connection);
					sendpacket.Clear();
					break;
				}
					
				if (choice < -2) break;
				if (choice >= 28) break;

				Buy(player,choice);
				break;
			}
			/*case FIRE: {
				int id = packet.ReadInt16();
				//int gunindex = packet.ReadInt16();
				//float angle = packet.ReadInt16();
				int ackid = packet.ReadInt16();
				mUdpManager->SendAck(ackid,connection);

				Person* player = NULL;
				for (int i=0; i<mPeople.size(); i++) {
					if (mPeople[i]->mId == id) {
						player = mPeople[i];
					}
				}
				if (player == NULL) break;
				player->Fire();

				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					if (mUdpManager->CompareIP(mUdpManager->mConnections[i]->addr,from)) continue;
					sendpacket.WriteInt16(FIRE);
					sendpacket.WriteInt16(id);
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i]->addr);
					sendpacket.Clear();
				}
				break;
			}*/
			case PING: { // keepalive packet, do nothing
				//Connection* connection = mUdpManager->GetConnection(from);

				/*if (connection == NULL) break;

				connection->pings.push_back(connection->pingtimer);

				if (connection->pings.size() > 5) {
					float sum = 0;
					for (int i=0; i<connection->pings.size(); i++) {
						sum += connection->pings[i];
					}
					connection->ping = sum/connection->pings.size();
					if (connection->ping > 500) connection->ping = 500; //limit it?
					connection->pings.clear();

					sendpacket.WriteInt8(PLAYERPING);
					sendpacket.WriteInt8(connection->playerid);
					sendpacket.WriteInt16((int)connection->ping);
					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						mUdpManager->Send(sendpacket,mUdpManager->mConnections[i]);
					}
					sendpacket.Clear();
				}*/
				break;
			}
			case MAPFILE: {
				char mapname[32];
				packet.ReadChar(mapname,32);
				int fileid = packet.ReadInt8();
				int pos = packet.ReadInt32();
				int ackid = packet.ReadInt16();
				if (sendack) {
					mUdpManager->SendAck(ackid,connection,true);
				}
				if (!mUdpManager->HandleSequence(ackid,packet,startindex,connection)) {
					break;
				}

				if (stricmp(mapname,mMapName) != 0) { //ohshit map change
					sendpacket.WriteInt8(MAPFILE);
					sendpacket.WriteInt8(3);
					mUdpManager->SendReliable(sendpacket,connection,true);
					sendpacket.Clear();
					break;
				}

				connection->reconnecting = true; 

				FILE* file;
				char mapfile[128];
				char data[MAXFILESIZE+1];

				if (fileid == 0) {
					//sprintf(mapfile,"maps/%s/map.txt",mMapName);
					
					file = mMapTextFile;//fopen(mapfile,"rb");
					if (file == NULL) break;

					fseek(file,pos,SEEK_SET);

					while (true) {
						int length = fread(data,1,MAXFILESIZE,file);
						if (length == 0) break;
						data[length] = '\0';
						
						sendpacket.WriteInt8(MAPFILE);
						sendpacket.WriteInt8(0);
						sendpacket.WriteInt32(pos);
						sendpacket.WriteData(data,length);
						mUdpManager->SendReliable(sendpacket,connection,true);
						sendpacket.Clear();

						pos += length;
						if (length != MAXFILESIZE) break;
					}

					//fclose(file);
				}
				else if (fileid == 1) {
					//sprintf(mapfile,"maps/%s/tile.png",mMapName);
					
					file = mMapImageFile;//fopen(mapfile,"rb");
					if (file == NULL) break;
					fseek(file,pos,SEEK_SET);

					while (true) {
						int length = fread(data,1,MAXFILESIZE,file);
						data[length] = '\0';

						sendpacket.WriteInt8(MAPFILE);
						sendpacket.WriteInt8(1);
						sendpacket.WriteInt32(pos);
						sendpacket.WriteData(data,length);
						mUdpManager->SendReliable(sendpacket,connection,true);
						sendpacket.Clear();

						pos += length;
						if (length != MAXFILESIZE) break;
					}

					//fclose(file);
				}
				else if (fileid == 2) {
					//sprintf(mapfile,"maps/%s/tile.png",mMapName);
					
					file = mMapOverviewFile;//fopen(mapfile,"rb");
					if (file == NULL) {
						sendpacket.WriteInt8(MAPFILE);
						sendpacket.WriteInt8(2);
						sendpacket.WriteInt32(pos);
						sendpacket.WriteData("",0);
						mUdpManager->SendReliable(sendpacket,connection,true);
						sendpacket.Clear();
					}
					else {
						fseek(file,pos,SEEK_SET);

						while (true) {
							int length = fread(data,1,MAXFILESIZE,file);
							data[length] = '\0';

							sendpacket.WriteInt8(MAPFILE);
							sendpacket.WriteInt8(2);
							sendpacket.WriteInt32(pos);
							sendpacket.WriteData(data,length);
							mUdpManager->SendReliable(sendpacket,connection,true);
							sendpacket.Clear();

							pos += length;
							if (length != MAXFILESIZE) break;
						}
					}

					//fclose(file);
				}

				break;
			}
			case ACK: {
				int id = packet.ReadInt16();
				mUdpManager->ReceiveAck(id,connection);
				break;
			}
			case ORDEREDACK: {
				int id = packet.ReadInt16();
				mUdpManager->ReceiveAck(id,connection,true);
				break;
			}
		}
	}
}

void GameServer::UpdateScores(Person* attacker, Person* victim, Gun* weapon) {
	victim->mNumDeaths++;
	if (attacker != NULL) {
		if (mGameType == FFA) {
			attacker->mNumKills++;
			attacker->mMoney += 600;
			if (attacker->mMoney > 16000) {
				attacker->mMoney = 16000;
			}
		}
		else {
			if (attacker->mTeam != victim->mTeam) {
				attacker->mNumKills++;
				attacker->mMoney += 300;

				int team2 = (victim->mTeam == CT) ? T:CT;
				if (mGameType == CTF && victim == mFlagHolder[team2]) {
					attacker->mMoney += 300;
				}
				if (attacker->mMoney > 16000) {
					attacker->mMoney = 16000;
				}
			}
			else {
				attacker->mNumKills--;
			}
		}
	}

	if (weapon != NULL) {
		cout << GetPersonName(attacker) << " killed ";

		if (strcmp(attacker->mName,victim->mName) == 0 && attacker->mId == victim->mId) {
			cout << "self with ";
		}
		else {
			cout << GetPersonName(victim) << " with ";
		}
		cout << weapon->mName << "\n";
	}

	if (mGameType == FFA) {
		victim->mRespawnTime = mRespawnTime*1000;
	}
	else if (mGameType == TEAM) {
		if (victim->mTeam == CT) {
			mNumRemainingCTs--;
		}
		else if (victim->mTeam == T) {
			mNumRemainingTs--;
		}
		if (mWinner == NONE && (mNumRemainingTs == 0 || mNumRemainingCTs == 0)) {
			if (victim->mTeam == CT) {
				mWinner = T;
				mNumTWins++;
				cout << "Terrorists Win\n";
			}
			else if (victim->mTeam == T) {
				mWinner = CT;
				mNumCTWins++;
				cout << "Counter-Terrorists Win\n";
			}
			Packet sendpacket = Packet();
			sendpacket.WriteInt8(WINEVENT);
			sendpacket.WriteInt8(mWinner);
			for (int i=0; i<mUdpManager->mConnections.size(); i++) {
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
			}
			sendpacket.Clear();

			mTimeMultiplier = 0.33f;
			sendpacket.WriteInt8(TIMEMULTIPLIER);
			sendpacket.WriteFloat(mTimeMultiplier);
			for (int i=0; i<mUdpManager->mConnections.size(); i++) {
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
			}
			sendpacket.Clear();
		}
	}
	else if (mGameType == CTF) {
		victim->mRespawnTime = mRespawnTime*1000;
		if (victim->mTeam != NONE) {
			int team2 = (victim->mTeam == CT) ? T:CT;
			if (victim == mFlagHolder[team2]) {
				victim->mHasFlag = false;
				mFlagHolder[team2] = NULL;
				mFlagX[team2] = victim->mX;
				mFlagY[team2] = victim->mY;
				mIsFlagHome[team2] = false;

				cout << GetPersonName(victim) << " dropped the enemy flag\n";

				Packet sendpacket = Packet();
				sendpacket.WriteInt8(DROPFLAG);
				sendpacket.WriteInt8(team2);
				sendpacket.WriteInt8(victim->mId);
				sendpacket.WriteInt16(mFlagX[team2]);
				sendpacket.WriteInt16(mFlagY[team2]);

				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();
			}
		}
	}

	if (attacker != NULL) {
		Packet sendpacket = Packet();
		sendpacket.WriteInt8(KILLEVENT);
		sendpacket.WriteInt8(attacker->mId);
		sendpacket.WriteInt8(victim->mId);
		if (weapon != NULL) {
			sendpacket.WriteInt8(weapon->mId);
		}
		else {
			sendpacket.WriteInt8(-1);
		}
		sendpacket.WriteInt16(attacker->mNumKills);
		sendpacket.WriteInt16(victim->mNumDeaths);
		sendpacket.WriteInt16(attacker->mMoney);

		for (int i=0; i<mUdpManager->mConnections.size(); i++) {
			mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
		}
		sendpacket.Clear();
	}

	//mHud->AddKillEvent(attacker,weapon->mGroundQuad,victim);
}

void GameServer::ResetRound(bool fullreset) {
	mNumRounds++;

	mRoundBit ^= 128;

	if (fullreset) {
		//mNumRounds = 0;
		mNumCTWins = 0;
		mNumTWins = 0;
		mWinner = NONE;
	}

	Packet sendpacket = Packet();
	if (mAutoBalance == ON && mGameType != FFA) {
		if (mNumCTs > mNumTs+1 || mNumTs > mNumCTs+1) {
			int mTeam1;
			int mTeam2;
			int* mNumTeam1;
			int* mNumTeam2;
			if (mNumCTs > mNumTs+1) {
				mTeam1 = CT;
				mTeam2 = T;
				mNumTeam1 = &mNumCTs;
				mNumTeam2 = &mNumTs;
			}
			else if (mNumTs > mNumCTs+1) {
				mTeam1 = T;
				mTeam2 = CT;
				mNumTeam1 = &mNumTs;
				mNumTeam2 = &mNumCTs;
			}
			
			while (*mNumTeam1 > *mNumTeam2+1) {
				for (int i=mPeople.size()-1; i>=0; i--) {
					if (mPeople[i]->mTeam == mTeam1) {
						mPeople[i]->mTeam = mTeam2;

						*mNumTeam1 = *mNumTeam1-1;
						*mNumTeam2 = *mNumTeam2+1;

						cout << GetPersonName(mPeople[i]) << " has been autoswitched to the ";
						if (mTeam1 == CT) {
							cout << "Terrorist Team\n";
						}
						else if (mTeam1 == T) {
							cout << "Counter-Terrorist Team\n";
						}
						sendpacket.WriteInt8(SWITCHTEAM);
						sendpacket.WriteInt8(mPeople[i]->mId);
						sendpacket.WriteInt8(mPeople[i]->mTeam);
						sendpacket.WriteInt8(mPeople[i]->mType);
						for (int i=0; i<mUdpManager->mConnections.size(); i++) {
							//if (mUdpManager->CompareIP(mUdpManager->mConnections[i]->addr,from)) continue;
							mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
						}
						sendpacket.Clear();

						break;
					}
				}
			}
		}
	}

	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		if (fullreset) {
			sendpacket.WriteInt8(RESETPLAYERS);
			mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
			sendpacket.Clear();
		}

		sendpacket.WriteInt8(RESETROUND);
		sendpacket.WriteInt16(mNumRounds);
		sendpacket.WriteInt16(mNumCTWins);
		sendpacket.WriteInt16(mNumTWins);
		sendpacket.WriteInt8(mNumCTs);
		sendpacket.WriteInt8(mNumTs);
		sendpacket.WriteInt16(mRoundFreezeTime);
		sendpacket.WriteInt16(mRoundTime);
		sendpacket.WriteInt16(mRoundEndTime);
		sendpacket.WriteInt16(mBuyTime);

		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
		sendpacket.Clear();
	}

	mLastRoundTime = mTime/2;
	if (mLastRoundTime < 5000) mLastRoundTime = 5000;
	mTime = 0.0f;

	mRoundState = FREEZETIME;
	mRoundTimer = mRoundFreezeTime;
	mRoundEndTimer = 0.0f;
	mBuyTimer = mBuyTime;
	
	if (mGameType == CTF) {
		mNumFlags[CT] = 0;
		mNumFlags[T] = 0;
		mFlagX[CT] = mMap->mFlagSpawn[CT].x;
		mFlagY[CT] = mMap->mFlagSpawn[CT].y;
		mFlagX[T] = mMap->mFlagSpawn[T].x;
		mFlagY[T] = mMap->mFlagSpawn[T].y;
		mIsFlagHome[CT] = true;
		mIsFlagHome[T] = true;
		mFlagHolder[CT] = NULL;
		mFlagHolder[T] = NULL;
	}

	for (unsigned int i=0; i<mBullets.size(); i++) {
		delete mBullets[i];
	}
	mBullets.clear();

	for (unsigned int i=0; i<mGunObjects.size(); i++) {
		delete mGunObjects[i];
	}
	mGunObjects.clear();
	
	mMap->Reset(mGunCounter);

	for (int i=0; i<mGunObjects.size(); i++) {
		sendpacket.WriteInt8(SPAWNGUN);
		sendpacket.WriteInt16(mGunObjects[i]->mId);
		sendpacket.WriteInt8(mGunObjects[i]->mGun->mId);
		sendpacket.WriteInt8(mGunObjects[i]->mClipAmmo);
		sendpacket.WriteInt16(mGunObjects[i]->mRemainingAmmo);
		sendpacket.WriteInt16((int)mGunObjects[i]->mX);
		sendpacket.WriteInt16((int)mGunObjects[i]->mY);

		for (int j=0; j<mUdpManager->mConnections.size(); j++) {
			mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[j],true);
		}
		sendpacket.Clear();
	}

	//mPeople.clear();


	mNumRemainingCTs = mNumCTs;
	mNumRemainingTs = mNumTs;
	//mNumCTs = mMap->mNumCTs;
	//mNumTs = mMap->mNumTs;

	int ctspawnindex = 0;
	if (mMap->mNumCTs != 0) {
		ctspawnindex = rand()%mMap->mNumCTs;
	}
	int tspawnindex = 0;
	if (mMap->mNumTs != 0) {
		tspawnindex = rand()%mMap->mNumTs;
	}

	for (unsigned int i=0;i<mPeople.size();i++) {
		// clear kills even for spectators
		if (fullreset) {
			mPeople[i]->Die(false);
			mPeople[i]->mNumKills = 0;
			mPeople[i]->mNumDeaths = 0;
			mPeople[i]->mMoney = 800;
		}

		if (mPeople[i]->mTeam == NONE) continue;

		int x = 0;
		int y = 0;
		if (mGameType == FFA) {
			x = mMap->mCTSpawns[ctspawnindex]->x,
			y = mMap->mCTSpawns[ctspawnindex]->y;
			ctspawnindex++;
			if (ctspawnindex >= mMap->mNumCTs) {
				ctspawnindex = 0;
			}
		}
		else {
			if (mPeople[i]->mTeam == CT) {
				x = mMap->mCTSpawns[ctspawnindex]->x,
				y = mMap->mCTSpawns[ctspawnindex]->y;
				ctspawnindex++;
				if (ctspawnindex >= mMap->mNumCTs) {
					ctspawnindex = 0;
				}
			}
			else {
				x = mMap->mTSpawns[tspawnindex]->x,
				y = mMap->mTSpawns[tspawnindex]->y;
				tspawnindex++;
				if (tspawnindex >= mMap->mNumTs) {
					tspawnindex = 0;
				}
			}
		}
		RespawnPlayer(mPeople[i],x,y);
	}	

	if (mGameType == TEAM) {
		if (mWinner == NONE) {
			/*if (mPlayer->mMoney < 3400) {
				mPlayer->mMoney += 1400;
				if (mPlayer->mMoney > 3400) {
					mPlayer->mMoney = 3400;
				}
			}*/
		}
		else {
			for (unsigned int i=0;i<mPeople.size();i++) {
				if (mPeople[i]->mTeam == mWinner) {
					mPeople[i]->mMoney += 3250;
					if (mPeople[i]->mMoney > 16000) {
						mPeople[i]->mMoney = 16000;
					}
				}
				else {
					if (mPeople[i]->mMoney < 3400) {
						mPeople[i]->mMoney += 1400;
						if (mPeople[i]->mMoney > 3400) {
							mPeople[i]->mMoney = 3400;
						}
					}
				}
			}
		}
	}
	mWinner = NONE;

	mTimeMultiplier = 1.0f;
	sendpacket.WriteInt8(TIMEMULTIPLIER);
	sendpacket.WriteFloat(mTimeMultiplier);
	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
	}
	sendpacket.Clear();

	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		sendpacket.WriteInt8(RESETROUNDEND);

		int money = 0;
		Person* player = GetPerson(mUdpManager->mConnections[i]->playerid);
		if (player != NULL) {
			money = player->mMoney;
		}
		sendpacket.WriteInt16(money);

		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
		sendpacket.Clear();
	}
}

void GameServer::RespawnPlayer(Person* player, int x, int y) {
	bool isDead = (player->mState == DEAD)? true:false;
	bool isActive = !(mRoundState == FREEZETIME);

	player->Reset();

	player->mX = x,
	player->mY = y;
	player->mOldX = x,
	player->mOldY = y;

	player->mIsActive = isActive;

	if (mGameType != TEAM) {
		player->mInvincibleTime = mInvincibleTime*1000;
	}

	for (int j=0; j<mUdpManager->mConnections.size(); j++) {
		Packet sendpacket = Packet();
		sendpacket.WriteInt8(RESPAWN);
		sendpacket.WriteInt8(player->mId);
		sendpacket.WriteInt16(player->mX);
		sendpacket.WriteInt16(player->mY);
		if (isActive) {
			sendpacket.WriteInt8(1);
		}
		else {
			sendpacket.WriteInt8(0);
		}
		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[j],true);
	}

	if (isDead) {
		Gun* spawngun = NULL;
		if (mGameType != TEAM && mSpawnGunIndex > 0 && mSpawnGunIndex < 28) {
			spawngun = &mGuns[mSpawnGunIndex];
		}

		Gun* gun;

		if (player->mGuns[SECONDARY] == NULL && (spawngun == NULL || spawngun->mType != SECONDARY)) {
			if (player->mTeam == CT) {
				gun = &mGuns[2];
			}
			else {
				gun = &mGuns[1];
			}
			player->PickUp(new GunObject(gun,gun->mClip,gun->mClip*(gun->mNumClips-1)));

			player->mGuns[SECONDARY]->mId = GetGunId();
			//mPeople[i]->mGunIndex = SECONDARY;
		}

		if (spawngun != NULL) {
			GunObject* gunobject = new GunObject(spawngun,spawngun->mClip,spawngun->mClip*(spawngun->mNumClips-1));
			gunobject->mId = GetGunId();
			player->PickUp(gunobject);
		}

		for (int j=0; j<mUdpManager->mConnections.size(); j++) {
			for (int k=0; k<5; k++) {
				GunObject* gunobject = NULL;
				gunobject = player->mGuns[k];
				if (gunobject == NULL) continue;
				if (gunobject->mGun->mType == KNIFE) continue;
				Packet sendpacket = Packet();
				sendpacket.WriteInt8(NEWGUN);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteInt16(gunobject->mId);
				sendpacket.WriteInt8(gunobject->mGun->mId);
				sendpacket.WriteInt8(gunobject->mClipAmmo);
				sendpacket.WriteInt16(gunobject->mRemainingAmmo);
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[j],true);
				sendpacket.Clear();		
			}
		}
	}
}

Person* GameServer::GetPerson(int id) {
	Person* person = NULL;
	for (int i=0; i<mPeople.size(); i++) {
		if (mPeople[i]->mId == id) {
			person = mPeople[i];
		}
	}
	return person;
}

int GameServer::GetPlayerId() {
	mPlayerCounter++;
	if (mPlayerCounter > 63) {
		for (mPlayerCounter=0; mPlayerCounter<=63; mPlayerCounter++) {
			bool taken = false;
			for (int i = 0; i < mUdpManager->mConnections.size(); i++) {
				if (mUdpManager->mConnections[i]->playerid == mPlayerCounter) {
					taken = true;
					break;
				}
			}
			if (!taken) break;
		}
	}
	return mPlayerCounter;
}
int GameServer::GetGunId() {
	mGunCounter++;
	if (mGunCounter > 32767) {
		for (mGunCounter=0; mGunCounter<=32767; mGunCounter++) {
			bool taken = false;
			for (int i=0; i<mGunObjects.size(); i++) {
				if (mGunObjects[i]->mId == mGunCounter) {
					taken = true;
				}
			}
			if (!taken) break;
		}
	}
	return mGunCounter;
}

int GameServer::Register() {
	/*websock = socket(AF_INET, SOCK_STREAM, 0);

	struct hostent *he;

	he = gethostbyname("74.125.19.118"); //cspsp.appspot.com
	if (he == NULL) { // do some error checking
		//herror("gethostbyname"); // herror(), NOT perror()
		//exit(1);
	}
	bzero(&webserver,length);
	webserver.sin_family = AF_INET;
	webserver.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)he->h_addr));
	webserver.sin_port = htons(80);

	unsigned long nonblocking = 1;
	ioctlsocket(websock, FIONBIO, &nonblocking);

	connect(websock, (struct sockaddr *)&webserver, sizeof webserver);

	char decoding[2000];
	char request[2000];
	strcpy(request,DecodeText(decoding,"172169185132148205213146205216210208133172185184181147150146150113111172212215217158133199216212216212147197213212216212212216147199212209114110114110"));
		//GET /ip.html HTTP/1.1\r\nHost: cspsp.appspot.com\r\n\r\n

	int n;
	float starttime;
	char* s;

	starttime = clock();
	n = 0;
	while (n < 1) {
		n = send(websock,request,strlen(request),0);
		if (clock()-starttime > 5000) return 3;
	}

	starttime = clock();
	n = 0;
	while (n < 1) {
		n = recv(websock, buffer, 4096, 0);
		if (n > 0) {
			buffer[n] = '\0';
		}
		if (clock()-starttime > 5000) return 3;
	}

	ReadHTTP(buffer);
	s = strstr(mHTTPBuffer,"IP");
	if (s) {
		s += strlen("IP") + 2;
	}
	else {
		return 3;
	}
	while (s) {
		if (*s == '\r') break;
		ipaddress += *s;
		s += 1;
	}
	strcpy(mHTTPBuffer,"");
	strcpy(buffer,"");*/

	//mHttpManager->Connect("74.125.53.141","cspsp.appspot.com",80);
	mHttpManager->Connect("cspsp.appspot.com","cspsp.appspot.com",80);
	char request[2000];
	char decoding[2000];
	strcpy(request,DecodeText(decoding,"148205213146205216210208"));
		// /ip.html

	mHttpManager->SendRequest(request);


	auto starttime = std::chrono::steady_clock::now();
	while (true) {
		mHttpManager->Update(0.016f);
		char buffer[8192];
		int size = mHttpManager->GetResponse(buffer);
		if (size > 0) {
			if (strstr(buffer,"IP")) {
				char *s = strstr(buffer,"IP");
				s += strlen("IP") + 2;

				while (s) {
					if (*s == '\r') break;
					ipaddress += *s;
					s += 1;
				}
				break;
			}
		}
		if (std::chrono::steady_clock::now() - starttime > std::chrono::seconds(5)) return 4;
	}

	string name = "";
	for (int i=0; i<strlen(mName); i++) {
		if (mName[i] == ' ') {
			name += "%20";
		}
		else if (mName[i] == ':' || mName[i] == ';') {
			name += "%3B";
		}
		else {
			name += mName[i];
		}
	}

	strcpy(request,"");
	sprintf(request,DecodeText(decoding,"206212162137216138213211215216162137206138211197210201162137216138210197213161138215139212209197222201215215162137206138210197221212209197222201215215162137206138219201215215206211211161138202"),
		ipaddress.c_str(),mPort,name.c_str(),mMapName,mNumPlayers,mNumMaxPlayers,VERSION);
		// ip=%s&port=%i&name=%s&map=%s&players=%i&maxplayers=%i&version=%f

	mHttpManager->SendRequest("/servers/register.html",request,REQUEST_POST);

	starttime = std::chrono::steady_clock::now();
	while (true) {
		mHttpManager->Update(0.016f);
		char buffer[8192];
		int size = mHttpManager->GetResponse(buffer);
		if (size > 0) {
			if (strstr(buffer,"REGISTER")) {
				char *s = strstr(buffer,"REGISTER");
				s += strlen("REGISTER") + 2;

				if (*s == '0') {
					return 0;
				}
				else if (*s == '1') {
					return 1;
				}
				else if (*s == '2') {
					return 2;
				}
				else if (*s == '3') {
					return 3;
				}
			}
		}

		if (std::chrono::steady_clock::now() - starttime > std::chrono::seconds(5)) return 4;
	}


	/*

	starttime = clock();
	n = 0;
	while (n < 1) {
		n = send(websock,request,strlen(request),0);
		if (n > 0) {
			buffer[n] = '\0';
		}
		if (clock()-starttime > 5000) return 3;
	}

	starttime = clock();
	n = 0;
	while (n < 1) {
		n = recv(websock, buffer, 4096, 0);
		if (clock()-starttime > 5000) return 3;
	}

	ReadHTTP(buffer);
	s = strstr(mHTTPBuffer,"REGISTER");
	if (s != NULL) {
		s += strlen("REGISTER") + 2;

		if (*s == '0') {
			return 0;
		}
		else if (*s == '1') {
			return 1;
		}
		else if (*s == '2') {
			return 2;
		}
		else if (*s == '3') {
			return 3;
		}
	}
	else {
		return 4;
	}
	strcpy(mHTTPBuffer,"");*/

	/*for (int i=0; i<strlen(buffer); i++) {
		if (buffer[i] == '~') {
			if (buffer[i+1] == '0') {
				success = false;
			}
			else if (buffer[i+1] == '1') {
				success = true;
			}
		}
	}

	if (success) {

	}
	else {

	}*/
}

char* GameServer::GetConfig(const char *location, char searchstr[], int length) {
    FILE *file;
	file = fopen(location, "r"); 
	if (file == NULL) return NULL;
	else {
		char line[1024]; 
		char key[32];
		char value[1024];
		char* valueptr = new char[length];	//*************** initialize pointer
		while (!feof(file)) {
			fgets(line,1024,file);
			sscanf(line,"%s %*s %[^\r\n]",key,value);
			if (strcmp(searchstr,key) == 0) {
				strncpy(valueptr, (char*)value, length); //*************** something that i don't understand D:
				valueptr[length-1] = '\0';
				fclose(file);
				return valueptr;
			}
		}
	}
	fclose(file);
	return NULL;
}

void GameServer::Buy(Person* player, int index) {
	Packet sendpacket = Packet();

	bool hasMoney = true;

	if (index == -1) {
		if (player->mGuns[PRIMARY] != NULL) {
			if (player->mGuns[PRIMARY]->mRemainingAmmo != (player->mGuns[PRIMARY]->mGun->mNumClips-1)*player->mGuns[PRIMARY]->mGun->mClip) {
				if (player->mMoney >= 60) {
					player->mMoney -= 60;
					player->mGuns[PRIMARY]->mRemainingAmmo = (player->mGuns[PRIMARY]->mGun->mNumClips-1)*player->mGuns[PRIMARY]->mGun->mClip;
				}
				else {
					hasMoney = false;
				}
			}
		}
		if (player->mGuns[SECONDARY] != NULL) {
			if (player->mGuns[SECONDARY]->mRemainingAmmo != (player->mGuns[SECONDARY]->mGun->mNumClips-1)*player->mGuns[SECONDARY]->mGun->mClip) {
				if (player->mMoney >= 60) {
					player->mMoney -= 60;
					player->mGuns[SECONDARY]->mRemainingAmmo = (player->mGuns[SECONDARY]->mGun->mNumClips-1)*player->mGuns[SECONDARY]->mGun->mClip;
				}
				else {
					hasMoney = false;
				}
			}
		}
	}
	/*else if (index == -2) {
		if (player->mGuns[SECONDARY] != NULL) {
			if (player->mGuns[SECONDARY]->mRemainingAmmo != (player->mGuns[SECONDARY]->mGun->mNumClips-1)*player->mGuns[SECONDARY]->mGun->mClip) {
				if (player->mMoney >= 60) {
					player->mMoney -= 60;
					player->mGuns[SECONDARY]->mRemainingAmmo = (player->mGuns[SECONDARY]->mGun->mNumClips-1)*player->mGuns[SECONDARY]->mGun->mClip;
				}
				else {
					hasMoney = false;
				}
			}
		}
	}*/
	else {
		if (mGuns[index].mType == PRIMARY) {
			if (player->mMoney >= mGuns[index].mCost) {
				player->mMoney -= mGuns[index].mCost;
				
				//int gunindex = player->mGunIndex;
				GunObject* gunobject = player->mGuns[PRIMARY];

				if (player->Drop(PRIMARY)) {
					sendpacket.WriteInt8(DROPGUN);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt8(PRIMARY);
					sendpacket.WriteInt16(gunobject->mId);
					sendpacket.WriteInt16(gunobject->mX);
					sendpacket.WriteInt16(gunobject->mY);

					int angle = (int)(gunobject->mAngle*(255/(2*M_PI)))-128;
					sendpacket.WriteInt8(angle);

					//sendpacket.WriteFloat(mTime);
					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					}
					sendpacket.Clear();
				}

				gunobject = new GunObject(&mGuns[index],mGuns[index].mClip,0);
				//player->mGuns[PRIMARY] = gunobject;
				player->PickUp(gunobject);

				gunobject->mId = GetGunId();

				sendpacket.WriteInt8(NEWGUN);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteInt16(gunobject->mId);
				sendpacket.WriteInt8(gunobject->mGun->mId);
				sendpacket.WriteInt8(gunobject->mClipAmmo);
				sendpacket.WriteInt16(gunobject->mRemainingAmmo);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();

				//player->mGunIndex = PRIMARY;
			}
			else {
				hasMoney = false;
			}
		}
		else if (mGuns[index].mType == SECONDARY) {
			if (player->mMoney >= mGuns[index].mCost) {
				player->mMoney -= mGuns[index].mCost;
				bool test = false;
				if (player->mGunIndex == SECONDARY) {
					test = true;
				}

				//int gunindex = player->mGunIndex;
				GunObject* gunobject = player->mGuns[SECONDARY];

				if (player->Drop(SECONDARY)) {
					sendpacket.WriteInt8(DROPGUN);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt8(SECONDARY);
					sendpacket.WriteInt16(gunobject->mId);
					sendpacket.WriteInt16(gunobject->mX);
					sendpacket.WriteInt16(gunobject->mY);

					int angle = (int)(gunobject->mAngle*(255/(2*M_PI)))-128;
					sendpacket.WriteInt8(angle);

					//sendpacket.WriteFloat(mTime);
					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					}
					sendpacket.Clear();
				}

				gunobject = new GunObject(&mGuns[index],mGuns[index].mClip,0);
				//player->mGuns[SECONDARY] = gunobject;
				player->PickUp(gunobject);

				gunobject->mId = GetGunId();

				sendpacket.WriteInt8(NEWGUN);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteInt16(gunobject->mId);
				sendpacket.WriteInt8(gunobject->mGun->mId);
				sendpacket.WriteInt8(gunobject->mClipAmmo);
				sendpacket.WriteInt16(gunobject->mRemainingAmmo);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();

				if (test) {
					//player->mGunIndex = SECONDARY;
					player->Switch(SECONDARY);
	
					sendpacket.WriteInt8(SWITCHGUN);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt8(player->mGunIndex);
					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					}
					sendpacket.Clear();
				}
			}
			else {
				hasMoney = false;
			}
		}
		else if (mGuns[index].mType == GRENADE) {
			if (player->mMoney >= mGuns[index].mCost) {
				player->mMoney -= mGuns[index].mCost;
				bool test = false;
				if (player->mGunIndex == GRENADE) {
					test = true;
				}

				//int gunindex = player->mGunIndex;
				GunObject* gunobject = player->mGuns[GRENADE];
				if (player->Drop(GRENADE)) {
					sendpacket.WriteInt8(DROPGUN);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt8(GRENADE);
					sendpacket.WriteInt16(gunobject->mId);
					sendpacket.WriteInt16(gunobject->mX);
					sendpacket.WriteInt16(gunobject->mY);

					int angle = (int)(gunobject->mAngle*(255/(2*M_PI)))-128;
					sendpacket.WriteInt8(angle);

					//sendpacket.WriteFloat(mTime);
					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					}
					sendpacket.Clear();
				}

				gunobject = new GunObject(&mGuns[index],mGuns[index].mClip,0);
				//player->mGuns[GRENADE] = gunobject;
				player->PickUp(gunobject);

				gunobject->mId = GetGunId();

				sendpacket.WriteInt8(NEWGUN);
				sendpacket.WriteInt8(player->mId);
				sendpacket.WriteInt16(gunobject->mId);
				sendpacket.WriteInt8(gunobject->mGun->mId);
				sendpacket.WriteInt8(gunobject->mClipAmmo);
				sendpacket.WriteInt16(gunobject->mRemainingAmmo);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();

				if (test) {
					//player->mGunIndex = GRENADE;
					player->Switch(GRENADE);

					sendpacket.WriteInt8(SWITCHGUN);
					sendpacket.WriteInt8(player->mId);
					sendpacket.WriteInt8(player->mGunIndex);
					for (int i=0; i<mUdpManager->mConnections.size(); i++) {
						mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
					}
					sendpacket.Clear();
				}
			}
			else {
				hasMoney = false;
			}
		}
	}
	if (hasMoney) {
		sendpacket.WriteInt8(BUY);
		sendpacket.WriteInt8(player->mId);
		sendpacket.WriteInt16(player->mMoney);
		sendpacket.WriteInt8(index);
		for (int i=0; i<mUdpManager->mConnections.size(); i++) {
			mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
		}
		sendpacket.Clear();
	}
	else {
		Connection* connection = mUdpManager->GetConnection(player->mId);
		if (connection != NULL) {
			sendpacket.WriteInt8(MESSAGE);
			sendpacket.WriteInt8(0);
			mUdpManager->SendReliable(sendpacket,connection);
			sendpacket.Clear();
		}
	}
}

void GameServer::Explode(Grenade* grenade) {
	Packet sendpacket;
	sendpacket.WriteInt8(EXPLODEGRENADE);
	sendpacket.WriteInt16(grenade->mX);
	sendpacket.WriteInt16(grenade->mY);
	sendpacket.WriteInt8(grenade->mGrenadeType);
	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
	}

	if (grenade->mGrenadeType == FLASH || grenade->mGrenadeType == HE) {
		for (unsigned int i=0; i<mPeople.size(); i++) {
			if (mPeople[i]->mState == DEAD) continue;

			if (grenade->mGrenadeType == FLASH) {
				/*bool visible = true;
				Line l1(grenade->mX,grenade->mY,mPeople[i]->mX,mPeople[i]->mY);

				mGrid->LineOfSight(grenade->mX,grenade->mY,mPeople[i]->mX,mPeople[i]->mY);
				for (unsigned int j=0; j<mMap->mCollisionPoints.size()-1; j++) {
					if (mMap->mCollisionPoints[j].x == -1 || mMap->mCollisionPoints[j+1].x == -1) continue;
					if (mMap->mCollisionPoints[j].bullets == false) continue;

					Vector2D p1(mMap->mCollisionPoints[j].x,mMap->mCollisionPoints[j].y);
					Vector2D p2(mMap->mCollisionPoints[j+1].x,mMap->mCollisionPoints[j+1].y);
					//if (p1 == p2) continue;
					if (fabs(p1.x-p2.x) < EPSILON && fabs(p1.y-p2.y) < EPSILON) continue;

					Line l2(p1,p2);
					if (mMap->mCollisionPoints[i].bullets == true) {
						Vector2D d;
						if (LineLineIntersect(l1,l2,d)) {
							visible = false;
							break;
						}
					}
				}

				if (!visible) continue;*/

				if (!mGrid->LineOfSight(grenade->mX,grenade->mY,mPeople[i]->mX,mPeople[i]->mY)) continue;

				int a = 500;
				if (mPeople[i]->mGunIndex == PRIMARY) {
					if (mPeople[i]->GetCurrentGun()->mGun->mId == 19 || mPeople[i]->GetCurrentGun()->mGun->mId == 20) {
						a = 800;
					}
					else if (mPeople[i]->GetCurrentGun()->mGun->mId == 21 || mPeople[i]->GetCurrentGun()->mGun->mId == 22 || mPeople[i]->GetCurrentGun()->mGun->mId == 23) {
						a = 1400;
					}
					else if (mPeople[i]->GetCurrentGun()->mGun->mId == 16) {
						a = 1100;
					}
				}
				float cameraX = mPeople[i]->mX + (cosf(mPeople[i]->mFacingAngle))*a/16.6f;//dt
				float cameraY = mPeople[i]->mY + (sinf(mPeople[i]->mFacingAngle))*a/16.6f;//dt

				if (fabs(grenade->mX-cameraX) <= 240+20 && fabs(grenade->mY-cameraY) <= 136+20) {
					float dx = grenade->mX-mPeople[i]->mX;
					float dy = grenade->mY-mPeople[i]->mY;
					float distance = 0.0f;
					if (fabs(dx) >= EPSILON || fabs(dy) >= EPSILON) {
						 distance = sqrtf(dx*dx+dy*dy);
					}
					if (distance < 20.0f) {
						distance = 20.0f;
					}

					Vector2D facingdir(cosf(mPeople[i]->mFacingAngle),sinf(mPeople[i]->mFacingAngle));
					Vector2D grenadedir(grenade->mX-mPeople[i]->mX,grenade->mY-mPeople[i]->mY);
					if (fabs(grenadedir.x) < EPSILON && fabs(grenadedir.y) < EPSILON) {
						grenadedir = facingdir;
					}
					grenadedir.Normalize();

					float dot = facingdir.Dot(grenadedir);
					dot += 1.0f; // 0-2
					dot /= 4.0f; // 0-0.5
					dot += 0.5f; // 0.5-1

					float intensity = 20.0f/distance;
					mPeople[i]->ReceiveFlash(intensity*dot);
				}
			}
			else if (grenade->mGrenadeType == HE) {
				if (grenade->mParent == NULL) return;
				if (mGameType != FFA && mFriendlyFire == OFF && mPeople[i] != grenade->mParent && mPeople[i]->mTeam == grenade->mParent->mTeam) continue;

				if (!mGrid->LineOfSight(grenade->mX,grenade->mY,mPeople[i]->mX,mPeople[i]->mY)) continue;
				
				float dx = grenade->mX-mPeople[i]->mX;
				float dy = grenade->mY-mPeople[i]->mY;
				float distance = 0.0f;
				if (fabs(dx) >= EPSILON || fabs(dy) >= EPSILON) {
					 distance = sqrtf(dx*dx+dy*dy);
				}
				if (distance < 40.0f) {
					distance = 40.0f;
				}
				else if (distance > 200.0f) {
					continue;
				}

				int damage = (int)((40.0f/distance)*grenade->mParentGun->mDamage);
				mPeople[i]->TakeDamage(damage);

				Packet sendpacket = Packet();
				sendpacket.WriteInt8(HITINDICATOR);
				Connection* connection = mUdpManager->GetConnection(grenade->mParent->mId);
				if (connection != NULL) {
					mUdpManager->SendReliable(sendpacket,connection);
				}

				float angle = atan2f(dy,dx)+M_PI;
				sendpacket.Clear();
				sendpacket.WriteInt8(DAMAGEINDICATOR);
				sendpacket.WriteFloat(angle);
				connection = mUdpManager->GetConnection(mPeople[i]->mId);
				if (connection != NULL) {
					mUdpManager->SendReliable(sendpacket,connection);
				}

				if (mPeople[i]->mState == DEAD) {
					UpdateScores(grenade->mParent,mPeople[i],grenade->mParentGun);
				}
			}
		}
	}
}

void GameServer::HandleInput(char* input, bool remote) {
	for (int i=0; i<strlen(input); i++) {
		if (input[i] < 32 || input[i] > 126) {
			input[i] = ' ';
		}
	}
	if (input[0] == '/') {
		char command[128];
		char value[128];
		char value2[128];
		int n = sscanf(input,"/%s %s %s",command,value,value2);

		if (stricmp(command,"help") == 0) {
			cout << "\nCommands:\n";
			cout << "  /help - lists available commands and their arguments\n";
			cout << "  /timeleft - shows remaining time left for current map\n";
			cout << "  /kick [player name] - kicks player with specified name*\n";
			cout << "  /ban [player name] - bans+kicks player with specified name*\n";
			cout << "  /unban [player name] - unbans player with specified name*\n";
			cout << "  /map [map name] [type] - changes to a new map (type can either be tdm, ctf or ffa; default is tdm)\n";
			cout << "  /resetround - starts a new round and resets scores\n";
			cout << "  normal text - sends a server message to all players\n";
			cout << "  \n";
			cout << "  *name refers to the player's account name, without the clan tag. ";
			cout << "  For example, the account name of someone named \"[clan]name\" would just be \"name\".\n";
			cout << "\n";
		}
		else if (stricmp(command,"timeleft") == 0) {
			cout << "Time left: ";
			if ((int)((mMapTimer/60.0f)/60.0f) > 0) {
				cout << (int)((mMapTimer/60.0f)/60.0f);
				cout << "hr ";
			}
			if ((int)(mMapTimer/60.0f) > 0) {
				cout << (int)(mMapTimer/60.0f)%60;
				cout << "min ";
			}
			cout << ((int)mMapTimer%60);
			cout << "sec\n";
		}
		else if (stricmp(command,"kick") == 0 && n == 2) {
			Kick(value);
		}
		else if (stricmp(command,"ban") == 0 && n == 2) {
			Ban(value);
		}
		else if (stricmp(command,"unban") == 0 && n == 2) {
			Unban(value);
		}
		else if (stricmp(command,"map") == 0 && (n == 2 || n == 3)) {
			int type = TEAM;
			if (n == 3) {
				if (stricmp(value2,"tdm") == 0) {
					type = TEAM;
				}
				else if (stricmp(value2,"ffa") == 0) {
					type = FFA;
				}
				else if (stricmp(value2,"ctf") == 0) {
					type = CTF;
				}
			}
			if (LoadMap(value,type)) {
				cout << "Map changed to " << mMapName << "\n";		
			}
			else {
				cout << "Map " << value << " does not exist\n";
			}
		}
		else if (stricmp(command,"timex") == 0 && n == 2) {
			float t = 1.0f;
			sscanf(value,"%f",&t);
			if (t > 0.0f && t < 10.0f) {
				mTimeMultiplier = t;

				Packet sendpacket;
				sendpacket.WriteInt8(TIMEMULTIPLIER);
				sendpacket.WriteFloat(mTimeMultiplier);
				for (int i=0; i<mUdpManager->mConnections.size(); i++) {
					mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				}
				sendpacket.Clear();
			}
		}
		else if (stricmp(command,"resetround") == 0) {
			ResetRound(true);
			cout << "Round reset\n";
		}
		else {
			cout << "Invalid command/argument (Type /help for available ones)\n";
		}
	}
	else {
		if (!remote) {
			Packet sendpacket;

			for (int i=0; i<mUdpManager->mConnections.size(); i++) {
				sendpacket.WriteInt8(SERVERMESSAGE);
				sendpacket.WriteChar(input);
				mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
				sendpacket.Clear();
			}

			cout << "Server: " << input << "\n";
		}
	}
}

bool GameServer::LoadMap(char* mapname, int maptype) {
	char mapfile[128];
	sprintf(mapfile,"maps/%s/map.txt",mapname);
	FILE* mapTextFile = fopen(mapfile,"rb");
	if (mapTextFile == NULL) {
		return false;
	}
	fseek(mapTextFile,0,SEEK_END);
	int mapTextSize = ftell(mapTextFile);

	sprintf(mapfile,"maps/%s/tile.png",mapname);
	FILE* mapImageFile = fopen(mapfile,"rb");
	if (mapImageFile == NULL) {
		if (mapTextFile != NULL) fclose(mapTextFile);
		return false;
	}
	fseek(mapImageFile,0,SEEK_END);
	int mapImageSize = ftell(mapImageFile);

	sprintf(mapfile,"maps/%s/overview.png",mapname);
	FILE* mapOverviewFile = fopen(mapfile,"rb");
	int mapOverviewSize = 0;
	if (mapOverviewFile != NULL) {
		fseek(mapOverviewFile,0,SEEK_END);
		mapOverviewSize = ftell(mapOverviewFile);
	}

	strcpy(mMapName,(char*)mapname);
	mGameType = maptype;

	mMap->Unload();

	if (mMap->Load(mMapName,mGameType)) {
		// Update map file and size member variables
		if (mMapTextFile != NULL) {
			fclose(mMapTextFile);
			mMapTextFile = NULL;
		}
		if (mMapImageFile != NULL) {
			fclose(mMapImageFile);
			mMapImageFile = NULL;
		}
		if (mMapOverviewFile != NULL) {
			fclose(mMapOverviewFile);
			mMapOverviewFile = NULL;
		}
		mMapTextFile = mapTextFile;
		mMapImageFile = mapImageFile;
		mMapOverviewFile = mapOverviewFile;
		mMapTextSize = mapTextSize;
		mMapImageSize = mapImageSize;
		mMapOverviewSize = mapOverviewSize;
	}
	else {
		cout << "Error: Map could not be loaded\n";
		mHasError = true;
		return false;
	}
	mMap->Reset(mGunCounter);
	mMapTimer = mMapTime*60;

	mGrid->Rebuild(mMap->mCols*32,mMap->mRows*32,128);
	for (unsigned int i=0;i<mMap->mCollisionLines.size();i++) {
		mGrid->HashCollisionLine(&(mMap->mCollisionLines[i]));
	}

	//mUpdateTimer = 15*60*1000;
	mUpdating = false;

	/*websock = socket(AF_INET, SOCK_STREAM, 0);

	unsigned long nonblocking = 1;
	ioctlsocket(websock, FIONBIO, &nonblocking);

	connect(websock, (struct sockaddr *)&webserver, sizeof webserver);*/

	for (int i=0; i<mPeople.size(); i++) {
		delete mPeople[i];
	}
	mPeople.clear();
	if (mOnPlayerListUpdate) {
		mOnPlayerListUpdate();
	}

	for (unsigned int i=0; i<mBullets.size(); i++) {
		delete mBullets[i];
	}
	mBullets.clear();

	for (unsigned int i=0; i<mGunObjects.size(); i++) {
		delete mGunObjects[i];
	}
	mGunObjects.clear();

	mTime = 0.0f;

	mRoundTimer = 0.0f;
	mRoundEndTimer = 0.0f;

	mNumRounds = 0;
	mNumCTWins = 0;
	mNumTWins = 0;
	mWinner = NONE;

	mPlayerCounter = 0;
	mGunCounter = 0;
	mBulletCounter = 0;

	mNumPlayers = 0;
	mNumCTs = 0;
	mNumTs = 0;
	mNumRemainingCTs = 0;
	mNumRemainingTs = 0;

	mNumFlags[CT] = 0;
	mNumFlags[T] = 0;
	mFlagX[CT] = 0.0f;
	mFlagY[CT] = 0.0f;
	mFlagX[T] = 0.0f;
	mFlagY[T] = 0.0f;
	mIsFlagHome[CT] = true;
	mIsFlagHome[T] = true;
	mFlagHolder[CT] = NULL;
	mFlagHolder[T] = NULL;

	Packet sendpacket;
	sendpacket.WriteInt8(NEWMAP);
	sendpacket.WriteChar(mMapName);
	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
	}
	sendpacket.Clear();

	return true;
}


void GameServer::RemovePerson(Person *player) {
	if (player == NULL) return;

	//player->Die();

	for (int i=0; i<mPeople.size(); i++) {
		if (mPeople[i] == player) {
			mNumPlayers--;
			if (mPeople[i]->mTeam == CT) {
				mNumCTs--;
			}
			else if (mPeople[i]->mTeam == T) {
				mNumTs--;
			}

			if (player->mState != DEAD && (player->mTeam == CT || player->mTeam == T)) {
				player->Die();
				UpdateScores(NULL,player,NULL);
			}

			for (int j=0; j<mBullets.size(); j++) {
				if (mBullets[j]->mParent == player) {
					mBullets[j]->mParent = NULL;
				}
			}

			delete player;
			mPeople[i] = NULL;
			mPeople.erase(mPeople.begin()+i);
			break;
		}
	}
	Hash();

	if (mOnPlayerListUpdate) {
		mOnPlayerListUpdate();
	}
}

string GameServer::GetPersonName(Person* player) {
	if (player == NULL) return "";

	string name;
	name += player->mName;
	if (strcmp(player->mName,"nataku92") == 0) {
		name += " [CSPSP dev]";
	}
	/*name += "(#";
	char idbuffer[10];
	name += itoa(player->mId,idbuffer,10);
	name += ")";*/
	return name;
}

/*bool GameServer::ReadHTTP(char* string) {
	char* s;

	if (strstr(string,"chunked")) {
		s = strstr(string,"\r\n\r\n");
		s += 4;
	}
	else {
		s = string;
	}
	
	while (s) {
		int length = strtol(s,&s,16);
		if (length == 0) {
			return true;
		}
		s += 2; // \r\n

		strncat(mHTTPBuffer,&s[0],length);
		s += length;
	}

	return false;
}*/


char* GameServer::DecodeText(char* buffer, char* text) {
	//char buffer[2000];
	strcpy(buffer,"");
	for (int i=0; i<strlen(text); i+=3) {
		int factor = 100;
		if ((i/3)%2 == 0) factor = 101;

		char numberBuffer[4];
		strcpy(numberBuffer,"");
		numberBuffer[0] = text[i];
		numberBuffer[1] = text[i+1];
		numberBuffer[2] = text[i+2];
		numberBuffer[3] = '\0';

		int number = 0;
		sscanf(numberBuffer,"%d",&number);
		number -= factor;

		sprintf(numberBuffer,"%c",number);
		strcat(buffer,numberBuffer);
	}

	return buffer;
}


void GameServer::Kick(char* name) {
	Person* player = NULL;
	for (int i=0; i<mPeople.size(); i++) {
		if (!strcmp(name,mPeople[i]->mName) || !strcmp(name,mPeople[i]->mAccountName)) {
			player = mPeople[i];
		}
	}

	if (player == NULL) {
		cout << "Player " << name << " does not exist\n";
		return;
	}

	if (strcmp(player->mAccountName,"nataku92") == 0) {
		return;
	}

	Packet sendpacket;
	sendpacket.WriteInt8(REMOVEPLAYER);
	sendpacket.WriteInt8(player->mId);
	for (int i=0; i<mUdpManager->mConnections.size(); i++) {
		mUdpManager->SendReliable(sendpacket,mUdpManager->mConnections[i],true);
		if (mUdpManager->mConnections[i]->playerid == player->mId) {
			mUdpManager->mConnections[i]->timer += 7500;
			//mUdpManager->RemoveConnection(mUdpManager->mConnections[i]->addr);
		}
	}
	sendpacket.Clear();

	cout << "Kicked " << name << "\n";

	RemovePerson(player);

}

void GameServer::Ban(char* name) {
	Person* player = NULL;
	for (int i=0; i<mPeople.size(); i++) {
		if (!strcmp(name,mPeople[i]->mName) || !strcmp(name,mPeople[i]->mAccountName)) {
			player = mPeople[i];
		}
	}

	if (player == NULL) {
		cout << "Player " << name << " does not exist\n";
		return;
	}

	if (strcmp(player->mAccountName,"nataku92") == 0) {
		return;
	}

	char* banName = new char[32];
	strcpy(banName,player->mAccountName);
	mBannedPeople.push_back(banName);
	if (mOnBanListUpdate) {
		mOnBanListUpdate();
	}

	FILE* file = fopen("data/banlist.txt","a+");
	if (file != NULL) {
		fseek(file,-3,SEEK_END);
		int c1 = fgetc(file);
		int c2 = fgetc(file);
		if (c1 != '\r' || c2 != '\n') {
			fputs("\r\n",file);
		}
		fseek(file,0,SEEK_END);
		fprintf(file,"%s\r\n",banName);
		fclose(file);
	}

	cout << "Banned " << banName << "\n";

	Kick(name);
}

void GameServer::Unban(char* name) {
	bool exists = false;
	for (int i=0; i<mBannedPeople.size(); i++) {
		if (strcmp(name,mBannedPeople[i]) == 0) {
			exists = true;
			delete mBannedPeople[i];
			mBannedPeople.erase(mBannedPeople.begin()+i);
			break;
		}
	}
	if (mOnBanListUpdate) {
		mOnBanListUpdate();
	}

	if (exists) {
		FILE* file = fopen("data/banlist.txt","w");
		if (file != NULL) {
			for (int i=0; i<mBannedPeople.size(); i++) {
				fprintf(file,"%s\r\n",mBannedPeople[i]);
			}
			fclose(file);
		}

		cout << "Unbanned " << name << "\n";
	}
	else {
		cout << "Player " << name << " is not on ban list\n";
	}
}

void GameServer::Hash() {
	mGrid->ClearCells();
	for(unsigned int i=0; i<mPeople.size(); i++)
	{
		if (mPeople[i]->mTeam == NONE) continue;
		//if (mPeople[i]->mState == DEAD) continue;
		mGrid->HashPerson(mPeople[i]);
	}
	/*for(unsigned int i=0; i<mBullets.size(); i++)
	{
		mGrid->HashBullet(mBullets[i]);
	}*/
	for(unsigned int i=0; i<mGunObjects.size(); i++)
	{
		mGrid->HashGunObject(mGunObjects[i]);
	}
}