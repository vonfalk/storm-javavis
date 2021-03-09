#include "stdafx.h"
#include "Open.h"
#include "Error.h"
#include "Core/Str.h"
#include "Core/Convert.h"

#if defined(WINDOWS)

#include <shellapi.h>
#pragma comment (lib, "Shell32.lib")

namespace storm {

	void open(Url *file) {
		DWORD error = (DWORD)ShellExecute(NULL, L"open", file->toS()->c_str(), NULL, NULL, SW_SHOWDEFAULT);
		const wchar *msg = null;
		switch (error) {
		case 0:
		case SE_ERR_OOM:
			msg = S("Out of memory.");
			break;
		case ERROR_FILE_NOT_FOUND:
			msg = S("File not found.");
			break;
		case ERROR_PATH_NOT_FOUND:
			msg = S("Path not found.");
			break;
		case SE_ERR_ACCESSDENIED:
			msg = S("Access denied.");
			break;
		case SE_ERR_ASSOCINCOMPLETE:
		case SE_ERR_NOASSOC:
			msg = S("Unknown or incomplete file association.");
			break;
		default:
			if (error <= 32)
				msg = S("Unknown error.");
		}

		if (msg)
			throw new (file) SystemError(TO_S(file, S("Failed to open ") << file << S(": ") << msg));
	}

}

#elif defined(POSIX)

#include <unistd.h>
#include <sys/wait.h>
#include <sys/fcntl.h>

namespace storm {

	// Spawn a new process. Properly indicates failures from "execlp" to the parent process.
	// 'params' is an array of parameters as one would give the execvp() function.
	// Makes sure we don't have to wait for the child process.
	// If "monitorExit" is set, then we wait until the process was terminated and return the exit code.
	static int execute(Engine &e, const char *params[], bool monitorExit = false) {
		// To detect errors in the child, we create a new pipe with the CLOEXEC flag set. As such,
		// we can wait until the write end is closed. If nothing was written, then we know that
		// everything went well (at least up to starting the process). Otherwise we write an error
		// message to the pipe before exiting.
		int pipefds[2];
		if (pipe(pipefds) != 0)
			throw new (e) SystemError(S("Failed to create a pipe for spawning a process."));

		int pipeRead = pipefds[0];
		int pipeWrite = pipefds[1];
		fcntl(pipeWrite, F_SETFD, FD_CLOEXEC);

		pid_t pid = fork();
		if (pid < 0) {
			close(pipeRead);
			close(pipeWrite);
			throw new (e) SystemError(S("Failed to spawn a child process."));
		}

		if (pid == 0) {
			// Child.
			// Close the read end of the pipe.
			close(pipeRead);

			// Fork once more to avoid zombies.
			pid = fork();
			if (pid < 0) {
				const char *msg = "Failed to spawn a child process.";
				write(pipeWrite, msg, strlen(msg));
				exit(1);
			}

			if (pid == 0) {
				// Child. Now we can call exec!
				execvp(params[0], (char **)params);

				// If we get here, the call failed.
				const char *prefix = "Failed to execute the child: ";
				const char *error = strerror(errno);
				write(pipeWrite, prefix, strlen(prefix));
				write(pipeWrite, error, strlen(error));
				exit(1);
			}

			// Parent. See if we should wait for the exit code from the child.
			if (monitorExit) {
				int status = 0;
				waitpid(pid, &status, 0);

				// Forward it.
				if (WIFEXITED(status))
					exit(WEXITSTATUS(status));
				if (WIFSIGNALED(status))
					raise(WTERMSIG(status));
			}

			exit(100);
		}

		// Close the write end from here.
		close(pipeWrite);

		// Wait for the first child.
		int status = 0;
		waitpid(pid, &status, 0);

		// Read from the pipe until we are done.
		char buffer[128];
		StrBuf *message = new (e) StrBuf();
		while (true) {
			ssize_t result = read(pipeRead, buffer, sizeof(buffer));
			if (result < 0) {
				if (errno == EINTR)
					continue;

				perror("Error reading the pipe");
				return 100;
			} else if (result == 0) {
				// No more data. The socket is closed!
				break;
			} else {
				// Save the message.
				*message << toWChar(e, buffer, size_t(result))->v;
			}
		}

		if (message->any())
			throw new (e) SystemError(message->toS());

		if (WIFEXITED(status))
			return WEXITSTATUS(status);
		if (WIFSIGNALED(status))
			return -WTERMSIG(status);
		return 0;
	}

	void open(Url *file) {
		// Note: We are not using "format" here since we want to support HTTP and HTTPS for example.
		const char *params[] = {
			"xdg-open",
			file->toS()->utf8_str(),
			NULL
		};
		int status = execute(file->engine(), params, true);
		if (status == 2)
			throw new (file) SystemError(TO_S(file, S("The file ") << file << S(" does not exist.")));
		if (status == 3)
			throw new (file) SystemError(TO_S(file, S("Unable to find a tool for opening ") << file));
		if (status != 0)
			throw new (file) SystemError(TO_S(file, S("Failed to open: ") << file));
	}

}

#endif

