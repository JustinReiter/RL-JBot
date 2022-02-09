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

	std::ofstream of;
	std::string playerName;

	// Called on init and dtor of plugin
	virtual void onLoad();
	virtual void onUnload();

	// Writes the current controller state to of
	void runGameTickLog(PlayerControllerWrapper);

	// Closes the file output stream (generally at end of game)
	void closeFileOutputStream();

	// Creates new file to stream output to (created at the beginning of a game)
	void initFileOutputStream();
};

