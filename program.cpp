#include "register_machine.h"
#include <iostream>
#include <string>

int main() {
	setlocale(LC_ALL, "Russian");

	std::string filename{ "examples/RM2.txt" };
	IMD::extended_register_machine erm(filename);
	erm.run();

	return 0;
}
