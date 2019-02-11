/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include <dirent.h>
#include <iostream>
#include "utils/FileManagement.hpp"

using namespace std;

vector<string> getAllFilesInDirs(const std::string& prefix, const vector<string>& testdirs) {
	vector<string> files;
	DIR *dir;
	struct dirent *ent;
	for (auto currTestDir = testdirs.cbegin(); currTestDir != testdirs.cend(); ++currTestDir) {
		dir = opendir((prefix + (*currTestDir)).c_str());
		if (dir != NULL) {
			while ((ent = readdir(dir)) != NULL) {
				if (ent->d_name[0] != '.') {
					files.push_back(prefix + (*currTestDir) + ent->d_name);
				}
			}
			closedir(dir);
		} else {
			clog << "FAIL    |  Could not open directory " << *currTestDir << "\n.";
		}
	}
	return files;
}

std::string getFirstLineInFile(const std::string& filename) {
	std::ifstream f(filename, std::ios::in);
	std::string line;
	std::getline(f, line);
	return line;
}

void removeFile(const std::string& filename) {
	if (fileIsReadable(filename.c_str())) {
		std::remove(filename.c_str());
	}
}
