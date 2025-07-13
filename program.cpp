#include "register_machine.h"
#include <iostream>
#include <string>

int main() {
	setlocale(LC_ALL, "Russian");

	std::string filename{ "RM3.txt" };
	IMD::basic_register_machine RM(filename, true);
	try {
		RM.run();
	}
	catch (const std::exception& e) {
		std::cout << e.what();
	}

	return 0;
}
