#include "../include/CGI.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <cstring>
#include <fcntl.h>

CGI::CGI(const std::string &scriptPath, const std::string &interpreter)
	: scriptPath(scriptPath), interpreter(interpreter) {}

void CGI::setEnvVar(const std::string &key, const std::string &value)
{
	env[key] = value;
}

void CGI::setRequestBody(const std::string &body)
{
	requestBody = body;
}

void CGI::setArgs(const std::vector<std::string> &arguments)
{
	args = arguments;
}

const std::string& CGI::getScriptPath() const { return scriptPath; }
void CGI::setScriptPath(const std::string &path) { scriptPath = path; }

const std::string& CGI::getInterpreter() const { return interpreter; }
void CGI::setInterpreter(const std::string &interp) { interpreter = interp; }

const std::map<std::string, std::string>& CGI::getEnv() const { return env; }
void CGI::setEnv(const std::map<std::string, std::string> &environment) { env = environment; }

const std::string& CGI::getRequestBody() const { return requestBody; }
const std::vector<std::string>& CGI::getArgs() const { return args; }

void CGI::setupFromRequest(const Request& req) {
	setEnvVar("REQUEST_METHOD", req.method);
	setEnvVar("PATH_INFO", req.path);
/* 	// Extract QUERY_STRING from path if present
	size_t qpos = req.path.find('?');
	if (qpos != std::string::npos)
		setEnvVar("QUERY_STRING", req.path.substr(qpos + 1));
	else
		setEnvVar("QUERY_STRING", "");
	// Set to or leave empty */
	setEnvVar("CONTENT_TYPE", req.headers.count("Content-Type") ? req.headers.at("Content-Type") : "");
	setEnvVar("CONTENT_LENGTH", req.headers.count("Content-Length") ? req.headers.at("Content-Length") : "");
	setRequestBody(req.body);
}

std::string CGI::execute()
{
	// Prepare environment
	std::vector<char*> envp;
	for (std::map<std::string, std::string>::iterator it = env.begin(); it != env.end(); ++it) {
		std::string entry = it->first + "=" + it->second;
		envp.push_back(strdup(entry.c_str()));
	}
	envp.push_back(NULL);

	// Prepare arguments
	std::vector<char*> execArgs;
	execArgs.push_back(const_cast<char*>(interpreter.c_str()));
	execArgs.push_back(const_cast<char*>(scriptPath.c_str()));
	for (size_t i = 0; i < args.size(); ++i)
	execArgs.push_back(const_cast<char*>(args[i].c_str()));
	execArgs.push_back(NULL);

	/* // print envp
	for (size_t i = 0; envp[i] != NULL; ++i)
		std::cout << envp[i] << std::endl;
	// print interpreter
	std::cout << interpreter << std::endl;
	// check access to script
	if (access(scriptPath.c_str(), X_OK) == -1)
		throw std::runtime_error("Script not executable");
	// check if interpreter is executable
	if (access(interpreter.c_str(), X_OK) == -1)
		throw std::runtime_error("Interpreter not executable");
	 */
	int inPipe[2], outPipe[2];
	if (pipe(inPipe) == -1 || pipe(outPipe) == -1)
		throw std::runtime_error("Pipe creation failed");

	pid_t pid = fork();
	if (pid < 0)
		throw std::runtime_error("Fork failed");

	if (pid == 0) 
	{
		// Child process
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		close(inPipe[1]);
		close(outPipe[0]);
		// Execute
		execve(interpreter.c_str(), &execArgs[0], &envp[0]);
		// If execve fails
		perror("execve");
		_exit(1);
	}

	// Parent process
	close(inPipe[0]);
	close(outPipe[1]);

	// Write request body to child
	if (!requestBody.empty())
		write(inPipe[1], requestBody.c_str(), requestBody.size());
	close(inPipe[1]);

	// Read output from child
	std::ostringstream output;
	char buffer[1024];
	ssize_t bytesRead;
	while ((bytesRead = read(outPipe[0], buffer, sizeof(buffer))) > 0)
		output.write(buffer, bytesRead);
	close(outPipe[0]);

	int status;
	waitpid(pid, &status, 0);

	return output.str();
}
