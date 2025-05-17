#include <algorithm>
#include "../include/CGIHandler.hpp"
#include "../include/Utils.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <vector>
#include <cstring>
#include <sstream>

CGIHandler::CGIHandler(const Request& req, const LocationConfig& loc)
    : success_(false) {
    // Build the absolute script path
    std::string locationRoot = loc.getRoot();
    std::string reqPath = req.getPath();
    if (!locationRoot.empty() && locationRoot[locationRoot.size()-1] == '/')
        locationRoot = locationRoot.substr(0, locationRoot.size()-1);
    if (!reqPath.empty() && reqPath[0] == '/')
        reqPath = reqPath.substr(1);
    scriptPath_ = locationRoot + "/" + reqPath;
    interpreterPath_ = loc.getCgiPath();
    requestBody_ = req.getBody();

    // Prepare standard CGI environment variables
    env_["REQUEST_METHOD"]  = req.getMethod();   // HTTP method, e.g., "GET" or "POST"
    env_["SCRIPT_NAME"]     = req.getPath();     // The requested script path
    env_["SERVER_PROTOCOL"] = req.getProtocol(); // HTTP version, e.g., "HTTP/1.1"
    env_["CONTENT_LENGTH"]  = req.getBody().empty() ? "0" : intToStr(req.getBody().size());

    // Add all HTTP headers as CGI environment variables
    // The CGI standard requires headers to be uppercased, dashes replaced with underscores, and prefixed with HTTP_
    // For example: "User-Agent" becomes "HTTP_USER_AGENT"
    // std::map<std::string, std::string> headers = req.getHeaders();
    // for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it) {
    //     std::string envKey = it->first;
    //     std::transform(envKey.begin(), envKey.end(), envKey.begin(), ::toupper); // Uppercase
    //     std::replace(envKey.begin(), envKey.end(), '-', '_');                    // Replace '-' with '_'
    //     env_["HTTP_" + envKey] = it->second;                                    // Prefix and set
    // }

    // NOT SURE IF THIS IS NEEDED - murat
}

std::string CGIHandler::run() {
    // Create pipes for stdin, stdout, and stderr
    int inPipe[2], outPipe[2], errPipe[2];
    if (pipe(inPipe) == -1 || pipe(outPipe) == -1 || pipe(errPipe) == -1) {
        errorMsg_ = "Pipe creation failed";
        return "";
    }

    pid_t pid = fork();
    if (pid < 0) {
        errorMsg_ = "Fork failed";
        return "";
    }

    if (pid == 0) {
        // --- Child process ---
        // Redirect stdin, stdout, stderr
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        dup2(errPipe[1], STDERR_FILENO);
        close(inPipe[1]);
        close(outPipe[0]);
        close(errPipe[0]);

        // Prepare environment variables for execve
        std::vector<char*> envp;
        for (std::map<std::string, std::string>::const_iterator it = env_.begin(); it != env_.end(); ++it) {
            std::string entry = it->first + "=" + it->second;
            envp.push_back(strdup(entry.c_str()));
        }
        envp.push_back(NULL);

        // Prepare arguments for execve
        char* argv[3];
        argv[0] = const_cast<char*>(interpreterPath_.c_str());
        argv[1] = const_cast<char*>(scriptPath_.c_str());
        argv[2] = NULL;

        // Execute the CGI script
        execve(interpreterPath_.c_str(), argv, &envp[0]);
        // If execve fails, exit child
        _exit(1);
    } else {
        // --- Parent process ---
        close(inPipe[0]);
        close(outPipe[1]);
        close(errPipe[1]);

        // Write request body to child (if any)
        if (!requestBody_.empty()) {
            write(inPipe[1], requestBody_.c_str(), requestBody_.size());
        }
        close(inPipe[1]);

        // Read stdout from child (CGI output)
        std::stringstream output;
        char buf[4096];
        ssize_t n;
        while ((n = read(outPipe[0], buf, sizeof(buf))) > 0) {
            output.write(buf, n);
        }
        close(outPipe[0]);

        // Read stderr from child (errors)
        std::stringstream errOutput;
        while ((n = read(errPipe[0], buf, sizeof(buf))) > 0) {
            errOutput.write(buf, n);
        }
        close(errPipe[0]);

        // Wait for child process to finish
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            success_ = true;
            return output.str();
        } else {
            errorMsg_ = "CGI script failed: " + errOutput.str();
            return "";
        }
    }
}

bool CGIHandler::wasSuccessful() const {
    return success_;
}

std::string CGIHandler::getError() const {
    return errorMsg_;
}
