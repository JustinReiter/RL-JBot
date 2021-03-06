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



	/// PRE/POST GAME HOOKS

	// Game end
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed",
		[this](std::string eventName) {
			teamFactor = 0;
			isGameActive = false;
			closeGameLogging();
	});
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
		[this](std::string eventName) {
			teamFactor = 0;
			isGameActive = false;
			closeGameLogging();
	});
	// Game start
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.WaitingForPlayers.BeginState",
		[this](std::string eventName) {
			initGameLogging();
			isGameActive = true;
	});
	// Kickoff start
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.StartRound",
		[this](std::string eventName) {
			initKickoffStart();
	});
	// Goal scored
	gameWrapper->HookEvent("Function TAGame.Ball_TA.OnHitGoal",
		[this](std::string eventName) {
			if (!isGameActive) return;
			initGoalSequence();
	});
	// Demo
	gameWrapper->HookEventWithCaller<CarWrapper>("Function TAGame.Car_TA.Demolish",
		[this](CarWrapper caller, void* params, std::string eventname) {
			cvarManager->log("The following player was demo'd: " + caller.GetPRI().GetPlayerName().ToString());
			if (caller.GetPRI().GetPlayerName().ToString() == playerName && isGameActive) {
				cvarManager->log("Disabling logging while player is dead");
				teamFactor = 0;
			}
			else if (isGameActive) {
				cvarManager->log("Opponent demo'ed... Continue logging with far values for opponent");
			}
	});
	// Car Spawn
	gameWrapper->HookEvent("Function TAGame.GameEvent_TA.AddCar",
		[this](std::string eventName) {
			if (teamFactor == 0 && isGameActive) {
				cvarManager->log("restarting game logging due to player respawn");
				initKickoffStart();
			}
	});

	
	initBoosts();
}

void BakkesFileLogger::onUnload() {
	// Close the output stream
	if (outputOFStream.is_open()) outputOFStream.close();
	if (inputOFStream.is_open()) inputOFStream.close();
}

void BakkesFileLogger::closeGameLogging() {
	if (outputOFStream.is_open()) {
		cvarManager->log("Closing the output stream for the following file: " + outputFileName.str());
		outputOFStream.close();
	}
	if (inputOFStream.is_open()) {
		cvarManager->log("Closing the input stream for the following file: " + inputFileName.str());
		inputOFStream.close();
	}
}

void BakkesFileLogger::initGameLogging() {
	// Close previous file if still opened
	if (outputOFStream.is_open() || inputOFStream.is_open()) closeGameLogging();

	// Declare the file writers to output
	std::time_t t = std::time(nullptr);
	tm* time = std::localtime(&t);

	// Open file with datetime name (prevent overwriting files)
	outputFileName.str("");
	inputFileName.str("");

	outputFileName.clear();
	inputFileName.clear();
	
	outputFileName << PATHDIR << std::put_time(time, "%Y-%m-%d_%H-%M-%S") << "_output.csv";
	inputFileName << PATHDIR << std::put_time(time, "%Y-%m-%d_%H-%M-%S") << "_input.csv";
	
	cvarManager->log("Opened new output file at: " + outputFileName.str());
	cvarManager->log("Opened new input file at: " + inputFileName.str());
	
	outputOFStream.open(outputFileName.str());
	inputOFStream.open(inputFileName.str());

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
	if (!gameWrapper->IsInGame() || !isGameActive || !caller || !outputOFStream.is_open() || !inputOFStream.is_open() || teamFactor == 0) return;

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

	BoostWrapper boostWrapper = playerCar.GetBoostComponent();
	if (!boostWrapper) return;
	bool isSuperSonic = playerCar.GetbSuperSonic();
	bool hasJumped = playerCar.GetbJumped();

	std::ostringstream tempInput;
	std::ostringstream tempOutput;

	// 11 player input parameters
	tempInput << teamFactor * location.X << "," << teamFactor * location.Y << "," << location.Z << ",";
	tempInput << teamFactor * velocity.X << "," << teamFactor * velocity.Y << "," << velocity.Z << ",";
	tempInput << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";
	tempInput << boostWrapper.GetCurrentBoostAmount() << "," << isSuperSonic << "," << hasJumped << ",";


	// Opponent input parameters
	bool hasAddedOpponent = false;
	for (CarWrapper car : server.GetCars()) {
		// If car is owned by player, skip
		// TODO implement the name or id matching
		if (car.GetPRI().GetPlayerName().ToString() == playerName) continue;

		Vector location = car.GetLocation();
		Vector velocity = car.GetVelocity();
		Vector angularVelocity = car.GetAngularVelocity();

		boostWrapper = playerCar.GetBoostComponent();
		if (!boostWrapper) return;
		bool isSuperSonic = car.GetbSuperSonic();
		bool hasJumped = car.GetbJumped();

		hasAddedOpponent = true;

		// 12 opponent input parameters
		tempInput << teamFactor * location.X << "," << teamFactor * location.Y << "," << location.Z << ",";
		tempInput << teamFactor * velocity.X << "," << teamFactor * velocity.Y << "," << velocity.Z << ",";
		tempInput << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z << ",";
		tempInput << boostWrapper.GetCurrentBoostAmount() << "," << isSuperSonic << "," << hasJumped << ",";
	}
	if (!hasAddedOpponent) {
		// 12 opponent input parameters
		tempInput << 0 << "," << 9999 << "," << 0 << ",";
		tempInput << 0 << "," << 0 << "," << 0 << ",";
		tempInput << 0 << "," << 0 << "," << 0 << ",";
		tempInput << 0 << "," << 0 << "," << 0 << ",";
	}


	// Ball input parameters
	BallWrapper ball = server.GetBall();
	if (!ball) return;
	location = ball.GetLocation();
	velocity = ball.GetVelocity();
	angularVelocity = ball.GetAngularVelocity();


	// 9 ball parameters
	tempInput << teamFactor * location.X << "," << teamFactor * location.Y << "," << location.Z << ",";
	tempInput << teamFactor * velocity.X << "," << teamFactor * velocity.Y << "," << velocity.Z << ",";
	tempInput << angularVelocity.X << "," << angularVelocity.Y << "," << angularVelocity.Z;


	// Boost parameters (each boost has the following data: {X, Y, isActive} )
	// For now there are: 34 parameters
	for (boost& boost : boosts) {
		tempInput << "," << boost.isActive;
	}
	tempInput << std::endl;

	inputOFStream << tempInput.str();

	// Player output parameters
	ControllerInput playerInput = caller.GetVehicleInput();
	tempOutput << playerInput.Steer << "," << playerInput.Throttle << "," << (playerInput.ActivateBoost || playerInput.HoldingBoost) << "," << playerInput.Jump << ",";
	tempOutput << playerInput.Pitch << "," << playerInput.Yaw << "," << playerInput.Roll << "," << playerInput.Handbrake << std::endl;

	outputOFStream << tempOutput.str();
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
		isGameActive = true;
	}
}

void BakkesFileLogger::initGoalSequence() {
	teamFactor = 0;
	isGameActive = false;
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
