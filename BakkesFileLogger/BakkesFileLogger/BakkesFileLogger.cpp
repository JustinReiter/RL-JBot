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
	oss << "C:/Users/Justi/Desktop/projects/JBot/BakkesFileLogger/logs/" << std::put_time(time, "%Y-%m-%d_%H-%M-%S") << ".log";
	of.open(oss.str());

	// Setup hook to run the logger on every tick
	gameWrapper->HookEventWithCaller<PlayerControllerWrapper>("Function TAGame.PlayerController_TA.PlayerMove",
		[this](PlayerControllerWrapper caller, void* params, std::string eventname) {
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
	// Close the output stream
	if (of.is_open()) of.close();
}

// TODO: figure out pitch, yaw & roll
void BakkesFileLogger::runGameTickLog(PlayerControllerWrapper caller) {
	// Only run if player is in game and caller is non-null
	if (!gameWrapper->IsInGame() || !caller || frames++ < 30) return;
	frames = 0;
	ServerWrapper server = gameWrapper->GetCurrentGameState();

	// Player input parameters
	// We need the following: position, velocity, 
	CarWrapper playerCar = gameWrapper->GetLocalCar();
	Vector location = playerCar.GetLocation();
	Vector velocity =  playerCar.GetVelocity();
	Vector angularVelocity = playerCar.GetAngularVelocity();

	bool isSuperSonic = playerCar.GetbSuperSonic();
	bool hasJumped = playerCar.GetbJumped();

	// 11 player input parameters
	of << location.X << "," << location.Y << "," << location.Z << ",";
	of << velocity.X << "," << velocity.Y << "," << velocity.Z << ",";
	of << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";
	of << isSuperSonic << "," << hasJumped << ",";


	// Opponent input parameters
	for (CarWrapper car : server.GetCars()) {
		// If car is owned by player, skip
		if (car.GetOwnerName() == playerCar.GetOwnerName()) continue;

		Vector location = car.GetLocation();
		Vector velocity = car.GetVelocity();
		Vector angularVelocity = car.GetAngularVelocity();

		bool isSuperSonic = car.GetbSuperSonic();
		bool hasJumped = car.GetbJumped();

		// 11 opponent input parameters
		of << location.X << "," << location.Y << "," << location.Z << ",";
		of << velocity.X << "," << velocity.Y << "," << velocity.Z << ",";
		of << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";
		of << isSuperSonic << "," << hasJumped << ",";
	}


	// Ball input parameters
	BallWrapper ball = server.GetBall();
	location = ball.GetLocation();
	velocity = ball.GetVelocity();
	angularVelocity = ball.GetAngularVelocity();


	// 9 ball parameters
	of << location.X << "," << location.Y << "," << location.Z << ",";
	of << velocity.X << "," << velocity.Y << "," << velocity.Z << ",";
	of << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";	


	// Player output parameters
	ControllerInput playerInput = caller.GetVehicleInput();
	of << playerInput.Steer << "," << playerInput.Throttle << "," << (playerInput.ActivateBoost || playerInput.HoldingBoost) << "," << playerInput.Jump << ",";
	of << playerInput.Pitch << "," << playerInput.Yaw << "," << playerInput.Roll << "," << playerInput.Handbrake << std::endl;
}