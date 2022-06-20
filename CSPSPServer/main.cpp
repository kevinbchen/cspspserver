#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
#include "GameServer.h"

std::string getInput() {
	std::string command;
	std::getline(std::cin, command);
	return command;
}

int main() {
	GameServer server;
	server.Init();

	std::future<std::string> future = std::async(getInput);
	auto start = std::chrono::steady_clock::now();
	while (!server.mHasError) {
		if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			std::string commandStr = future.get();
			// Copy to a mutable cstring of max length 1024 for server.HandleInput
			commandStr = commandStr.substr(0, 1024);
			int length = commandStr.length();
			char command[1025];
			std::copy(commandStr.data(), commandStr.data() + length, command);
			command[length] = '\0';
			server.HandleInput(command);
			future = std::async(getInput);
		}

		auto now = std::chrono::steady_clock::now();
		float dt = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(now - start).count();
		start = now;
		server.Update(dt);
		std::this_thread::sleep_until(now + std::chrono::milliseconds(10));
	}

	return 0;
}
