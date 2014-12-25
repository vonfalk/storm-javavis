#include <iostream>
#include <string>
#include <fstream>

using namespace std;

int main(int argc, const char **argv) {
	ifstream config("buildconfig");
	string build;
	getline(config, build);
	string project;
	getline(config, project);

	bool clean = false;
	bool all = false;
	bool release = false;
	bool valgrind = false;

	string compile;
	for (int i = 1; i < argc; i++) {
		const char *s = argv[i];
		if (strcmp(s, "-c") == 0)
			clean = true;
		else if (strcmp(s, "-r") == 0)
			release = true;
		else if (strcmp(s, "-a") == 0)
			all = true;
		else if (strcmp(s, "-v") == 0)
			valgrind = true;
	}

	if (clean)
		compile = "devenv storm.sln /Clean " + build;
	else
		compile = "devenv storm.sln /Build " + build + " /Project " + project;

	if (all)
		compile = "devenv storm.sln /Build " + build;

	if (release)
		compile = "devenv storm.sln /Build Release";

	int r = system(compile.c_str());

	if (r == 0 && !clean && !all && !release) {
		cout << endl;

		string cmd = build + "\\" + project;
		if (valgrind) {
			cmd = "drmemory.exe -logdir Debug -batch -results_to_stderr -- " + cmd;
		}

		system(cmd.c_str());
	}

	return 0;
}
