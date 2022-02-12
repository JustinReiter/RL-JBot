#include "pch.h"
#include "BakkesFileLogger.h"


BAKKESMOD_PLUGIN(BakkesFileLogger, "Gameplay Data Logger", plugin_version, PERMISSION_ALL)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void BakkesFileLogger::onLoad() {
	_globalCvarManager = cvarManager;
	cvarManager->log("Plugin loaded!");

	// Configure the current player's name
	playerName = gameWrapper->GetPlayerName().ToString();
	cvarManager->log("Set playerName to: " + playerName);

	
	/// GAME HOOKS
	
	// Log data every game tick
	gameWrapper->HookEventWithCaller<PlayerControllerWrapper>("Function TAGame.PlayerController_TA.PlayerMove",
		[this](PlayerControllerWrapper caller, void* params, std::string eventname) {
			runGameTickLog(caller);
	});

	// Set boosts inactive on every boost pickup
	gameWrapper->HookEventWithCaller<ActorWrapper>("Function TAGame.VehiclePickup_Boost_TA.Pickup",
		[this](ActorWrapper caller, void* params, std::string eventname) {
			BoostWrapper boostWrapper(caller.memory_address);
			Vector location = caller.GetLocation();
			for (boost& boost : boosts) {
				if (boost.X == location.X && boost.Y == location.Y) {
					boost.isActive = false;
					break;
				}
			}
	});

	// Set boosts active on every boost respawn
	gameWrapper->HookEventWithCaller<ActorWrapper>("Function VehiclePickup_Boost_TA.Idle.BeginState",
		[this](ActorWrapper caller, void* params, std::string eventname) {
			BoostPickupWrapper boostWrapper(caller.memory_address);
			Vector location = boostWrapper.GetLocation();
			for (boost& boost : boosts) {
				if (boost.X == location.X && boost.Y == location.Y) {
					boost.isActive = true;
					break;
				}
			}
	});



	/// PRE/POST GAME HOOKS .GetTeamNum2();

	// Game end
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed",
		[this](std::string eventName) {
			closeGameLogging();
			teamFactor = 0;
	});
	// Game start
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState",
		[this](std::string eventName) {
			initGameLogging();
	});
	// Kickoff start
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound",
		[this](std::string eventName) {
			initKickoffStart();
	});
	// Goal scored
	gameWrapper->HookEvent("Function TAGame.Ball_TA.OnHitGoal",
		[this](std::string eventName) {
			initGoalSequence();
	});

	
	initBoosts();
}

void BakkesFileLogger::onUnload() {
	// Close the output stream
	if (of.is_open()) of.close();
}

void BakkesFileLogger::closeGameLogging() {
	if (of.is_open()) {
		cvarManager->log("Closing the output stream for the following file: " + outputFileName.str());
		of.close();
	}
}

void BakkesFileLogger::initGameLogging() {
	// Close previous file if still opened
	if (of.is_open()) closeGameLogging();

	// Declare the file writers to output
	std::time_t t = std::time(nullptr);
	tm* time = std::localtime(&t);

	// Open file with datetime name (prevent overwriting files)
	outputFileName.clear();
	outputFileName << PATHDIR << std::put_time(time, "%Y-%m-%d_%H-%M-%S") << ".log";
	cvarManager->log("Opened new file at: " + outputFileName.str());
	of.open(outputFileName.str());

	// Reset team factor for new game
	teamFactor = 0;

	// Last step, prepare the boosts
	refreshBoosts();
}

/// <summary>
/// Log data at every game tick for the player. This will store a single line consisting of the input and output parameters to the neural network
/// </summary>
/// <param name="caller"></param>
void BakkesFileLogger::runGameTickLog(PlayerControllerWrapper caller) {
	// Only run if player is in game and caller is non-null
	// TODO: add check to make sure that the current player is you
	if (!gameWrapper->IsInGame() || !caller || !of.is_open() || teamFactor == 0) return;

	// Ugly null-check code to avoid try-catch
	ServerWrapper server = getServerWrapper();
	if (!server) return;
	PlayerControllerWrapper player = server.GetLocalPrimaryPlayer();
	if (!player) return;
	PriWrapper pri = player.GetPRI();
	if (!pri) return;
	if (pri.GetPlayerName().ToString() != playerName) return;

	// Player input parameters
	// We need the following: position, velocity, 
	CarWrapper playerCar = gameWrapper->GetLocalCar();
	if (!playerCar) return;
	Vector location = playerCar.GetLocation();
	Vector velocity =  playerCar.GetVelocity();
	Vector angularVelocity = playerCar.GetAngularVelocity();

	bool isSuperSonic = playerCar.GetbSuperSonic();
	bool hasJumped = playerCar.GetbJumped();

	// 11 player input parameters
	of << teamFactor * location.X << "," << teamFactor * location.Y << "," << location.Z << ",";
	of << teamFactor * velocity.X << "," << teamFactor * velocity.Y << "," << velocity.Z << ",";
	of << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";
	of << isSuperSonic << "," << hasJumped << ",";


	// Opponent input parameters
	for (CarWrapper car : server.GetCars()) {
		// If car is owned by player, skip
		// TODO implement the name or id matching
		if (car.GetPRI().GetPlayerName().ToString() == playerName) continue;

		Vector location = car.GetLocation();
		Vector velocity = car.GetVelocity();
		Vector angularVelocity = car.GetAngularVelocity();

		bool isSuperSonic = car.GetbSuperSonic();
		bool hasJumped = car.GetbJumped();

		// 11 opponent input parameters
		of << teamFactor * location.X << "," << teamFactor * location.Y << "," << location.Z << ",";
		of << teamFactor * velocity.X << "," << teamFactor * velocity.Y << "," << velocity.Z << ",";
		of << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";
		of << isSuperSonic << "," << hasJumped << ",";
	}


	// Ball input parameters
	BallWrapper ball = server.GetBall();
	location = ball.GetLocation();
	velocity = ball.GetVelocity();
	angularVelocity = ball.GetAngularVelocity();


	// 9 ball parameters
	of << teamFactor * location.X << "," << teamFactor * location.Y << "," << location.Z << ",";
	of << teamFactor * velocity.X << "," << teamFactor * velocity.Y << "," << velocity.Z << ",";
	of << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";


	// Boost parameters (each boost has the following data: {X, Y, isActive} )
	// For now there are: 3 x 34 parameters
	for (boost& boost : boosts) {
		of << boost.X << "," << boost.Y << "," << boost.isActive << ",";
	}

	// Player output parameters
	ControllerInput playerInput = caller.GetVehicleInput();
	of << playerInput.Steer << "," << playerInput.Throttle << "," << (playerInput.ActivateBoost || playerInput.HoldingBoost) << "," << playerInput.Jump << ",";
	of << playerInput.Pitch << "," << playerInput.Yaw << "," << playerInput.Roll << "," << playerInput.Handbrake << std::endl;
}

