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

#pragma once

#include <vector>
#include <string>
#include <fstream>

std::vector<std::string> getAllFilesInDirs(const std::string& prefix, const std::vector<std::string>& dirs);

std::string getFirstLineInFile(const std::string& filename);

void removeFile(const std::string& filename);

template<typename T>
bool fileIsReadable(const T* filename) { //quick and dirty
	std::ifstream f(filename, std::ios::in);
	bool exists = f.is_open();
	if (exists) {
		f.close();
	}
	return exists;
}
