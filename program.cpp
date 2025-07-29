#include "register_machine.h"
#include <iostream>
#include <string>

int main() {
	setlocale(LC_ALL, "Russian");

	std::string filename{ "RM1.txt" };
	IMD::extended_register_machine erm(filename);
	erm.run();
/*
	std::ifstream ifs(filename);
	ifs.seekg(0);
	std::string line;
	while(std::getline(ifs, line)){
		std::cout << line << " <=> " << ifs.tellg() << std::endl;
	}

*/
	return 0;
}
