#include "register_machine.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

void basic_register_machine::reset() {
	this->_carriage = 0;
	this->_registers.clear();
	this->_instructions.resize(0);
	this->_output_registers.resize(0);
	this->_input_registers.resize(0);
	this->_filename = ""s;
}

void extended_register_machine::reset() {
	basic_register_machine::reset();
	this->input_register_values.resize(0);
}

void extended_register_machine::_include_files(const std::string& filename) {
	std::ifstream ifs(filename);
	std::string line;
	std::vector<std::string> filenames;

	_file_stack.push({ filename, true });

	while (std::getline(ifs, line) && line.find(COMPOSITION) != std::string::npos) {
		auto temp = line.find(COMPOSITION);
		auto filename = line.substr(temp + COMPOSITION.length());
		trim(filename);
		filenames.push_back(filename);
	}

	for (auto it = filenames.rbegin(); it != filenames.rend(); ++it) _file_stack.push({ *it, false });
}

// Запуск РМ
void basic_register_machine::run() {
	load_all_instructions();
	execute_all_instructions();
}

void extended_register_machine::execute() {
	while (!this->_file_stack.empty()) {
		auto [file, flag] = this->_file_stack.top();
		this->_file_stack.pop();

		if (!flag) {
			_include_files(file);
		}
		else {

			std::vector<int> results{};
			for (const auto& x : this->_output_registers)
				results.push_back(this->_registers[x]);

			this->reset();
			this->_filename = file;
			this->input_register_values = results;
			load_all_instructions();
			if (this->input_register_values.size() > 0) {
				size_t index{ 0 };
				for (const auto& x : this->_input_registers) {
					this->_registers[x] = this->input_register_values[index];
					++index;
				}
			}
			else {
				for (const auto& x : this->_input_registers) {
					std::cout << "Введите значения для " << x << ": ";
					std::cin >> this->_registers[x];
				}
			}
			execute_all_instructions();
			// Передача аргументов в РМ (возможно пустых для первого запуска)
			// Выполнение этой регистровой машины
			// Сохранение результата
			// Очищение РМ
		}
	}
}

// Запуск РМ
void extended_register_machine::run() {
	this->_include_files(_filename);
	this->execute();
}

// Загрузка всех команд
void basic_register_machine::load_all_instructions() {
	std::ifstream ifs(this->_filename);
	std::string line;

	// Первая строка содержит аргументы, разделённые пробелами
	if (std::getline(ifs, line))
		parse_input_arguments(line);

	// Последующие строки, за исключением последней, содержат метки

	size_t expected_number{ 0 };
	while (std::getline(ifs, line)) {
		auto separator_position = line.find(SEPARATOR);
		if (separator_position == std::string::npos) break;
		
		std::string number = line.substr(0, separator_position);
		std::string instruction = line.substr(separator_position + SEPARATOR.length());
		trim(instruction);

		if (std::stoi(number) != expected_number)
			throw std::invalid_argument("The instructions are not written in sequence");

		this->_instructions.push_back(instruction);
		++expected_number;
	}

	// В последней строке описываются выходные регистры
	std::getline(ifs, line);
	this->parse_output_arguments(line);

	// Проверка, что после выходных аргументов ничего нет
	if (std::getline(ifs, line))
		throw std::invalid_argument("Unexpected data after output registers");
}

// Загрузка всех команд
void extended_register_machine::load_all_instructions() {
	std::ifstream ifs(this->_filename);
	std::string line;

	// Обработка команд композиции
	while (std::getline(ifs, line) && line.find(COMPOSITION) != std::string::npos);

	// Обработка аргументов
	parse_input_arguments(line);

	// Последующие строки, за исключением последней, содержат метки

	size_t expected_number{ 0 };
	while (std::getline(ifs, line)) {
		auto separator_position = line.find(SEPARATOR);
		if (separator_position == std::string::npos) break;

		std::string number = line.substr(0, separator_position);
		std::string instruction = line.substr(separator_position + SEPARATOR.length());
		trim(instruction);

		if (std::stoi(number) != expected_number)
			throw std::invalid_argument("The instructions are not written in sequence");

		this->_instructions.emplace_back(instruction);
		++expected_number;
	}

	// В последней строке описываются выходные регистры
	std::getline(ifs, line);
	this->parse_output_arguments(line);

	// Проверка, что после выходных аргументов ничего нет
	if (std::getline(ifs, line))
		throw std::invalid_argument("Unexpected data after output registers");
}

void basic_register_machine::print_carriage(const std::string& separator) const {
	std::cout << "carriage: " << this->_carriage << separator;
}

