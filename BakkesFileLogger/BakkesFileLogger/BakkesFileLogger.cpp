#include "pch.h"
#include "BakkesFileLogger.h"


BAKKESMOD_PLUGIN(BakkesFileLogger, "Gameplay Data Logger", plugin_version, PERMISSION_ALL)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void BakkesFileLogger::onLoad() {
	_globalCvarManager = cvarManager;
	cvarManager->log("Plugin loaded!");

	// Declare the file writers to output
	std::time_t t = std::time(nullptr);
	tm *time = std::localtime(&t);
	
	// Open file with datetime name (prevent overwriting files)
	std::ostringstream oss;
	oss << std::put_time(time, "%d-%m-%Y %H-%M-%S") << ".log";
	of.open(oss.str());


	// Setup hook to run the logger on every tick
	gameWrapper->HookEventWithCaller<PlayerControllerWrapper>("Function TAGame.PlayerController_TA.PlayerMove",
		[this](PlayerControllerWrapper caller, void* params, std::string eventname) {
			runGameTickLog(caller);
	});

	gameWrapper->HookEventWithCaller<PlayerControllerWrapper>("Function TAGame.PlayerInput_TA.PlayerInput",
		[this](PlayerControllerWrapper caller, void* params, std::string eventName) {

			runGameTickLog(caller);
	});

	// Potential hooks?
	// Function TAGame.Vehicle_TA.SetVehicleInput
	// Function TAGame.PlayerInput_TA.PlayerInput
	// Function TAGame.PlayerController_TA.Driving.PlayerMove
	// Function TAGame.PlayerController_TA.PlayerMove
	// Function TAGame.PlayerInput_TA.PlayerInput
}

void BakkesFileLogger::onUnload() {
	if (of.is_open()) of.close();
}

void BakkesFileLogger::runGameTickLog(PlayerControllerWrapper caller) {
	// Only run if player is in game
	if (!gameWrapper->IsInOnlineGame() || caller) return;


}