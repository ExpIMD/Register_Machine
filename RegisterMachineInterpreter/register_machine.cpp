#include "register_machine.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace IMD {

	// Реализация инструкций

	const std::string& instruction::description() const noexcept {
		return this->_description;
	}

	void goto_instruction::execute() noexcept {
		auto goto_position = this->_description.find(GOTO);
		auto L = this->_description.substr(goto_position + GOTO.length());

		trim(L);

		this->_rm._carriage = std::stoi(L);
	}

	void move_instruction::execute() noexcept {
		++this->_rm._carriage;

		auto separator_position = this->_description.find(MOVE);
		std::string left_part = this->_description.substr(0, separator_position);
		std::string right_part = this->_description.substr(separator_position + MOVE.length());

		trim(left_part);
		trim(right_part);

		if (!is_register(right_part) || !is_register(left_part))
			throw std::runtime_error("");

		this->_rm._registers[left_part] = this->_rm._registers[right_part];
		this->_rm._registers[right_part] = 0;

		return;
	}

	void assigment_instruction::execute() noexcept {

		++this->_rm._carriage;

		auto separator_position = this->_description.find(ASSIGNMENT);
		std::string left_part = this->_description.substr(0, separator_position);
		std::string right_part = this->_description.substr(separator_position + ASSIGNMENT.length());

		trim(left_part);
		trim(right_part);

		// TODO: использование substr плохо

		// Обработка инструкции инкремента
		auto operation = right_part.find(PLUS);
		if (operation != std::string::npos) {
			std::string left_operand = right_part.substr(0, operation);
			std::string right_operand = right_part.substr(operation + PLUS.length());

			trim(left_operand);
			trim(right_operand);

			this->_rm._registers[left_part] = this->_rm.get_value(left_operand) + this->_rm.get_value(right_operand);

			return;
		}

		// Обработка инструкции декремента
		operation = right_part.find(MINUS);
		if (operation != std::string::npos) {
			std::string left_operand = right_part.substr(0, operation);
			std::string right_operand = right_part.substr(operation + MINUS.length());

			trim(left_operand);
			trim(right_operand);

			this->_rm._registers[left_part] = std::max(0, this->_rm.get_value(left_operand) - this->_rm.get_value(right_operand));

			return;
		}

		// Обработка инструкции присваивания (копирования)
		this->_rm._registers[left_part] = this->_rm.get_value(right_part);

		return;
	}

	void condition_instruction::execute() noexcept {
		auto if_position = this->_description.find(IF);
		auto then_position = this->_description.find(THEN);
		auto else_position = this->_description.find(ELSE);
		auto goto1_position = this->_description.find(GOTO, if_position);
		auto goto2_position = this->_description.find(GOTO, else_position);

		std::string condition = this->_description.substr(if_position + IF.length(), then_position - if_position - std::string(IF).length());
		std::string true_L = this->_description.substr(goto1_position + GOTO.length(), else_position - goto1_position - std::string(GOTO).length());
		std::string false_L = this->_description.substr(goto2_position + GOTO.length());

		trim(condition);
		trim(true_L);
		trim(false_L);

		auto equal_position = condition.find(EQUAL);

		std::string left_part = condition.substr(0, equal_position);
		std::string right_part = condition.substr(equal_position + EQUAL.length());

		trim(left_part);
		trim(right_part);

		if (this->_rm._registers[left_part] == 0) this->_rm._carriage = std::stoi(true_L);
		else this->_rm._carriage = std::stoi(false_L);
	}

	void stop_instruction::execute() noexcept {
		this->_rm._is_stopped = true;
	}

	void extended_condition_instruction::execute() noexcept {
		auto if_position = this->_description.find(IF);
		auto then_position = this->_description.find(THEN);
		auto else_position = this->_description.find(ELSE);
		auto goto1_position = this->_description.find(GOTO, if_position);
		auto goto2_position = this->_description.find(GOTO, else_position);

		std::string condition = this->_description.substr(if_position + IF.length(), then_position - if_position - std::string(IF).length());
		std::string true_L = this->_description.substr(goto1_position + GOTO.length(), else_position - goto1_position - std::string(GOTO).length());
		std::string false_L = this->_description.substr(goto2_position + GOTO.length());

		trim(condition);
		trim(true_L);
		trim(false_L);

		auto equal_position = condition.find(EQUAL);

		std::string left_part = condition.substr(0, equal_position);
		std::string right_part = condition.substr(equal_position + EQUAL.length());

		trim(left_part);
		trim(right_part);

		if (is_register(right_part)) {
			if (this->_rm._registers[left_part] == this->_rm._registers[right_part]) this->_rm._carriage = std::stoi(true_L);
			else this->_rm._carriage = std::stoi(false_L);
		}
		else {
			if (this->_rm._registers[left_part] == std::stoi(right_part)) this->_rm._carriage = std::stoi(true_L);
			else this->_rm._carriage = std::stoi(false_L);
		}
	}

	// Реализация вспомогательных методов

	// Удаление лишних пробелов слева и справа от строки
	void trim(std::string& line) {
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		if (line.empty()) return;
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
	}

	bool is_register(const std::string& line) noexcept {
		if (line.empty())
			return false;

		if (!std::isalpha(line[0]))
			return false; // Первый символ регистра должен быть буквой

		for (size_t i{ 1 }; i < line.size(); ++i) // Остальные символы регистра могут быть буквами или цифрами
			if (!std::isalnum(line[i]))
				return false;

		return true;
	}

	// Реализация базовой РМ

	// Конструктор
	basic_register_machine::basic_register_machine(const std::string& filename, bool is_verbose) noexcept : _filename(filename), _is_verbose(is_verbose), _carriage(0), _registers(), _instructions(), _output_registers(), _is_stopped(false) {}

	// Запуск РМ
	void basic_register_machine::run() {
		this->load_all_instructions();

		for (const auto& x : this->_input_registers) { // Запрос ввода значения для входных регистров
			std::cout << "Введите значения для " << x << ": ";
			std::cin >> this->_registers[x];
		}

		this->execute_all_instructions();
		this->print_output_registers(" "); // Вывод выходных регистров
	}

	// Сброс РМ
	void basic_register_machine::reset() {
		this->_carriage = 0;
		this->_registers.clear();
		this->_instructions.resize(0);
		this->_output_registers.resize(0);
		this->_input_registers.resize(0);
		this->_filename = ""s;
		this->_is_stopped = false;
	}

	// Печать входных регистров без перехода на новую строку
	void basic_register_machine::print_input_registers(const std::string& separator) const noexcept {
		for (const auto& reg : this->_input_registers) std::cout << reg << ": " << this->_registers.at(reg) << separator;
	}
	// Печать входных регистров с переходом на новую строку
	void basic_register_machine::println_input_registers(const std::string& separator) const noexcept {
		basic_register_machine::print_input_registers(separator);
		std::cout << std::endl;
	}

	// Печать всех регистров без перехода на новую строку
	void basic_register_machine::print_all_registers(const std::string& separator) const noexcept {
		for (const auto& [x, y] : this->_registers) std::cout << x << ": " << y << separator;
	}
	// Печать всех регистров с переходом на новую строку
	void basic_register_machine::println_all_registers(const std::string& separator) const noexcept {
		basic_register_machine::print_all_registers(separator);
		std::cout << std::endl;
	}

	// Печать выходных регистров без перехода на новую строку
	void basic_register_machine::print_output_registers(const std::string& separator) const noexcept {
		for (const auto& x : this->_output_registers)
			std::cout << x << ": " << this->_registers.at(x) << separator; // TODO: ошибка если переменная объявлена только на выходе
	}
	// Печать выходных регистров с переходом на новую строку
	void basic_register_machine::println_output_registers(const std::string& separator) const noexcept {
		basic_register_machine::print_output_registers(separator);
		std::cout << std::endl;
	}

	// Печать каретки без перехода на новую строку
	void basic_register_machine::print_carriage(const std::string& separator) const noexcept {
		std::cout << "carriage: " << this->_carriage << separator;
	}
	// Печать каретки с переходом на новую строку
	void basic_register_machine::println_carriage(const std::string& separator) const noexcept {
		basic_register_machine::print_carriage(separator);
		std::cout << std::endl;
	}

	// Загрузка всех команд
	void basic_register_machine::load_all_instructions() {
		std::ifstream ifs(this->_filename);
		std::string line;

		// TODO: проверка формата

		if (!ifs) // Выброс исключения, если файл не найден
			throw std::runtime_error("Error processing file");

		// Первая строка содержит аргументы, разделённые пробелами
		if (std::getline(ifs, line)) {
			this->parse_input_registers(line);
		}

		// Последующие строки, за исключением последней, содержат метки
		size_t expected_number{ 0 }; // Ожидаемый номер метки
		while (std::getline(ifs, line)) {
			auto separator_position = line.find(SEPARATOR);
			if (separator_position == std::string::npos)
				break;

			std::string number = line.substr(0, separator_position);
			std::string instruction = line.substr(separator_position + SEPARATOR.length());

			trim(instruction);
			trim(number);

			if (std::stoi(number) != expected_number)
				throw std::invalid_argument("Filename: " + this->_filename + ", the instructions are not written in sequence");

			if (this->is_valid_assignment_instruction(instruction)) {
				++expected_number;
				this->_instructions.push_back(std::make_unique<assigment_instruction>(instruction, *this));
			}
			else if (this->is_valid_condition_instruction(instruction)) {
				this->_instructions.push_back(std::make_unique<condition_instruction>(instruction, *this));
				++expected_number;
			}
				
			else if (this->is_valid_stop_instruction(instruction)) {
				this->_instructions.push_back(std::make_unique<stop_instruction>(instruction, *this));
				++expected_number;
			}
			else throw std::invalid_argument("Filename: " + this->_filename + ", " + instruction + " isn't correct");
		}

		// В последней строке описываются выходные регистры
		this->parse_output_registers(line);

		// Проверка, что после выходных аргументов ничего нет
		if (std::getline(ifs, line))
			throw std::invalid_argument("Filename: " + this->_filename + ", unexpected data after output registers");
	}

	// Выполнение всех инструкций
	void basic_register_machine::execute_all_instructions() {
		while (!this->_is_stopped) {
			const auto& current_instruction = this->_instructions[this->_carriage];

			// Вывод текущего состояния РМ
			if (this->_is_verbose) {
				this->println_all_registers();
				std::cout << this->_carriage << ": " << current_instruction->description() << std::endl;
			}

			current_instruction->execute();
		}
	}


	// Проверка корректности условной инструкции
	bool basic_register_machine::is_valid_condition_instruction(const std::string& instruction) const noexcept {
		std::string pattern{ R"(^\s*)"s + IF + R"(\s+(\w+)\s*)"s + EQUAL + R"(\s*0\s+)"s + THEN + R"(\s+)"s + GOTO + R"(\s+(\d+)\s+)"s + ELSE + R"(\s+)"s + GOTO + R"(\s+(\d+)\s*$)"s };
		std::regex regex{ pattern };
		return std::regex_match(instruction, regex);
	}
	// Проверка корректности инструкции присваивания
	bool basic_register_machine::is_valid_assignment_instruction(const std::string& instruction) const noexcept {
		const std::string pm = "["s + PLUS + MINUS + "]"s;

		std::string pattern{ R"(^\s*(\w+)\s*)"s + ASSIGNMENT + R"(\s*(?:(\d+)|\1\s*)"s + pm + R"(\s*1|1\s*)"s + pm + R"(\s*\1)\s*$)"s };
		std::regex regex{ pattern };
		return std::regex_match(instruction, regex);
	}
	// Проверка корректности формата остановочной команды
	bool basic_register_machine::is_valid_stop_instruction(const std::string& instruction) const noexcept {
		std::string pattern{ R"(^\s*)"s + STOP + R"(\s*$)"s };
		std::regex regex{ pattern };
		return std::regex_match(instruction, regex);
	}
	// Проверка корректности формата строки с входными регистрами
	bool basic_register_machine::is_valid_input_registers_line(const std::string& instruction) const noexcept {
		std::string pattern{ R"(^\s*(\w+)(\s+\w+)*\s*$)"s };
		std::regex regex{ pattern };
		return std::regex_match(instruction, regex);
	}
	// Проверка корректности формата строки с выходными регистрами
	bool basic_register_machine::is_valid_output_registers_line(const std::string& instruction) const noexcept {
		std::string pattern{ R"(^\s*(\w+)(\s+\w+)*\s*$)"s };
		std::regex regex{ pattern };
		return std::regex_match(instruction, regex);
	}

	// Парсинг входных регистров
	void basic_register_machine::parse_input_registers(const std::string& line) {
		if (!this->is_valid_input_registers_line(line))
			throw std::runtime_error("Filename: " + this->_filename + ", the line does not contain input registers");

		std::string variable;
		std::istringstream iss(line);
		while (iss >> variable)
			this->_input_registers.push_back(variable);
	}

	// Парсинг выходных регистров
	void basic_register_machine::parse_output_registers(const std::string& line) {
		if (!this->is_valid_output_registers_line(line))
			throw std::runtime_error("Filename: " + this->_filename + ", the line does not contain output registers");

		std::string variable;
		std::istringstream iss(line);
		while (iss >> variable)
			this->_output_registers.push_back(variable);
	}

	// Получает целочисленное значение из строки, в которой может быть описани литерал или регистр
	int basic_register_machine::get_value(const std::string& line) {
		if (std::all_of(line.begin(), line.end(), ::isdigit)) return std::stoi(line);
		if (is_register(line)) return this->_registers[line];
		throw std::runtime_error("");
	}

	// Реализация расширенной РМ

	// Конструктор
	extended_register_machine::extended_register_machine(const std::string& filename, bool is_verbose) noexcept : basic_register_machine(filename, is_verbose), _file_stack() {}

	// Запуск РМ
	void extended_register_machine::run() {
		this->_include_files(_filename); // Обрабатываем исходный файл
		while (!this->_file_stack.empty()) {
			auto [file, flag] = this->_file_stack.top(); // Получаем верхний файл и флаг (обработан или нет)
			this->_file_stack.pop();

			if (!flag)
				this->_include_files(file); // Обрабатываем верхний файл
			else {
				std::vector<int> results{}; // Вектор промежуточных результатов
				for (const auto& x : this->_output_registers)
					results.push_back(this->_registers[x]);

				// Обновление состояния РМ перед новым запуском
				this->reset();
				this->_filename = file;
				this->load_all_instructions();

				if (results.size() > 0) { // Если есть сохранённые результаты, передаем их во входные регистры
					size_t index{ 0 };
					for (const auto& x : this->_input_registers) {
						this->_registers[x] = results[index];
						++index;
					}
				}
				else { // Если результатов нет (первый запуск), запрашиваем значения для входных регистров у пользователя
					for (const auto& x : this->_input_registers) {
						std::cout << "Введите значения для " << x << ": ";
						std::cin >> this->_registers[x];
					}
				}

				this->execute_all_instructions(); // Выполнение верхнего файла
			}
		}
		this->print_output_registers(" "); // После выполнения всех файлов выводим значения выходных регистров
	}

	// Загрузка всех команд
	void extended_register_machine::load_all_instructions() {
		std::ifstream ifs(this->_filename);
		std::string line;

		if (!ifs) // Выброс исключения, если файл не найден
			throw std::runtime_error("Filename: " + this->_filename + ", error processing file");


		while (std::getline(ifs, line) && line.find(COMPOSITION) != std::string::npos); // Команда композиции не является инструкцией, поэтому пропускаем её

		// Обработка входных регистров
		this->parse_input_registers(line);

		// Последующие строки, за исключением последней, содержат метки

		size_t expected_number{ 0 };
		while (std::getline(ifs, line)) {
			auto separator_position = line.find(SEPARATOR);
			if (separator_position == std::string::npos)
				break;

			std::string number = line.substr(0, separator_position);
			std::string instruction = line.substr(separator_position + SEPARATOR.length());

			trim(instruction);
			trim(number);

			if (std::stoi(number) != expected_number)
				throw std::invalid_argument("Filename: " + this->_filename + ", the instructions are not written in sequence");

			if (this->is_valid_assignment_instruction(instruction)) {
				++expected_number;
				this->_instructions.push_back(std::make_unique<assigment_instruction>(instruction, *this));
			}
			else if (this->is_valid_condition_instruction(instruction)) {
				this->_instructions.push_back(std::make_unique<extended_condition_instruction>(instruction, *this));
				++expected_number;
			}

			else if (this->is_valid_stop_instruction(instruction)) {
				this->_instructions.push_back(std::make_unique<stop_instruction>(instruction, *this));
				++expected_number;
			}
			else if (this->is_valid_goto_instruction(instruction)) {
				this->_instructions.push_back(std::make_unique<goto_instruction>(instruction, *this));
				++expected_number;
			}
			else if (this->is_valid_move_instruction(instruction)) {
				this->_instructions.push_back(std::make_unique<move_instruction>(instruction, *this));
				++expected_number;
			}
			else throw std::invalid_argument("Filename: " + this->_filename + ", " + instruction + " isn't correct");
		}

		// В последней строке описываются выходные регистры
		this->parse_output_registers(line);

		// Проверка, что после выходных аргументов ничего нет
		if (std::getline(ifs, line))
			throw std::invalid_argument("Filename: " + this->_filename + ", unexpected data after output registers");
	}

	// Выполнение всех команд
	void extended_register_machine::execute_all_instructions() {

		while (!this->_is_stopped) {
			const auto& current_instruction = this->_instructions[this->_carriage];

			// Вывод текущего состояния РМ
			if (this->_is_verbose) {
				this->println_all_registers(" ");
				std::cout << this->_carriage << ": " << current_instruction->description() << std::endl;
			}

			this->_instructions[this->_carriage]->execute();
		}
	}
	// Проверка корректности формата условной инструкции
	bool extended_register_machine::is_valid_condition_instruction(const std::string& command) const noexcept {
		std::string pattern{ R"(^\s*if\s+(\w+)\s*==\s*(\w+|\d+)\s+then\s+goto\s+(\d+)\s+else\s+goto\s+(\d+)\s*$)" }; // TODO: не используются макросы
		std::regex regex{ pattern };
		return std::regex_match(command, regex);
	}

	// Проверка корректности формата инструкции присваивания
	bool extended_register_machine::is_valid_assignment_instruction(const std::string& command) const noexcept {
		std::string pattern{ R"(^\s*(\w+)\s*<-\s*(?:(\d+)|(\w+)\s*([+\-])\s*(\d+|\w+)|(\d+|\w+)\s*([+\-])\s*(\w+)|(\w+))\s*$)" }; // TODO: не используются макросы
		std::regex regex{ pattern };
		return std::regex_match(command, regex);
	}

	// Проверка корректности формата инструкции перемещения
	bool extended_register_machine::is_valid_move_instruction(const std::string& command) const noexcept {
		std::string pattern{ R"(^\s*(\w+)\s*)"s + MOVE + R"(\s*(\w+)\s*$)" };
		std::regex regex{ pattern };
		return std::regex_match(command, regex);
	}
	// Проверка корректности формата инструкции передвижения
	bool extended_register_machine::is_valid_goto_instruction(const std::string& command) const noexcept {
		std::string pattern{ R"(^\s*)"s + GOTO + R"(\s+(\w+)\s*$)" };
		std::regex regex{ pattern };
		return std::regex_match(command, regex);
	}

	// Обработка всех команд композиции в текущем файла и добавление включаемых файлов в стек
	void extended_register_machine::_include_files(const std::string& filename) {
		std::ifstream ifs(filename);
		std::string line;
		std::vector<std::string> filenames;

		_file_stack.push({ filename, true });

		while (std::getline(ifs, line)) {
			auto composition_position = line.find(COMPOSITION);
			if (composition_position == std::string::npos) break;

			auto filename = line.substr(composition_position + COMPOSITION.length()); // Получение имени включаемого файла

			trim(filename);

			filenames.push_back(filename);
		}

		for (auto it = filenames.rbegin(); it != filenames.rend(); ++it)
			_file_stack.push({ *it, false });
	}
}