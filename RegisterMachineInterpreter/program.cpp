#include "register_machine.h"
#include <string>
#include <iostream>

int main() {
	std::string filename{ "RM.txt" };
	register_machine RM(filename);

	RM.run();

	RM.print_commands();
	std::cout << std::endl;
	RM.print_registers();



	return 0;
}