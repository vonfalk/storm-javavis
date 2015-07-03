#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <Windows.h>

typedef unsigned int nat;

using namespace std;

vector<string> findFiles(const string &str) {
	vector<string> r;
	WIN32_FIND_DATA data;
	HANDLE h = FindFirstFile(str.c_str(), &data);
	if (h == INVALID_HANDLE_VALUE)
		return r;

	do {
		r.push_back(data.cFileName);
	} while (FindNextFile(h, &data));

	return r;
}

int main(int argc, const char **argv) {
	ifstream config("buildconfig");
	string build;
	getline(config, build);
	string project;
	getline(config, project);

	string params;
	size_t split = project.find('#');
	if (split != string::npos) {
		params = " " + project.substr(split + 1);
		project = project.substr(0, split);
	}

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

	if (r == 0 && all) {
		vector<string> paths = findFiles(build + "\\*Test.exe");
		for (nat i = 0; i < paths.size(); i++) {
			string cmd = build + "\\" + paths[i];
			cout << endl << cmd << endl;
			system(cmd.c_str());
		}
	} else if (r == 0 && !clean && !release) {
		cout << endl;

		string cmd = build + "\\" + project + params;
		if (valgrind) {
			cmd = "drmemory.exe -logdir Debug -batch -results_to_stderr -- " + cmd;
		}

		cout << cmd << endl;
		system(cmd.c_str());
	}

	return 0;
}
