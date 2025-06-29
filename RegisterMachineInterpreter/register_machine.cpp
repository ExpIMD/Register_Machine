#include "register_machine.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace IMD {

	// Реализация токена

	// Конструктор
	basic_register_machine::token::token(const token_type& type, const std::string& text) noexcept: _type(type), _text(text) {}

	// Возвращает тип токена
	const token_type& basic_register_machine::token::type() const noexcept {
		return this->_type;
	}
	// Возвращает тип токена
	const std::string& basic_register_machine::token::text() const noexcept {
		return this->_text;
	}

	// Реализация базового лексера

	// Конструктор
	basic_register_machine::basic_lexer::basic_lexer(std::string_view line) noexcept : _line(line), _carriage(0) {}

	// Возвращает вектор токенов
	std::vector<basic_register_machine::token> basic_register_machine::basic_lexer::tokenize() {
		std::vector<token> tokens{};
		while (true) {
			this->skip_spaces();
			if (eof()) break;

			if (auto token = this->next_token())
				tokens.push_back(*token);
			else
				throw std::runtime_error("Unknown token at position: " + std::to_string(this->_carriage));
		}
		tokens.emplace_back(token_type::eof, "");
		return tokens;
	}

	// Возвращает следующий токен
	std::optional<basic_register_machine::token> basic_register_machine::basic_lexer::next_token() {
		if (this->eof())
			return std::nullopt;

		char c = this->_line[this->_carriage];
		if (std::isalpha(c))
			return this->parse_register_or_keyword();
		if (std::isdigit(c))
			return this->parse_literal();
		if (c == '<') // TODO: МАКРОСЫ
			return parse_operator_copy_assignment();
		if (c == '+') {
			++this->_carriage;
			return token{ token_type::operator_plus, PLUS };
		}
		if (c == '-') {
			++this->_carriage;
			return token{ token_type::operator_minus, MINUS };
		}
		if (c == '=')
			return parse_operator_equal();

		return std::nullopt;
	}

	// Парсинг регистра или ключевого слова
	basic_register_machine::token basic_register_machine::basic_lexer::parse_register_or_keyword() {
		size_t start = this->_carriage;
		while (!this->eof() && std::isalnum(this->_line[this->_carriage]))
			++this->_carriage;

		std::string text = std::string(_line.substr(start, this->_carriage - start));

		// Ключевые слова
		if (text == IF)
			return { token_type::keyword_if, text };
		if (text == THEN)
			return { token_type::keyword_then, text };
		if (text == ELSE)
			return { token_type::keyword_else, text };
		if (text == GOTO)
			return { token_type::keyword_goto, text };
		if (text == STOP)
			return { token_type::keyword_stop, text };

		return { token_type::variable, text }; // Встречен регистр
	}

	// Парсинг литерала
	basic_register_machine::token basic_register_machine::basic_lexer::parse_literal() {
		size_t start = this->_carriage;
		while (!this->eof() && std::isdigit(_line[this->_carriage]))
			++this->_carriage;
		return { token_type::literal, std::string(this->_line.substr(start, this->_carriage - start)) };
	}

	// Парсинг операторов копирующего и перемещающего присваивания
	basic_register_machine::token basic_register_machine::basic_lexer::parse_operator_copy_assignment() {
		if (this->_carriage + COPY.length() - 1 < this->_line.size()) {
			if (this->_line.substr(this->_carriage, COPY.length()) == COPY) {
				this->_carriage += COPY.length();
				return { token_type::operator_copy_assignment, COPY };
			}
		}

		throw std::runtime_error("Invalid operator starting with '<' at position: " + std::to_string(this->_carriage));
	}

	// Парсинг оператора сравнения на равенство
	basic_register_machine::token basic_register_machine::basic_lexer::parse_operator_equal() {


		if (this->_carriage + EQUAL.length() - 1 < this->_line.size() && this->_line.substr(this->_carriage, EQUAL.length()) == EQUAL) {
			this->_carriage += EQUAL.length();
			return { token_type::operator_equal, EQUAL };
		}
		throw std::runtime_error("Invalid operator '=' at posistion: " + std::to_string(this->_carriage));
	}

	// Пропуск пробелов
	void basic_register_machine::basic_lexer::skip_spaces() noexcept {
		while (!this->eof() && std::isspace(this->_line[this->_carriage]))
			++this->_carriage;
	}

	// Проверка на конец строки
	bool basic_register_machine::basic_lexer::eof() const noexcept {
		return this->_carriage >= this->_line.size();
	}

	// Реализация расширенного лексера

	extended_register_machine::extended_lexer::extended_lexer(const std::string& line) noexcept : basic_lexer(line) {}

	// Парсинг оператора копирующего присваивания
	basic_register_machine::token extended_register_machine::extended_lexer::parse_operator_move_assignment() {
		if (this->_carriage + MOVE.size() <= this->_line.size() && this->_line.substr(this->_carriage, MOVE.size()) == MOVE) {
			this->_carriage += MOVE.size();
			return token{ token_type::operator_move_assignment, MOVE };
		}

		throw std::runtime_error("Invalid operator starting with '<' at position: " + std::to_string(this->_carriage));
	}

	// Возвращает следующий токен
	std::optional<basic_register_machine::token> extended_register_machine::extended_lexer::next_token() {
		if (this->eof())
			return std::nullopt;

		char c = this->_line[this->_carriage];
		if (std::isalpha(c))
			return this->parse_register_or_keyword();
		if (std::isdigit(c))
			return this->parse_literal();
		if (c == '<') // TODO: МАКРОСЫ
			return parse_operator_copy_assignment();
		if (c == '~')
			return parse_operator_move_assignment();
		if (c == '+') {
			++this->_carriage;
			return token{ token_type::operator_plus, PLUS };
		}
		if (c == '-') {
			++this->_carriage;
			return token{ token_type::operator_minus, MINUS };
		}
		if (c == '=')
			return parse_operator_equal();

		return std::nullopt;
	}

	// Реализация расширенного парсерса
	
	// Конструктор
	extended_register_machine::extended_parser::extended_parser(const std::vector<token>& tokens) noexcept : basic_parser(tokens) {};

	// Парсинг произвольной инструкции
	instruction_ptr extended_register_machine::extended_parser::parse_instruction(basic_register_machine& rm) {
		if (this->eof())
			throw std::runtime_error("Empty instruction");

		auto current_type = this->preview().type();

		if (current_type == token_type::keyword_stop) {
			return this->parse_stop_instruction(rm);
		}
		else if (current_type == token_type::keyword_if) {
			return this->parse_condition_instruction(rm);
		}
		else if (current_type == token_type::keyword_goto) {
			return this->parse_goto_assignment_instruction(rm);
		}
		else if (current_type == token_type::variable) {

			auto next_token = this->_tokens[this->_carriage + 1];
			if (next_token.type() == token_type::operator_copy_assignment)
				return this->parse_copy_assignment_instruction(rm);
			else if (next_token.type() == token_type::operator_move_assignment) {
				return this->parse_move_assignment_instruction(rm);
			}
			else {
				throw std::runtime_error("Expected assignment operator after register");
			}
		}
		throw std::runtime_error("Unknown instruction start");

	}

	instruction_ptr extended_register_machine::extended_parser::parse_goto_assignment_instruction(basic_register_machine& rm) {
		++this->_carriage;

		auto number_token = this->preview();

		if (number_token.type() != token_type::literal)
			throw std::runtime_error("Expected number after '" + GOTO + "'");

		size_t mark = std::stoul(number_token.text());

		++this->_carriage;

		return std::make_unique<goto_instruction>(rm, mark);

	}

	instruction_ptr extended_register_machine::extended_parser::parse_move_assignment_instruction(basic_register_machine& rm) {
		auto to_register_token = this->preview();

		++this->_carriage;

		// Обработка целевого регистра
		if (to_register_token.type() != token_type::variable)
			throw std::runtime_error("Expected register at start of move assignment");

		// Обработка оператора копирующего присваивания
		if (!this->is_type_match(token_type::operator_move_assignment))
			throw std::runtime_error("Expected '"s + MOVE + "' after target register"s);

		++this->_carriage;

		// Обработка левого операнда
		auto from_register_token = this->preview();

		if (from_register_token.type() != token_type::variable)
			throw std::runtime_error("Expected register after '"s + MOVE + "'"s);
		
		return std::make_unique<move_assignment_instruction>(rm, to_register_token.text(), from_register_token.text());
	}

	instruction_ptr extended_register_machine::extended_parser::parse_copy_assignment_instruction(basic_register_machine& rm) {
		auto target_token = this->preview();

		++this->_carriage;

		// Обработка целевого регистра
		if (target_token.type() != token_type::variable)
			throw std::runtime_error("Expected register at start of copy assignment");

		// Обработка оператора копирующего присваивания
		if (!this->is_type_match(token_type::operator_copy_assignment))
			throw std::runtime_error("Expected '"s + COPY + "' after target register"s);

		++this->_carriage;

		// Обработка левого операнда
		auto left_operand_token = this->preview();

		if (left_operand_token.type() != token_type::variable && left_operand_token.type() != token_type::literal)
			throw std::runtime_error("Expected register or literal after '"s + COPY + "'"s);

		++this->_carriage;

		// Проверяем есть ли операция
		if (this->is_type_match(token_type::operator_plus)) {
			++this->_carriage;
			auto right_operand_token = this->preview();
			++this->_carriage;
			if (right_operand_token.type() != token_type::literal && right_operand_token.type() != token_type::variable)
				throw std::runtime_error("Expected number or literal after '" + PLUS "'");

			if (right_operand_token.type() == token_type::literal && std::stoi(right_operand_token.text()) < 0)
				throw std::runtime_error("Only positive integers allowed for subtraction");

			return std::make_unique<copy_assignment_instruction>(rm,
				target_token.text(),
				copy_assignment_instruction::operation::Add,
				left_operand_token.text(),
				right_operand_token.text());
		}
		else if (this->is_type_match(token_type::operator_minus)) {
			++this->_carriage;

			auto right_operand_token = this->preview();

			++this->_carriage;

			if (right_operand_token.type() != token_type::literal && right_operand_token.type() != token_type::variable) // TODO: странно обрабатывает отрицательные числа
				throw std::runtime_error("Expected number or literal after '" + MINUS "'");

			if (right_operand_token.type() == token_type::literal && std::stoi(right_operand_token.text()) < 0)
				throw std::runtime_error("Only positive integers allowed for subtraction");

			return std::make_unique<copy_assignment_instruction>(rm,
				target_token.text(),
				copy_assignment_instruction::operation::Subtract,
				left_operand_token.text(),
				right_operand_token.text());
		}
		else {
			// Простое присваивание: x <- a, где a — положительное число
			if (left_operand_token.type() == token_type::literal) {
				if (std::stoi(left_operand_token.text()) < 0)
					throw std::runtime_error("Only positive integers allowed for assignment");

				return std::make_unique<copy_assignment_instruction>(rm,
					target_token.text(),
					copy_assignment_instruction::operation::None,
					left_operand_token.text(),
					"");
			}
			else if (left_operand_token.type() == token_type::variable) {
				return std::make_unique<copy_assignment_instruction>(rm,
					target_token.text(),
					copy_assignment_instruction::operation::None,
					left_operand_token.text(),
					"");
			}
			else throw std::runtime_error("Expected number or literal in " + COPY);
		}
	}

	// Реализация базового парсера

	// Конструктор
	basic_register_machine::basic_parser::basic_parser(const std::vector<token>& tokens) noexcept : _tokens(tokens), _carriage(0) {}

	// Проверка на конец вектора токенов
	bool basic_register_machine::basic_parser::eof() const noexcept {
		return this->_carriage >= this->_tokens.size() || this->_tokens[this->_carriage].type() == token_type::eof;;
	}

	// Возвращает текущий токен
	const basic_register_machine::token& basic_register_machine::basic_parser::preview() const {
		return this->_tokens[this->_carriage];
	}

	// Парсинг произвольной инструкции
	instruction_ptr basic_register_machine::basic_parser::parse_instruction(basic_register_machine& rm) {
		if (this->eof())
			throw std::runtime_error("Empty instruction");

		if (this->preview().type() == token_type::keyword_stop) {
			return parse_stop_instruction(rm);
		}
		else if (this->preview().type() == token_type::keyword_if) {
			return parse_condition_instruction(rm);
		}
		else if (this->preview().type() == token_type::variable) {
			return parse_copy_assignment_instruction(rm);
		}
		else
			throw std::runtime_error("Unknown instruction start");
	}

	// Парсинг остановочной инструкции
	instruction_ptr basic_register_machine::basic_parser::parse_stop_instruction(basic_register_machine& rm) {
		if (this->is_type_match(token_type::keyword_stop)) {
			++this->_carriage;
			return std::make_unique<stop_instruction>(rm);
		}

		throw std::runtime_error("Invalid stop instruction");
	}

	// Парсинг условной инструкции
	instruction_ptr basic_register_machine::basic_parser::parse_condition_instruction(basic_register_machine& rm) {
		// Формат: if <reg> == <value> then goto <num> else goto <num>
		++this->_carriage;

		// Обработка регистра
		auto register_token = this->preview();
		if (register_token.type() != token_type::variable)
			throw std::runtime_error("Expected register after if");

		++this->_carriage;

		// Обработка оператора сравнения на равенство
		if (!this->is_type_match(token_type::operator_equal))
			throw std::runtime_error("Expected '" + EQUAL + "' after register");

		++this->_carriage;

		// Обработка сравниваемого значения
		if (!this->is_type_match(token_type::literal) || this->preview().text() != "0")
			throw std::runtime_error("Expected number 0 after '" + EQUAL + "'");

		++this->_carriage;

		// Обработка ключевого слова THEN
		if (!this->is_type_match(token_type::keyword_then))
			throw std::runtime_error("Expected '" + THEN + "'");

		++this->_carriage;

		// Обработка ключевого слова GOTO
		if (!this->is_type_match(token_type::keyword_goto))
			throw std::runtime_error("Expected '" + GOTO + "' after '" + THEN + "'");

		++this->_carriage;

		// Обработка GOTO1
		auto goto_true_token = this->preview();
		if (!this->is_type_match(token_type::literal))
			throw std::runtime_error("Expected number after '" + GOTO + "'");

		++this->_carriage;

		// Обработка ELSE
		if (!this->is_type_match(token_type::keyword_else))
			throw std::runtime_error("Expected 'else'");

		++this->_carriage;

		// Обработка ключевого слова GOTO
		if (!this->is_type_match(token_type::keyword_goto))
			throw std::runtime_error("Expected '" + GOTO + "' after '" + ELSE + "'");

		++this->_carriage;

		// Обработка GOTO2
		auto goto_false_token = this->preview();
		if (!this->is_type_match(token_type::literal))
			throw std::runtime_error("Expected number after '" + GOTO + "'");

		return std::make_unique<condition_instruction>(rm,
			register_token.text(),
			std::stoul(goto_true_token.text()),
			std::stoul(goto_false_token.text()));
	}

	// Парсинг инструкции копирующего присваивания
	instruction_ptr basic_register_machine::basic_parser::parse_copy_assignment_instruction(basic_register_machine& rm) {
		auto target_token = this->preview();

		++this->_carriage;

		// Обработка целевого регистра
		if (target_token.type() != token_type::variable)
			throw std::runtime_error("Expected register at start of copy assignment");

		// Обработка оператора копирующего присваивания
		if (!this->is_type_match(token_type::operator_copy_assignment))
			throw std::runtime_error("Expected '"s + COPY + "' after target register"s);

		++this->_carriage;

		// Обработка левого операнда
		auto left_operand_token = this->preview();

		if (left_operand_token.type() != token_type::variable && left_operand_token.type() != token_type::literal)
			throw std::runtime_error("Expected register after '"s + COPY + "'"s);
		if (left_operand_token.type() == token_type::variable && left_operand_token.text() != target_token.text()) // Левый операнд должен совпадать с целевым регистром
			throw std::runtime_error("Left operand must be the same register as target in addition");

		++this->_carriage;

		// Проверяем есть ли операция
		if (this->is_type_match(token_type::operator_plus)) {
			++this->_carriage;
			auto right_operand_token = this->preview();
			++this->_carriage;
			if (right_operand_token.type() != token_type::literal)
				throw std::runtime_error("Expected number after '" + PLUS "'");

			if (right_operand_token.text() != "1")
				throw std::runtime_error("Only increment by 1 is allowed");

			return std::make_unique<copy_assignment_instruction>(rm,
				target_token.text(),
				copy_assignment_instruction::operation::Add,
				left_operand_token.text(),
				right_operand_token.text());
		}
		else if (this->is_type_match(token_type::operator_minus)) {
			++this->_carriage;
			auto right_operand_token = this->preview();
			++this->_carriage;
			if (right_operand_token.type() != token_type::literal)
				throw std::runtime_error("Expected number after '" + MINUS "'");

			if (right_operand_token.text() != "1")
				throw std::runtime_error("Only decrement by 1 is allowed");

			return std::make_unique<copy_assignment_instruction>(rm,
				target_token.text(),
				copy_assignment_instruction::operation::Subtract,
				left_operand_token.text(),
				right_operand_token.text());
		}
		else {
			// Простое присваивание: x <- a, где a — положительное число
			if (left_operand_token.type() != token_type::literal)
				throw std::runtime_error("Simple assignment only allows positive integer literals");

			// Проверка, что число положительное
			if (std::stoi(left_operand_token.text()) < 0)
				throw std::runtime_error("Only positive integers allowed for assignment");

			return std::make_unique<copy_assignment_instruction>(rm,
				target_token.text(),
				copy_assignment_instruction::operation::None,
				left_operand_token.text(),
				"");
		}
	}

	// Проверка, соответсвует ли тип текущей инструкции заданному типу
	bool basic_register_machine::basic_parser::is_type_match(const token_type& type) const noexcept {
		if (!this->eof() && this->_tokens[this->_carriage].type() == type)
			return true;
		return false;
	}

	// Реализация вспомогательных методов

	// Удаление лишних пробелов слева и справа от строки
	void trim(std::string& line) {
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		if (line.empty()) return;
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
	}

	std::string_view trim(std::string_view line) {
		size_t start = line.find_first_not_of(" \t\r\n");
		if (start == std::string_view::npos) return {};
		size_t end = line.find_last_not_of(" \t\r\n");
		return line.substr(start, end - start + 1);
	}

	// Проверка, что в строке записан регистр
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


	// Получает целочисленное значение из строки, в которой может быть описани литерал или регистр
	int get_value(const basic_register_machine& brm, const std::string& line) {
		if (std::all_of(line.begin(), line.end(), ::isdigit))
			return std::stoi(line);
		if (is_register(line))
			return brm._registers.at(line);
		throw std::runtime_error("");
	}

	// Удаляет комменатрии в строке
	void remove_comment(std::string& line) {
		auto comment_position = line.find(COMMENT);
		if (comment_position != std::string::npos)
			line.erase(comment_position);
		trim(line);
	}

	// Реализация инструкций

	// Конструктор
	instruction::instruction(basic_register_machine& rm) noexcept: _rm(rm) {}

	// Конструктор
	copy_assignment_instruction::copy_assignment_instruction(basic_register_machine& rm, const std::string& target_register, const operation& operation, const std::string& left_operand, const std::string& right_operand) noexcept:
		instruction(rm), _target_register(target_register), _operation(operation), _left_operand(left_operand), _right_operand(right_operand) {}

	// Выполнение инструкции копирующего присваивания
	void copy_assignment_instruction::execute() {

		++this->_rm._carriage;

		try {
			int value{ 0 };
			switch (this->_operation) {
			case operation::None:
				value = get_value(this->_rm, _left_operand);
				break;
			case operation::Add:
				value = get_value(this->_rm, _left_operand) + get_value(this->_rm, _right_operand);
				break;
			case operation::Subtract:
				value = get_value(this->_rm, _left_operand) - get_value(this->_rm, _right_operand);
				if (value < 0)
					value = 0;
				break;
			}
			_rm._registers[_target_register] = value;
		}
		catch (...) {}
	}

	// Конструктор
	condition_instruction::condition_instruction(basic_register_machine& rm, const std::string& compared_register, size_t goto_true, size_t goto_false) noexcept:
		instruction(rm), _compared_register(compared_register), _goto_true(goto_true), _goto_false(goto_false) {}
	// Выполнение условной инструкции
	void condition_instruction::execute() noexcept {
		if (this->_rm._registers[this->_compared_register] == 0) this->_rm._carriage = this->_goto_true;
		else this->_rm._carriage = this->_goto_false;
	}
	// Конструктор
	stop_instruction::stop_instruction(basic_register_machine& rm) noexcept : instruction(rm) {}
	// Выполнение остановочной инструкции
	void stop_instruction::execute() noexcept {
		this->_rm._is_stopped = true;
	}

	// Конструктор
	extended_condition_instruction::extended_condition_instruction(basic_register_machine& rm, const std::string& compared_register, size_t compared_value, size_t goto_true, size_t goto_false) noexcept : _compared_value(compared_value), condition_instruction(rm, compared_register, goto_true, goto_false) {}
	// Выполнение расширенной условной инструкции
	void extended_condition_instruction::execute() noexcept {
		if (this->_rm._registers[this->_compared_register] == _compared_value) this->_rm._carriage = this->_goto_true;
		else this->_rm._carriage = this->_goto_false;
	}

	// Конструктор
	goto_instruction::goto_instruction(basic_register_machine& rm, size_t mark) noexcept : instruction(rm), _mark(mark) {}
	// Выполнение инструкции передвижения
	void goto_instruction::execute() noexcept {
		this->_rm._carriage = this->_mark;
	}

	//Конструктор
	move_assignment_instruction::move_assignment_instruction(basic_register_machine& rm, const std::string& to_register, const std::string& from_register) noexcept : instruction(rm), _to_register(to_register), _from_register(from_register) {}
	// Выполнение инструкции перемещения
	void move_assignment_instruction::execute() {
		++this->_rm._carriage;
		this->_rm._registers[this->_to_register] = this->_rm._registers[this->_from_register];
		this->_rm._registers[this->_from_register] = 0;
		return;
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
		this->_is_verbose = false;
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
			std::cout << x << ": " << this->_registers.at(x) << separator;
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
			remove_comment(line);
			this->parse_input_registers(line);
		}

		// Последующие строки, за исключением последней, содержат метки
		size_t expected_number{ 0 }; // Ожидаемый номер метки
		while (std::getline(ifs, line)) {

			remove_comment(line);
			if (line.empty()) continue;

			auto separator_position = line.find(SEPARATOR);
			if (separator_position == std::string::npos)
				break;

			std::string number = line.substr(0, separator_position);
			std::string instruction = line.substr(separator_position + SEPARATOR.length());

			trim(instruction);
			trim(number);

			if (number.empty() || std::stoi(number) != expected_number)
				throw std::invalid_argument("Filename: " + this->_filename + ", the instructions are not written in sequence");
			try {
				basic_lexer lexer(instruction);
				auto tokens = lexer.tokenize();
				basic_parser parser(tokens);
				auto instr_ptr = parser.parse_instruction(*this);
				this->_instructions.push_back(std::move(instr_ptr));
			}
			catch (const std::exception& ex) {
				throw std::runtime_error("Filename: " + this->_filename + ", invalid instruction at line " + std::to_string(expected_number) + ": " + ex.what());
			}

			++expected_number;
		}

		// В последней строке описываются выходные регистры
		this->parse_output_registers(line);

		// Проверка, что после выходных аргументов ничего нет
		while (std::getline(ifs, line)) {
			remove_comment(line);
			if (!line.empty())
				throw std::invalid_argument("Filename: " + this->_filename + ", unexpected data after output registers");
		}
	}

	// Выполнение всех инструкций
	void basic_register_machine::execute_all_instructions() {
		while (!this->_is_stopped) {
			if (this->_carriage >= this->_instructions.size())
				throw std::runtime_error("The register machine is stuck in a loop");
			const auto& current_instruction = this->_instructions[this->_carriage];

			// Вывод текущего состояния РМ
			if (this->_is_verbose) {
				this->println_all_registers();
				std::cout << this->_carriage << ": " << "..." << std::endl;
			}

			current_instruction->execute();
		}
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
		while (iss >> variable) {
			this->_input_registers.push_back(variable);
			this->_registers.try_emplace(variable, 0);
		}
	}

	// Парсинг выходных регистров
	void basic_register_machine::parse_output_registers(const std::string& line) {
		if (!this->is_valid_output_registers_line(line))
			throw std::runtime_error("Filename: " + this->_filename + ", the line does not contain output registers");

		std::string variable;
		std::istringstream iss(line);
		while (iss >> variable) {
			this->_output_registers.push_back(variable);
			this->_registers.try_emplace(variable, 0);
		}
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
				auto is_verbose = this->_is_verbose;

				// Обновление состояния РМ перед новым запуском
				this->reset();
				this->_filename = file;
				this->_is_verbose = is_verbose;
				this->load_all_instructions();

				if (results.size() > 0) { // Если есть сохранённые результаты, передаем их во входные регистры
					if (results.size() < this->_input_registers.size()) // Проверка на соотношение входов и выходов связанных РМ
						throw std::runtime_error("Not enough input values for arguments");

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

				if (this->_is_verbose)
					std::cout << this->_filename << std::endl;

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


		while (std::getline(ifs, line)) {// Команда композиции не является инструкцией, поэтому пропускаем её
			remove_comment(line);

			if (this->is_valid_input_registers_line(line)) break;
			if (!this->is_valid_composition_command(line))
				throw std::runtime_error("The call instruction is error");
		}


		// Обработка входных регистров
		this->parse_input_registers(line);

		// Последующие строки, за исключением последней, содержат метки

		size_t expected_number{ 0 };
		while (std::getline(ifs, line)) {
			remove_comment(line);
			if (line.empty()) continue;

			auto separator_position = line.find(SEPARATOR);
			if (separator_position == std::string::npos)
				break;

			std::string number = line.substr(0, separator_position);
			std::string instruction = line.substr(separator_position + SEPARATOR.length());

			trim(instruction);
			trim(number);

			if (number.empty() || std::stoi(number) != expected_number)
				throw std::invalid_argument("Filename: " + this->_filename + ", the instructions are not written in sequence");

			try {
				extended_lexer lexer(instruction);
				auto tokens = lexer.tokenize();
				extended_parser parser(tokens);
				auto instr_ptr = parser.parse_instruction(*this);
				this->_instructions.push_back(std::move(instr_ptr));
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Filename: " + this->_filename + ", invalid instruction at line " + std::to_string(expected_number) + ": " + e.what());
			}

			++expected_number;
		}

		// В последней строке описываются выходные регистры
		this->parse_output_registers(line);

		// Проверка, что после выходных аргументов ничего нет
		while (std::getline(ifs, line)) {
			remove_comment(line);
			if (!line.empty())
				throw std::invalid_argument("Filename: " + this->_filename + ", unexpected data after output registers");
		}
	}

	// Выполнение всех команд
	void extended_register_machine::execute_all_instructions() {

		while (!this->_is_stopped) {
			const auto& current_instruction = this->_instructions[this->_carriage];

			// Вывод текущего состояния РМ
			if (this->_is_verbose) {
				this->println_all_registers(" ");
				std::cout << this->_carriage << ": " << "..." << std::endl;
			}

			this->_instructions[this->_carriage]->execute();
		}
	}

	// Проверка корректности формата команды композиции
	bool extended_register_machine::is_valid_composition_command(const std::string& command) const noexcept {
		std::string pattern{ R"(^\s*)"s + COMPOSITION + R"(\s+([\w.\-]+)\s*$)" };
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
			remove_comment(line);

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