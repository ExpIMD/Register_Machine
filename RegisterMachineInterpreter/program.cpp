#include "basic_register_machine.h"
#include <iostream>
#include <string>

int main() {
	setlocale(LC_ALL, "Russian");

	std::string filename{ "RM.txt" };
	basic_register_machine RM(filename);

	RM.run();



	return 0;
}