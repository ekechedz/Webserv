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
    // Build script path from location root and request path
    std::string locationRoot = loc.getRoot();
    std::string reqPath = req.getPath();
    if (!locationRoot.empty() && locationRoot[locationRoot.size()-1] == '/')
        locationRoot = locationRoot.substr(0, locationRoot.size()-1);
    if (!reqPath.empty() && reqPath[0] == '/')
        reqPath = reqPath.substr(1);
    scriptPath_ = locationRoot + "/" + reqPath;
    interpreterPath_ = loc.getCgiPath();
    requestBody_ = req.getBody();
    // Prepare environment variables
    env_["REQUEST_METHOD"] = req.getMethod();
    env_["SCRIPT_NAME"] = req.getPath();
    env_["SERVER_PROTOCOL"] = req.getProtocol();
    env_["CONTENT_LENGTH"] = req.getBody().empty() ? "0" : intToStr(req.getBody().size());
    std::map<std::string, std::string> headers = req.getHeaders();
    for (std::map<std::string, std::string>::iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string key = it->first;
        std::transform(key.begin(), key.end(), key.begin(), ::toupper);
        std::replace(key.begin(), key.end(), '-', '_');
        env_["HTTP_" + key] = it->second;
    }
}

std::string CGIHandler::run() {
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
        // Child
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        dup2(errPipe[1], STDERR_FILENO);
        close(inPipe[1]); close(outPipe[0]); close(errPipe[0]);
        // Prepare envp
        std::vector<char*> envp;
        for (std::map<std::string, std::string>::const_iterator it = env_.begin(); it != env_.end(); ++it) {
            std::string entry = it->first + "=" + it->second;
            envp.push_back(strdup(entry.c_str()));
        }
        envp.push_back(NULL);
        // Prepare argv
        char* argv[3];
        argv[0] = const_cast<char*>(interpreterPath_.c_str());
        argv[1] = const_cast<char*>(scriptPath_.c_str());
        argv[2] = NULL;
        execve(interpreterPath_.c_str(), argv, &envp[0]);
        // If execve fails
        _exit(1);
    } else {
        // Parent
        close(inPipe[0]);
        close(outPipe[1]);
        close(errPipe[1]);

        // Write request body to child
        if (!requestBody_.empty()) {
            write(inPipe[1], requestBody_.c_str(), requestBody_.size());
        }
        close(inPipe[1]);

        // Read output from child (stdout)
        std::stringstream output;
        char buf[4096];
        ssize_t n;
        while ((n = read(outPipe[0], buf, sizeof(buf))) > 0) {
            output.write(buf, n);
        }
        close(outPipe[0]);

        // Read error output from child (stderr)
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
