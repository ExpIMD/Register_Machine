#include "register_machine.h"
#include <iostream>
#include <string>

int main() {
	setlocale(LC_ALL, "Russian");

	std::string filename{ "RM1.txt" };
	basic_register_machine RM(filename, true);

	RM.run();
	RM.print_output_registers(" ");




	return 0;
}