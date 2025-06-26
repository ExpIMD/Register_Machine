#include "register_machine.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

void register_machine::parse_input_arguments(const std::string& line) {
	std::string variable;
	std::istringstream iss(line);
	while (iss >> variable) {
		std::cout << "Введите значение для " << variable << ": ";
		std::cin >> this->_registers[variable];
	}
}

void register_machine::parse_output_arguments(const std::string& line) {
	std::string variable;
	std::istringstream iss(line);
	while (iss >> variable) 
		this->_output_registers.push_back(variable);
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
	execute_commands();
	print_output_registers();
}

void register_machine::print_output_registers() const {
	for (const auto& x : this->_output_registers)
		std::cout << x << " = " << this->_registers.at(x);
}

void register_machine::execute_commands() {
	while (true) {
		const auto& [mark, command] = this->_commands[this->_carriage];

		if (command.find(ASSIGNMENT) != std::string::npos) {
			assigment_command(command);
			++this->_carriage;
		}

		if (command.find(IF) != std::string::npos) {
			condition_command(command);
		}

		if (command.find(STOP) != std::string::npos) {
			stop_command();
			break;
		}


		


	}


}

void register_machine::condition_command(const std::string& command) {
	auto if_position = command.find(IF);
	auto then_position = command.find(THEN);
	auto else_position = command.find(ELSE);
	auto goto1_position = command.find(GOTO, if_position);
	auto goto2_position = command.find(GOTO, else_position);

	if (if_position == std::string::npos || then_position == std::string::npos || else_position == std::string::npos
		|| goto1_position == std::string::npos || goto2_position == std::string::npos)
		throw std::invalid_argument("");
	if (!(if_position < then_position && then_position < goto1_position && goto1_position < else_position && else_position < goto2_position))
		throw std::invalid_argument("");

	std::string condition = command.substr(if_position + 2, then_position - if_position - 2);
	std::string true_L = command.substr(goto1_position + 4, else_position - goto1_position - 4);
	std::string false_L = command.substr(goto2_position + 4);

	trim(condition);
	trim(true_L);
	trim(false_L);

	auto equal_pos = condition.find(EQUAL);
	if (equal_pos == std::string::npos)
		throw std::invalid_argument("");

	std::string left_part = condition.substr(0, equal_pos);
	std::string right_part = condition.substr(equal_pos + 2);
	trim(left_part);
	trim(right_part);

	if (right_part != "0") throw std::invalid_argument("");
	//TODO: остальные проверки

	if (this->_registers[left_part] == 0) this->_carriage = std::stoi(true_L);
	else this->_carriage = std::stoi(false_L);
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
			this->_registers[left_part] = std::max(0, 1 - this->_registers[right_operand]); // TODO: Всегда будет только +1
		}
		else if (right_operand == "1") {
			this->_registers[left_part] = std::max(0, this->_registers[left_operand] - 1);
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

	if (std::getline(ifs, line)) parse_input_arguments(line); // Первая строка - входные аргументы

	while (std::getline(ifs, line)) {
		auto temp = line.find(SEPARATOR);
		if (temp == std::string::npos) { // Дошли до выходных аргументов
			break;
		}

		std::string mark = line.substr(0, temp);
		std::string instruction = line.substr(temp + 1);
		this->_commands.emplace_back(mark, instruction); // TODO: добавить проверку на корректность марки инструкции
	}
	// Последняя строка - выходные аргументы
	std::getline(ifs, line);
	this->parse_output_arguments(line);
}