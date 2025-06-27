#include "register_machine.h"
#include <iostream>
#include <string>

int main() {
	setlocale(LC_ALL, "Russian");

	std::string filename{ "RM1.txt" };
	extended_register_machine RM(filename, false);

	RM.run();


	return 0;
}
