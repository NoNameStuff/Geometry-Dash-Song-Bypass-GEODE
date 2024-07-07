// Settings
const char* popupTitle = "Song Bypass";

// Other librarys to include
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

#include <ShlObj_core.h>

// Include the Geode headers
#include <Geode/Geode.hpp>

#include <Geode/modify/LevelInfoLayer.hpp>

// Brings cocos2d and all Geode namespaces to the current scope
using namespace geode::prelude;

void downloadSong(std::string songId, bool notify) {
	// Annoying
	PWSTR localAppData = NULL;
	std::string songLocation;
	if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &localAppData) == S_OK) {
		char dest[MAX_PATH];
		wcstombs(dest, localAppData, MAX_PATH);
		songLocation = dest;
		songLocation += "\\GeometryDash\\"; // Geometry dash
	}
	else {
		if (notify)
			FLAlertLayer::create(popupTitle, "Failed to get your music folder", "OK")->show(); // How
		return;
	}

	// TODO: Rework songs internally

	// Contruct url
	std::string urlStr = "https://nns4.me/gd-song-bypass/songs/" + songId + ".mp3";
	const char* url = urlStr.c_str();

	// Destination + file name
	std::string destinationStr = songLocation + songId + ".mp3";
	const char* destination = destinationStr.c_str();

	// Download file to destination
	HRESULT downloadResult = URLDownloadToFile(NULL, url, destination, 0, NULL);
	if (downloadResult == S_OK) {
		if (notify)
			FLAlertLayer::create(popupTitle, "Successfully downloaded song " + songId, "OK")->show();
		return;
	}
	else if (downloadResult == E_OUTOFMEMORY) {
		if (notify)
			FLAlertLayer::create(popupTitle, "Out of memory", "OK")->show();
		return;
	}
	else if (downloadResult == INET_E_DOWNLOAD_FAILURE) {
		if (notify)
			FLAlertLayer::create(popupTitle, "Download failure", "OK")->show();
		return;
	}

	// An error occured! (im pretty sure this triggers only if the song wasn't found)
	if (notify)
		FLAlertLayer::create(popupTitle, "Song (" + songId + ")" + " not found", "OK")->show();
}

void pageDownload(std::string songId, std::map<int, bool> songsMap, bool notify) {
	bool foundSongsIdLoop = false;
	bool attemptedDownload = false;

	for (auto const& [id, isDownloaded] : songsMap)
	{
		if (!isDownloaded) {
			downloadSong(std::to_string(id), notify);
			attemptedDownload = true;
		}
		foundSongsIdLoop = true;
	}

	if (!foundSongsIdLoop) {
		downloadSong(songId, notify);
		attemptedDownload = true;
	}

	if (!attemptedDownload && notify)
		FLAlertLayer::create(popupTitle, "All songs are already downloaded", "OK")->show();
}

#include <string>

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
	static inline std::string songId = "";
	static inline std::map<int, bool> songsMap{};

	bool init(GJGameLevel * level, bool challenge) {
		if (!LevelInfoLayer::init(level, challenge)) {
			return false;
		}

		bool isEnabled = Mod::get()->getSettingValue<bool>("is-enabled");
		if (!isEnabled)
			return true;

		// Get the song id(s)
		songId = std::to_string(LevelInfoLayer::m_songWidget->m_customSongID);
		songsMap = LevelInfoLayer::m_songWidget->m_songs;

		for (auto const& [id, isDownloaded] : songsMap)
		{
			log::debug("Songs Map ->");
			log::debug("ID: {0}", id);
			log::debug("Is Downloaded: {0}", isDownloaded);
		}

		// Create download button
		auto downloadSongBtn = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png"),
			this,
			menu_selector(MyLevelInfoLayer::downloadBtnCallback)
		);

		auto leftSideMenu = this->getChildByID("left-side-menu");
		leftSideMenu->addChild(downloadSongBtn);

		downloadSongBtn->setID("gdsb-download-btn"_spr);

		leftSideMenu->updateLayout();

		// Auto download
		bool autoDownload = Mod::get()->getSettingValue<bool>("auto-download");
		if (autoDownload == true)
			pageDownload(songId, songsMap, false);

		// Debug
		log::debug("Finished loading level page and ran autodownload check");

		return true;
	}

	// Callback for download button on level page
	void downloadBtnCallback(CCObject*) {
		std::string popupDescription = "Download song " + songId;

		int amountOfOtherSongs = 0;
		for (auto const& [id, isDownloaded] : songsMap)
		{
			if (!isDownloaded)
				amountOfOtherSongs += 1;
		}

		if (amountOfOtherSongs > 1)
			popupDescription += " and " + std::to_string(amountOfOtherSongs - 1) + " more";

		popupDescription += "?";

		// TODO: make setting to disable popup
		geode::createQuickPopup(
			popupTitle,
			popupDescription,
			"Cancel", "Download",
			[](auto, bool btn2) {
				if (btn2) {
					pageDownload(songId, songsMap, true);
				}
			}
		);
	}
};