#include "../include/Server.hpp"
#include "../include/Socket.hpp"
#include "../include/Request.hpp"
#include "../include/Webserver.hpp"
#include "../include/CGIHandler.hpp"
#include "../include/Logger.hpp"
#include "../include/Utils.hpp"
#include <dirent.h>
#include <algorithm>

void Server::list_directory(const std::string &path, Response& res)
{
	DIR* dir = opendir(path.c_str());
	if (!dir) {
		logError("Failed to open directory: " + path + " - " + std::string(strerror(errno)));
		res.setStatus(500);
		std::string body = "<html><body><h1>500 Internal Server Error</h1><p>Cannot read directory.</p></body></html>";
		res.setHeader("Content-Type", "text/html");
		res.setHeader("Content-Length", intToStr(body.size()));
		res.setBody(body);
		return;
	}

	// Extract the URL path from the filesystem path
	std::string urlPath = path;
	if (urlPath.length() >= 3 && urlPath.substr(0, 3) == "www") {
		urlPath = urlPath.substr(3);
	}
	if (urlPath.empty()) {
		urlPath = "/";
	}

	// Start building HTML response
	std::ostringstream html;
	html << "<html><head><title>Index of " << urlPath << "</title></head><body>\n";
	html << "<h1>Index of " << urlPath << "</h1>\n";
	html << "<ul>\n";

	// Add parent directory link if not at root
	if (urlPath != "/") {
		html << "<li><a href=\"../\">[Parent Directory]</a></li>\n";
	}

	// Read directory entries
	std::vector<std::string> entries;
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;

		// Skip current and parent directory entries
		if (name == "." || name == "..") {
			continue;
		}
		entries.push_back(name);
	}
	closedir(dir);

	// Simple alphabetical sort
	std::sort(entries.begin(), entries.end());

	// Generate list items for each entry
	for (std::vector<std::string>::const_iterator it = entries.begin();
		 it != entries.end(); ++it) {
		const std::string& name = *it;
		std::string fullEntryPath = path + "/" + name;

		bool isDir = isDirectory(fullEntryPath);
		std::string displayName = name;
		if (isDir) {
			displayName += "/";
		}

		html << "<li><a href=\"" << name;
		if (isDir) html << "/";
		html << "\">" << displayName << "</a></li>\n";
	}

	html << "</ul>\n";
	html << "</body></html>\n";

	std::string responseBody = html.str();
	res.setStatus(200);
	res.setHeader("Content-Type", "text/html");
	res.setHeader("Content-Length", intToStr(responseBody.size()));
	res.setBody(responseBody);

	logInfo("Directory listing generated for: " + path);
}
