#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include "version.h"
#include <ctime>
#include <iomanip>
#include <fstream>
#include <sstream>

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class BakkesFileLogger: public BakkesMod::Plugin::BakkesModPlugin/*, public BakkesMod::Plugin::PluginSettingsWindow*//*, public BakkesMod::Plugin::PluginWindow*/
{
	/// <summary>
	/// Struct for storing boost information. One set of boosts are maintained throughout the plugin execution.
	/// </summary>
	struct boost {
		boost(float X, float Y, bool isActive) : X(X), Y(Y), isActive(isActive) {};
		float X;
		float Y;
		bool isActive;
	};

	const std::string PATHDIR = "C:/Users/Justi/Desktop/projects/JBot/BakkesFileLogger/logs/";

	std::ofstream outputOFStream;
	std::ofstream inputOFStream;
	std::ostringstream outputFileName;
	std::ostringstream inputFileName;
	std::string playerName;
	int teamFactor = 0;
	bool isGameActive = false;
	std::vector<boost> boosts;

	/// <summary>
	/// Setup the hooks at the beginning of the plugin execution
	/// </summary>
	virtual void onLoad();

	/// <summary>
	/// Close the file output stream at the end of the plugin execution
	/// </summary>
	virtual void onUnload();

	// Writes the current controller state to of
	void runGameTickLog(PlayerControllerWrapper);

	/// <summary>
	/// Close the file output stream
	/// </summary>
	void closeGameLogging();

	/// <summary>
	/// Initializes the logger variables at the start of every game. The file output stream is initialized to the current datetime and boosts initialized on.
	/// </summary>
	void initGameLogging();

	/// <summary>
	/// Initializes the flags used between goals and sets the team adjustment factor
	/// </summary>
	void initKickoffStart();

	/// <summary>
	/// Initializes the flags used on goal score (prevents extra logging)
	/// </summary>
	void initGoalSequence();

	/// <summary>
	/// Initializes the boost vector at the start of each game.
	/// The boosts are arranged in ascending order in y, then x
	/// </summary>
	void initBoosts();

	/// <summary>
	/// Reset the boosts to be all active
	/// </summary>
	void refreshBoosts();

	/// <summary>
	/// Checks the type of game being played and returns an appropriate server object.
	/// </summary>
	/// <returns>ServerWrapper || null</returns>
	ServerWrapper getServerWrapper();
};