void BakkesFileLogger::initKickoffStart() {
	// Ugly null-check code to avoid try-catch
	ServerWrapper server = getServerWrapper();
	if (!server) return;
	PlayerControllerWrapper player = server.GetLocalPrimaryPlayer();
	if (!player) return;
	PriWrapper pri = player.GetPRI();
	if (!pri) return;
	if (pri.GetPlayerName().ToString() != playerName) return;
	if (teamFactor == 0) {
		teamFactor = pri.GetTeamNum2() == 0 ? 1 : -1;
		cvarManager->log("Team factor set to: " + std::to_string(teamFactor) + "... Enabling logging");
	}
}

void BakkesFileLogger::initGoalSequence() {
	teamFactor = 0;
	cvarManager->log("Goal scored... Disabling logging");
}

void BakkesFileLogger::initBoosts() {
	// Setup the big boosts
	boosts.push_back({-3072.0, -4096.0, true});
	boosts.push_back({ 3072.0, -4096.0, true});
	boosts.push_back({-3584.0,     0.0, true});
	boosts.push_back({ 3584.0,     0.0, true});
	boosts.push_back({-3072.0,  4096.0, true});
	boosts.push_back({ 3072.0,  4096.0, true});

	// Setup the small boosts
	boosts.push_back({    0.0, -4240.0, true});
	boosts.push_back({-1792.0, -4184.0, true});
	boosts.push_back({ 1792.0, -4184.0, true});
	boosts.push_back({ -940.0, -3308.0, true});
	boosts.push_back({  940.0, -3308.0, true});
	boosts.push_back({    0.0, -2816.0, true});
	boosts.push_back({-3584.0, -2484.0, true});
	boosts.push_back({ 3584.0, -2484.0, true});
	boosts.push_back({-1788.0, -2300.0, true});
	boosts.push_back({ 1788.0, -2300.0, true});

	boosts.push_back({-2048.0, -1036.0, true});
	boosts.push_back({    0.0, -1024.0, true});
	boosts.push_back({ 2048.0, -1036.0, true});
	boosts.push_back({-1024.0,     0.0, true});
	boosts.push_back({ 1024.0,     0.0, true});
	boosts.push_back({-2048.0,  1036.0, true});
	boosts.push_back({    0.0,  1024.0, true});
	boosts.push_back({ 2048.0,  1036.0, true});
	boosts.push_back({-1788.0,  2300.0, true});
	boosts.push_back({ 1788.0,  2300.0, true});

	boosts.push_back({-3584.0, 2484.0, true});
	boosts.push_back({ 3584.0, 2484.0, true});
	boosts.push_back({    0.0, 2816.0, true});
	boosts.push_back({ -940.0, 3310.0, true});
	boosts.push_back({  940.0, 3308.0, true});
	boosts.push_back({-1792.0, 4184.0, true});
	boosts.push_back({ 1792.0, 4184.0, true});
	boosts.push_back({    0.0, 4240.0, true});
}

void BakkesFileLogger::refreshBoosts() {
	for (boost& boost : boosts) {
		boost.isActive = true;
	}
}

ServerWrapper BakkesFileLogger::getServerWrapper() {
	if (gameWrapper->IsInReplay()) return gameWrapper->GetGameEventAsReplay().memory_address;
	else if (gameWrapper->IsInOnlineGame()) return gameWrapper->GetOnlineGame();
	else if (gameWrapper->IsInFreeplay()) return gameWrapper->GetGameEventAsServer();
	else if (gameWrapper->IsInCustomTraining()) return gameWrapper->GetGameEventAsServer();
	else if (gameWrapper->IsSpectatingInOnlineGame()) return gameWrapper->GetOnlineGame();
	else if (gameWrapper->IsInGame()) return gameWrapper->GetGameEventAsServer();
	else return NULL;
}
