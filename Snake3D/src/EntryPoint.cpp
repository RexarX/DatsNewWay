#pragma once

#include "Application.h"

int main(int argc, char** argv) {
	Snake::Log::Init();
	CORE_WARN("Started logging session!");

	std::cout << "Enter your token: ";
	std::string token;
	std::cin >> token;

	if (token.empty()) {
		CORE_ASSERT(false, "Failed to start application: no token provided!");
		return 1;
	}

	Snake::Application app("Snake3D", 1280, 720, 0, 1);
	app.ConnectToServer("https://games-test.datsteam.dev/play/snake3d", token);
	app.Run();

	return 0;
}