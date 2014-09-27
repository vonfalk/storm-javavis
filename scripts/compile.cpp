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

	string compile;
	if (argc > 1) {
		if (strcmp(argv[1], "-c") == 0)
			clean = true;
		else if (strcmp(argv[1], "-r") == 0)
			release = true;
		else if (strcmp(argv[1], "-a") == 0)
			all = true;
	}

	if (clean)
		compile = "devenv storm.sln /Clean";
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
		system(cmd.c_str());
	}

	return 0;
}
