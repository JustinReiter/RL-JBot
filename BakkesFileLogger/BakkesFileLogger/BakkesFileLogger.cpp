#include "pch.h"
#include "BakkesFileLogger.h"


BAKKESMOD_PLUGIN(BakkesFileLogger, "Gameplay Data Logger", plugin_version, PERMISSION_ALL)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void BakkesFileLogger::onLoad() {
	_globalCvarManager = cvarManager;
	cvarManager->log("Plugin loaded!");

	// Configure the current player's name
	playerName = gameWrapper->GetGameEventAsServer().GetLocalPrimaryPlayer().GetPRI().GetPlayerName().ToString();
	cvarManager->log("Set playerName to: " + playerName);

	
	/// GAME HOOKS
	
	// Log data every game tick
	gameWrapper->HookEventWithCaller<PlayerControllerWrapper>("Function TAGame.PlayerController_TA.PlayerMove",
		[this](PlayerControllerWrapper caller, void* params, std::string eventname) {
			runGameTickLog(caller);
	});

	// Set boosts inactive on every boost pickup
	gameWrapper->HookEventWithCaller<BoostWrapper>("Function TAGame.VehiclePickup_Boost_TA.Pickup",
		[this](BoostWrapper caller, void* params, std::string eventname) {
			Vector location = caller.GetLocation();
			for (std::shared_ptr<boost> boost : boosts) {
				if (boost->X == location.X && boost->Y == location.Y) {
					boost->isActive = false;
					break;
				}
			}
	});



	/// PRE/POST GAME HOOKS

	// Game end
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed",
		[this](std::string eventName) {
			closeGameLogging();
	});
	// Game start
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState",
		[this](std::string eventName) {
			initGameLogging();
	});

	
	initBoosts();
	// Potential hooks?
	// Function TAGame.Vehicle_TA.SetVehicleInput
	// Function TAGame.PlayerInput_TA.PlayerInput
	// Function TAGame.PlayerController_TA.PlayerMove
	// Function TAGame.PlayerInput_TA.PlayerInput
}

void BakkesFileLogger::onUnload() {
	// Close the output stream
	if (of.is_open()) of.close();
}

void BakkesFileLogger::closeGameLogging() {
	if (of.is_open()) of.close();
}

void BakkesFileLogger::initGameLogging() {
	// Close previous file if still opened
	if (of.is_open()) closeGameLogging();

	// Declare the file writers to output
	std::time_t t = std::time(nullptr);
	tm* time = std::localtime(&t);

	// Open file with datetime name (prevent overwriting files)
	std::ostringstream oss;
	oss << PATHDIR << std::put_time(time, "%Y-%m-%d_%H-%M-%S") << ".log";
	cvarManager->log("Opened new file at: " + oss.str());
	of.open(oss.str());

	// Last step, prepare the boosts
	refreshBoosts();
}

/// <summary>
/// Log data at every game tick for the player. This will store a single line consisting of the input and output parameters to the neural network
/// </summary>
/// <param name="caller"></param>
void BakkesFileLogger::runGameTickLog(PlayerControllerWrapper caller) {
	// Only run if player is in game and caller is non-null
	if (!gameWrapper->IsInGame() || !caller || caller.GetPRI().GetPlayerName().ToString() != playerName || !of.is_open()) return;
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
		if (car.GetPRI().GetPlayerName().ToString() == playerName) continue;

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

void BakkesFileLogger::initBoosts() {
	// Setup the big boosts
	boosts.push_back(std::make_shared<boost>(-3072.0, -4096.0, true));
	boosts.push_back(std::make_shared<boost>( 3072.0, -4096.0, true));
	boosts.push_back(std::make_shared<boost>(-3584.0,     0.0, true));
	boosts.push_back(std::make_shared<boost>( 3584.0,     0.0, true));
	boosts.push_back(std::make_shared<boost>(-3072.0,  4096.0, true));
	boosts.push_back(std::make_shared<boost>( 3072.0,  4096.0, true));

	// Setup the small boosts
	boosts.push_back(std::make_shared<boost>(    0.0, -4240.0, true));
	boosts.push_back(std::make_shared<boost>(-1792.0, -4184.0, true));
	boosts.push_back(std::make_shared<boost>( 1792.0, -4184.0, true));
	boosts.push_back(std::make_shared<boost>( -940.0, -3308.0, true));
	boosts.push_back(std::make_shared<boost>(  940.0, -3308.0, true));
	boosts.push_back(std::make_shared<boost>(    0.0, -2816.0, true));
	boosts.push_back(std::make_shared<boost>(-3584.0, -2484.0, true));
	boosts.push_back(std::make_shared<boost>( 3584.0, -2484.0, true));
	boosts.push_back(std::make_shared<boost>(-1788.0, -2300.0, true));
	boosts.push_back(std::make_shared<boost>( 1788.0, -2300.0, true));

	boosts.push_back(std::make_shared<boost>(-2048.0, -1036.0, true));
	boosts.push_back(std::make_shared<boost>(    0.0, -1024.0, true));
	boosts.push_back(std::make_shared<boost>( 2048.0, -1036.0, true));
	boosts.push_back(std::make_shared<boost>(-1024.0,     0.0, true));
	boosts.push_back(std::make_shared<boost>( 1024.0,     0.0, true));
	boosts.push_back(std::make_shared<boost>(-2048.0,  1036.0, true));
	boosts.push_back(std::make_shared<boost>(    0.0,  1024.0, true));
	boosts.push_back(std::make_shared<boost>( 2048.0,  1036.0, true));
	boosts.push_back(std::make_shared<boost>(-1788.0,  2300.0, true));
	boosts.push_back(std::make_shared<boost>( 1788.0,  2300.0, true));

	boosts.push_back(std::make_shared<boost>(-3584.0, 2484.0, true));
	boosts.push_back(std::make_shared<boost>( 3584.0, 2484.0, true));
	boosts.push_back(std::make_shared<boost>(    0.0, 2816.0, true));
	boosts.push_back(std::make_shared<boost>( -940.0, 3310.0, true));
	boosts.push_back(std::make_shared<boost>(  940.0, 3308.0, true));
	boosts.push_back(std::make_shared<boost>(-1792.0, 4184.0, true));
	boosts.push_back(std::make_shared<boost>( 1792.0, 4184.0, true));
	boosts.push_back(std::make_shared<boost>(    0.0, 4240.0, true));
}

void BakkesFileLogger::refreshBoosts() {
	for (std::shared_ptr<boost> boost : boosts) {
		boost->isActive = true;
	}
}
