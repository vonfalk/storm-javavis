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

	string compile;
	if (argc > 1 && strcmp(argv[1], "-c") == 0)
		clean = true;

	if (clean)
		compile = "devenv storm.sln /Clean";
	else
		compile = "devenv storm.sln /Build " + build + " /Project " + project;

	int r = system(compile.c_str());

	if (r == 0 && !clean) {
		cout << "Running!" << endl;

		string cmd = build + "\\" + project;
		system(cmd.c_str());
	}

	return 0;
}
