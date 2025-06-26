#include "register_machine.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

void register_machine::parsing_input_arguments(const std::string& line) {
	std::string variable;
	std::istringstream iss(line);
	while (iss >> variable) {
		this->_registers[variable] = 0;
	}
}

void register_machine::print_commands() const {
	for (const auto& x : this->_commands)
		std::cout << x.first << ":" << x.second << std::endl;
}
void register_machine::print_registers() const {
	for (const auto& x : this->_registers)
		std::cout << x.first << " = " << x.second << std::endl;
}

void register_machine::run() {
	load_commands();
	execute_command();
}

void register_machine::execute_command() {
	while (true) {
		const auto& [mark, command] = this->_commands[this->_carriage];

		auto temp = command.find(ASSIGNMENT);
		if (temp != std::string::npos) {
			assigment_command(command);
		}

		temp = command.find(STOP);
		if (temp != std::string::npos) {
			stop_command();
			break;
		}


		++this->_carriage;


	}
}

void register_machine::stop_command() {
	this->_carriage = 0;
}

void register_machine::trim(std::string& line) const {
	size_t start = line.find_first_not_of(" \t\r\n");
	size_t end = line.find_last_not_of(" \t\r\n");
	if (start == std::string::npos) line = "";
	else line = line.substr(start, end - start + 1);
}

void register_machine::assigment_command(const std::string& command) {
	auto separator = command.find(ASSIGNMENT);
	std::string left_part = command.substr(0, separator);
	std::string right_part = command.substr(separator + 2); //TODO: 2 - магическое число, нужно использовать length

	trim(left_part);
	trim(right_part);

	auto operation = right_part.find(PLUS); // Обработка плюса
	if (operation != std::string::npos) {
		std::string left_operand = right_part.substr(0, operation);
		std::string right_operand = right_part.substr(operation + 1);
		trim(left_operand);
		trim(right_operand);
		if (left_operand == "1") {
			this->_registers[left_part] = 1 + this->_registers[right_operand]; // TODO: Всегда будет только +1
		}
		else if (right_operand == "1") {
			this->_registers[left_part] = this->_registers[left_operand] + 1;
		}
		return;
	}

	operation = right_part.find(MINUS); // Обработка минуса
	if (operation != std::string::npos) {
		std::string left_operand = right_part.substr(0, operation);
		std::string right_operand = right_part.substr(operation + 1);
		trim(left_operand);
		trim(right_operand);
		if (left_operand == "1") {
			this->_registers[left_part] = 1 - this->_registers[right_operand]; // TODO: Всегда будет только +1
		}
		else if (right_operand == "1") {
			this->_registers[left_part] = this->_registers[left_operand] - 1;
		}
		return;
	}

	// Обработка присваивания целого числа
	this->_registers[left_part] = std::stoi(right_part);
	return;
}

void register_machine::load_commands() {
	std::ifstream ifs(this->_filename);
	std::string line;

	if (std::getline(ifs, line)) parsing_input_arguments(line); // Первая строка - входные аргументы

	while (std::getline(ifs, line)) {
		auto temp = line.find(SEPARATOR);
		if (temp == std::string::npos) { // Дошли до выходных аргументов
			break;
		}

		std::string mark = line.substr(0, temp);
		std::string instruction = line.substr(temp + 1);
		this->_commands.emplace_back(mark, instruction); // TODO: добавить проверку на корректность марки инструкции
	}
}