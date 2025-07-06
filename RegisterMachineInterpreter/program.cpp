#include "register_machine.h"
#include <iostream>
#include <string>

int main() {



	setlocale(LC_ALL, "Russian");

	std::string filename{ "RM5.txt" };
	IMD::extended_register_machine RM(filename, true);

	RM.run();

	try {
		
	}
	catch (const std::exception& e) {
		std::cout << e.what();
	}

	return 0;
}
