#include "basic_register_machine.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

// Запуск РМ
void basic_register_machine::run() {
	load_all_commands();
	execute_all_commands();
	print_output_registers();
}

// Загрузка всех команд
void basic_register_machine::load_all_commands() {
	std::ifstream ifs(this->_filename);
	std::string line;

	// Первая строка содержит аргументы, разделённые пробелами
	if (std::getline(ifs, line)) parse_input_arguments(line);

	// Последующие строки, за исключением последней, содержат метки
	size_t expected_number{ 0 };
	while (std::getline(ifs, line)) {
		auto separator_position = line.find(SEPARATOR);
		if (separator_position == std::string::npos) break;
		

		std::string number = line.substr(0, separator_position);
		std::string instruction = line.substr(separator_position + SEPARATOR.length());
		this->trim(instruction);

		if (std::stoi(number) != expected_number)
			throw std::invalid_argument("The instructions are not written in sequence");

		this->_commands.emplace_back(instruction);
		++expected_number;
	}

	// В последней строке описываются выходные регистры
	std::getline(ifs, line);
	this->parse_output_arguments(line);

	// Проверка, что после выходных аргументов ничего нет
	if (std::getline(ifs, line))
		throw std::invalid_argument("Unexpected data after output registers");
}

// Выполнение всех команд
void basic_register_machine::execute_all_commands() {
	while (true) {
		const auto& command = this->_commands[this->_carriage];

		// TODO: оператор композиции не реализован

		if (command.find(ASSIGNMENT) != std::string::npos) {
			execute_assigment_command(command);
			++this->_carriage;
		}

		if (command.find(IF) != std::string::npos) execute_condition_command(command);

		if (command.find(STOP) != std::string::npos) { // РМ завершает свою работу только по достижению остановочной инструкции
			execute_stop_command(command);
			break;
		}
	}
}

// Печать выходных регистров
void basic_register_machine::print_output_registers() const {
	for (const auto& x : this->_output_registers)
		std::cout << x << " = " << this->_registers.at(x) << std::endl;
}

// Проверка корректности условной инструкции
bool basic_register_machine::is_valid_condition_command(const std::string& command) const {
	std::string pattern{ R"(^\s*if\s+(\w+)\s*==\s*0\s+then\s+goto\s+(\d+)\s+else\s+goto\s+(\d+)\s*$)" }; // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Проверка корректности инструкции присваивания
bool basic_register_machine::is_valid_assignment_command(const std::string& command) const {
	std::string pattern{ R"(^\s*(\w+)\s*<-\s*(?:(\d+)|(\w+)\s*([+\-])\s*1|1\s*([+\-])\s*(\w+))\s*$)" };  // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Проверка корректности формата остановочной команды
bool basic_register_machine::is_valid_stop_command(const std::string& command) const {
	std::string pattern{ R"(^stop$)" };  // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Парсинг аргументов
void basic_register_machine::parse_input_arguments(const std::string& line) {
	std::string variable;
	std::istringstream iss(line);
	while (iss >> variable) {
		std::cout << "Введите значение для " << variable << ": ";
		std::cin >> this->_registers[variable];
	}
}

// Парсинг результатов (выходных регистров)
void basic_register_machine::parse_output_arguments(const std::string& line) {
	std::string variable;
	std::istringstream iss(line);
	while (iss >> variable)
		this->_output_registers.push_back(variable);
}

// Выполнение инструкции присваивания
void basic_register_machine::execute_assigment_command(const std::string& command) {
	if (!this->is_valid_assignment_command(command))
		throw std::runtime_error("The assignment statement has an invalid format");

	auto separator_position = command.find(ASSIGNMENT);
	std::string left_part = command.substr(0, separator_position);
	std::string right_part = command.substr(separator_position + ASSIGNMENT.length());

	trim(left_part);
	trim(right_part);

	// Обработка инструкции вида L: x <- x + 1 или L: x <- 1 + x
	auto operation = right_part.find(PLUS);
	if (operation != std::string::npos) {
		std::string left_operand = right_part.substr(0, operation);
		std::string right_operand = right_part.substr(operation + PLUS.length());

		trim(left_operand);
		trim(right_operand);

		if (left_operand == "1") this->_registers[left_part] = 1 + this->_registers[right_operand];
		else if (right_operand == "1") this->_registers[left_part] = this->_registers[left_operand] + 1;

		return;
	}

	// Обработка инструкции вида L: x <- x - 1 или L: x <- 1 - x
	operation = right_part.find(MINUS);
	if (operation != std::string::npos) {
		std::string left_operand = right_part.substr(0, operation);
		std::string right_operand = right_part.substr(operation + MINUS.length());

		trim(left_operand);
		trim(right_operand);

		if (left_operand == "1") this->_registers[left_part] = std::max(0, 1 - this->_registers[right_operand]);
		else if (right_operand == "1") this->_registers[left_part] = std::max(0, this->_registers[left_operand] - 1);

		return;
	}

	// Обработка инструкции вида L: x <- a, где a - положительное целое число
	this->_registers[left_part] = std::stoi(right_part);

	return;
}

// Выполнение условной инструкции 
void basic_register_machine::execute_condition_command(const std::string& command) {
	if (!this->is_valid_condition_command(command))
		throw std::runtime_error("The conditional construct has an invalid format");

	auto if_position = command.find(IF);
	auto then_position = command.find(THEN);
	auto else_position = command.find(ELSE);
	auto goto1_position = command.find(GOTO, if_position);
	auto goto2_position = command.find(GOTO, else_position);
	

	std::string condition = command.substr(if_position + IF.length(), then_position - if_position - std::string(IF).length());
	std::string true_L = command.substr(goto1_position + GOTO.length(), else_position - goto1_position - std::string(GOTO).length());
	std::string false_L = command.substr(goto2_position + GOTO.length());

	trim(condition);
	trim(true_L);
	trim(false_L);

	auto equal_position = condition.find(EQUAL);

	std::string left_part = condition.substr(0, equal_position);
	std::string right_part = condition.substr(equal_position + EQUAL.length());
	trim(left_part);
	trim(right_part);

	if (right_part != "0") throw std::invalid_argument(""); //TODO: остальные проверки

	if (this->_registers[left_part] == 0) this->_carriage = std::stoi(true_L);
	else this->_carriage = std::stoi(false_L);
}

// Выполнение остановочной инструкции
void basic_register_machine::execute_stop_command(const std::string& command) {
	if (!this->is_valid_stop_command(command))
		throw std::runtime_error("The stop instruction is in an invalid format");

	this->_carriage = 0;
}

// Удаление лишних пробелов слева и справа от строки
void basic_register_machine::trim(std::string& line) const {
	size_t start = line.find_first_not_of(" \t\r\n");
	size_t end = line.find_last_not_of(" \t\r\n");
	if (start == std::string::npos) line = "";
	else line = line.substr(start, end - start + 1);
}