// Выполнение всех команд
void basic_register_machine::execute_all_instructions() {
	while (true) {
		const auto& command = this->_instructions[this->_carriage];

		if (this->_is_verbose) {
			this->print_carriage(" ");
			this->print_all_registers(" ");
			std::cout << std::endl;
			std::cout << this->_carriage << ": " << command << std::endl;
		}

		// TODO: оператор композиции не реализован

		if (command.find(ASSIGNMENT) != std::string::npos) {
			this->execute_assigment_instruction(command);
			++this->_carriage;
		}

		if (command.find(IF) != std::string::npos)
			this->execute_condition_instruction(command);

		if (command.find(STOP) != std::string::npos) { // РМ завершает свою работу только по достижению остановочной инструкции
			this->execute_stop_instruction(command);
			break;
		}
	}
}

// Выполнение всех команд
void extended_register_machine::execute_all_instructions() {

	while (true) {
		const auto& instruction = this->_instructions[this->_carriage];

		if (this->_is_verbose) {
			this->print_all_registers(" ");
			std::cout << std::endl;
			std::cout << this->_carriage << ": " << instruction << std::endl;
		}

		// TODO: оператор композиции не реализован call
		// TODO: оператор сброса регистров reset
		// TODO: оператор деления и умножения
		// TODO: swap

		if (instruction.find(MOVE) != std::string::npos) {
			this->execute_move_command(instruction);
			++this->_carriage;
			continue;
		}

		if (instruction.find(ASSIGNMENT) != std::string::npos) {
			this->execute_assigment_instruction(instruction);
			++this->_carriage;
			continue;
		}

		if (instruction.find(IF) != std::string::npos) {
			this->execute_condition_instruction(instruction);
			continue;
		}

		if (instruction.find(GOTO) != std::string::npos) {
			this->execute_goto_command(instruction);
			continue;
		}

		if (instruction.find(STOP) != std::string::npos) { // РМ завершает свою работу только по достижению остановочной инструкции
			this->execute_stop_instruction(instruction);
			break;
		}
	}
}

void extended_register_machine::execute_goto_command(const std::string& command) {
	if (!is_valid_goto_command(command))
		throw std::runtime_error("The goto statement has an invalid format");

	auto goto_position = command.find(GOTO);
	auto L = command.substr(goto_position + GOTO.length());

	trim(L);
	this->_carriage = std::stoi(L);
}

