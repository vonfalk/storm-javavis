#include "stdafx.h"
#include "Open.h"
#include "Error.h"


#if defined(WINDOWS)

#include <shellapi.h>
#pragma comment (lib, "Shell32.lib")

namespace storm {

	void open(Url *file) {
		if (!ShellExecute(NULL, L"open", file->toS()->c_str(), NULL, NULL, SW_SHOWDEFAULT))
			throw new (file) SystemError(TO_S(file, "Failed to open " << file));
	}

}

#elif defined(POSIX)

#include <unistd.h>

namespace storm {

	void open(Url *file) {
		// Note: We are not using "format" here since we want to support HTTP and HTTPS for example.
		const char *name = file->toS()->utf8_str();
		pid_t pid = fork();
		if (pid < 0)
			throw new (file) SystemError(S("Failed to spawn a new process."));

		if (pid == 0) {
			// Fork once more to avoid zombie processes.
			if (fork() == 0) {
				// Child. Call exec.
				execlp("xdg-open", "xdg-open", name, NULL);
				exit(1);
			}
			exit();
		}

		// Wait for the first child.
		int status = 0;
		waitpid(pid, &status, 0);

		// Note: We can not detect if "xdg-open" actually managed to run or not.
	}

}

#endif

