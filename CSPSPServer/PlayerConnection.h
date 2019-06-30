#pragma once

#include <winsock.h>
#include <vector>
#include "Person.h"

class PlayerConnection
{
private:
	struct sockaddr_in addrTo;
	std::vector<Person*>* players;

protected:

public:

	PlayerConnection(struct sockaddr_in from, std::vector<Person*>* players);
	~PlayerConnection();
};