bool extended_register_machine::is_valid_goto_command(const std::string& command) const {
	std::string pattern{ R"(^\s*goto\s+(\w+)\s*$)" }; // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

void extended_register_machine::execute_move_command(const std::string& command) {
	if (!this->is_valid_move_command(command))
		throw std::runtime_error("The assignment statement has an invalid format");

	auto separator_position = command.find(MOVE);
	std::string left_part = command.substr(0, separator_position);
	std::string right_part = command.substr(separator_position + MOVE.length());

	trim(left_part);
	trim(right_part);

	if (!is_variable(right_part) || !is_variable(left_part))
		throw std::runtime_error("");

	this->_registers[left_part] = this->_registers[right_part];
	this->_registers[right_part] = 0;

	return;
}

void basic_register_machine::print_all_registers(const std::string& separator) const {
	for (const auto& [reg, val] : this->_registers) std::cout << reg << ": " << val << separator;
}

// Печать входных регистров
void basic_register_machine::print_input_registers(const std::string& separator) const {
	for (const auto& reg : this->_input_registers) std::cout << reg << ": " << this->_registers.at(reg) << separator;
}

// Проверка корректности формата перемещающей команды
bool extended_register_machine::is_valid_move_command(const std::string& command) const {
	std::string pattern{ R"(^\s*(\w+)\s*<<-\s*(\w+)\s*$)"}; // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Печать выходных регистров
void basic_register_machine::print_output_registers(const std::string& separator) const {
	for (const auto& x : this->_output_registers)
		std::cout << x << ": " << this->_registers.at(x) << separator;
}

// Проверка корректности условной инструкции
bool basic_register_machine::is_valid_condition_instruction(const std::string& command) const {
	std::string pattern{ R"(^\s*if\s+(\w+)\s*==\s*0\s+then\s+goto\s+(\d+)\s+else\s+goto\s+(\d+)\s*$)" }; // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Проверка корректности условной инструкции
bool extended_register_machine::is_valid_condition_instruction(const std::string& command) const {
	std::string pattern{ R"(^\s*if\s+(\w+)\s*==\s*(\w+|\d+)\s+then\s+goto\s+(\d+)\s+else\s+goto\s+(\d+)\s*$)" }; // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Проверка корректности инструкции присваивания
bool basic_register_machine::is_valid_assignment_instruction(const std::string& command) const {
	std::string pattern{ R"(^\s*(\w+)\s*<-\s*(?:(\d+)|\1\s*([+\-])\s*1|1\s*([+\-])\s*\1)\s*$)" };
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Проверка корректности условной инструкции
bool extended_register_machine::is_valid_assignment_instruction(const std::string& command) const {
	std::string pattern{ R"(^\s*(\w+)\s*<-\s*(?:(\d+)|(\w+)\s*([+\-])\s*(\d+|\w+)|(\d+|\w+)\s*([+\-])\s*(\w+)|(\w+))\s*$)" }; // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}


// Проверка корректности формата остановочной команды
bool basic_register_machine::is_valid_stop_instruction(const std::string& command) const {
	std::string pattern{ R"(^stop$)" };  // TODO: не используются макросы
	std::regex regex{ pattern };
	return std::regex_match(command, regex);
}

// Парсинг аргументов
void basic_register_machine::parse_input_arguments(const std::string& line) {
	std::string variable;
	std::istringstream iss(line);
	while (iss >> variable) {
		_input_registers.push_back(variable);
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
void basic_register_machine::execute_assigment_instruction(const std::string& command) {
	if (!this->is_valid_assignment_instruction(command))
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

		this->_registers[left_part] = this->get_value(left_operand) + this->get_value(right_operand);

		return;
	}

	// Обработка инструкции вида L: x <- x - 1 или L: x <- 1 - x
	operation = right_part.find(MINUS);
	if (operation != std::string::npos) {
		std::string left_operand = right_part.substr(0, operation);
		std::string right_operand = right_part.substr(operation + MINUS.length());

		trim(left_operand);
		trim(right_operand);

		this->_registers[left_part] = std::max(0, this->get_value(left_operand) - this->get_value(right_operand));

		return;
	}

	// Обработка инструкции вида L: x <- a, где a - положительное целое число
	this->_registers[left_part] = this->get_value(right_part);

	return;
}

int basic_register_machine::get_value(const std::string& line) { // TODO: const модификатор
	if (std::all_of(line.begin(), line.end(), ::isdigit)) return std::stoi(line);
	if (this->is_variable(line)) return this->_registers[line];
	throw std::runtime_error("");
}

void extended_register_machine::execute_assigment_instruction(const std::string& command) {
	if (!this->is_valid_assignment_instruction(command))
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

		this->_registers[left_part] = this->get_value(left_operand) + this->get_value(right_operand);

		return;
	}

	// Обработка инструкции вида L: x <- x - 1 или L: x <- 1 - x
	operation = right_part.find(MINUS);
	if (operation != std::string::npos) {
		std::string left_operand = right_part.substr(0, operation);
		std::string right_operand = right_part.substr(operation + MINUS.length());

		trim(left_operand);
		trim(right_operand);

		this->_registers[left_part] = std::max(0, this->get_value(left_operand) - this->get_value(right_operand));

		return;
	}

	this->_registers[left_part] = this->get_value(right_part);

	return;
}

// Выполнение условной инструкции 
void basic_register_machine::execute_condition_instruction(const std::string& command) {
	if (!this->is_valid_condition_instruction(command))
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

void extended_register_machine::execute_condition_instruction(const std::string& command) {
	if (!this->is_valid_condition_instruction(command))
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

	if (this->is_variable(right_part)) {
		if (this->_registers[left_part] == this->_registers[right_part]) this->_carriage = std::stoi(true_L);
		else this->_carriage = std::stoi(false_L);
	}
	else {
		if (this->_registers[left_part] == std::stoi(right_part)) this->_carriage = std::stoi(true_L);
		else this->_carriage = std::stoi(false_L);
	}
}

// Выполнение остановочной инструкции
void basic_register_machine::execute_stop_instruction(const std::string& command) {
	if (!this->is_valid_stop_instruction(command))
		throw std::runtime_error("The stop instruction is in an invalid format");

	this->_carriage = 0;
}

// Удаление лишних пробелов слева и справа от строки
void trim(std::string& line) {
	size_t start = line.find_first_not_of(" \t\r\n");
	size_t end = line.find_last_not_of(" \t\r\n");
	if (start == std::string::npos) line = "";
	else line = line.substr(start, end - start + 1);
}

bool basic_register_machine::is_variable(const std::string& line) const {
	if (line.empty()) return false;

	if (!std::isalpha(line[0])) return false; // Первый символ регистра должен быть буквой

	for (size_t i{ 1 }; i < line.size(); ++i) // Остальные символы регистра могут быть буквами или цифрами
		if (!std::isalnum(line[i])) return false;

	return true;
}

// Оператор присваивания
basic_register_machine& basic_register_machine::operator=(const basic_register_machine& other) noexcept {
	if (this != &other) {
		this->_carriage = other._carriage;
		this->_registers = other._registers;
		this->_instructions = other._instructions;
		this->_output_registers = other._output_registers;
		this->_filename = other._filename;
		this->_is_verbose = other._is_verbose;
	}

	return *this;
}
// Оператор move-присваивания
basic_register_machine& basic_register_machine::operator=(basic_register_machine&& other) noexcept {
	this->_carriage = std::move(other._carriage);
	this->_registers = std::move(other._registers);
	this->_instructions = std::move(other._instructions);
	this->_output_registers = std::move(other._output_registers);
	this->_filename = std::move(other._filename);
	this->_is_verbose = std::move(other._is_verbose);
	return *this;
}