#include "register_machine.h"
#include <iostream>
#include <string>

int main() {
	setlocale(LC_ALL, "Russian");

	std::string filename{ "RM1.txt" };
	IMD::extended_register_machine RM(filename, true);
	RM.run();


	return 0;
}
