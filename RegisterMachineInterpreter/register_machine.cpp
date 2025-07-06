#include "register_machine.h"
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace IMD {

	// Реализация вспомогательных методов

	// Удаление лишних пробелов слева и справа от исходной строки
	void trim(std::string& line) noexcept {
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		if (line.empty()) return;
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
	}

	// Возвращает новую строку без лишних пробелов слева и справа от исходной
	std::string_view trim(std::string_view line) noexcept {
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

	// Проверка, что в строке записан неотрицательный целый литерал
	bool is_literal(const std::string& line) noexcept {
		if (line.empty())
			return false;

		for (size_t i{ 0 }; i < line.size(); ++i) // Остальные символы регистра могут быть буквами или цифрами
			if (!std::isdigit(line[i]))
				return false;

		return true;
	}

	// Проверка, что в строке записано название файла с расширением
	bool is_filename(const std::string& line) noexcept {
		size_t dot_position = line.find('.');

		if (dot_position == std::string::npos || dot_position == 0 || dot_position == line.size() - 1)
			return false;

		// Проверка имени файла (от начала до точки)
		for (size_t i = 0; i < dot_position; ++i) {
			char c = line[i];
			if (std::isspace(static_cast<unsigned char>(c)) || std::iscntrl(static_cast<unsigned char>(c)))
				return false;
		}

		for (size_t i = dot_position + 1; i < line.size(); ++i) {
			char c = line[i];
			if (std::isspace(static_cast<unsigned char>(c)) || std::iscntrl(static_cast<unsigned char>(c)))
				return false;
		}

		return true;
	}

	// Удаление комментария в строке
	void remove_comment(std::string& line) noexcept {
		auto comment_position = line.find(COMMENT);
		if (comment_position != std::string::npos)
			line.erase(comment_position);
		trim(line);
	}

	// Получает целочисленное значение из строки, в которой может быть описани литерал или регистр
	int get_value(const basic_register_machine& brm, const std::string& line) {
		if (std::all_of(line.begin(), line.end(), ::isdigit))
			return std::stoi(line);
		if (is_register(line))
			return brm._registers.at(line);
		throw std::runtime_error("");
	}

	// Реализация токена

	// Конструктор
	basic_register_machine::token::token(const token_type& type, const std::string& text) noexcept : _type(type), _text(text) {}

	// Возвращает тип токена
	const token_type& basic_register_machine::token::type() const noexcept {
		return this->_type;
	}
	// Возвращает контекст токена
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

		static const std::vector<std::pair<std::string, token_type>> keyword_type_pairs = {
			{COPY, token_type::operator_copy_assignment},
			{EQUAL, token_type::operator_equal},
			{PLUS, token_type::operator_plus},
			{MINUS, token_type::operator_minus},
			{STOP, token_type::keyword_stop},
			{IF, token_type::keyword_if},
			{THEN, token_type::keyword_then},
			{ELSE, token_type::keyword_else},
			{GOTO, token_type::keyword_goto},
		};

		for (const auto& [keyword, type] : keyword_type_pairs) { // TODO: из-за этого места хуже обрабатываются ошибки!!!
			if (this->_line.size() - this->_carriage >= keyword.size() &&
				this->_line.compare(this->_carriage, keyword.size(), keyword) == 0) {
				this->_carriage += keyword.size();
				return token{ type, keyword };
			}
		}

		size_t start = this->_carriage;
		while (!this->eof() && !std::isspace(this->_line[this->_carriage]))
			++this->_carriage;

		std::string text = std::string(this->_line.substr(start, this->_carriage - start));

		if (is_literal(text))
			return token{ token_type::literal, text };

		else if (is_register(text))
			return token{ token_type::variable, text };

		return std::nullopt;
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

	// Возвращает следующий токен
	std::optional<basic_register_machine::token> extended_register_machine::extended_lexer::next_token() {
		if (this->eof())
			return std::nullopt;

		static const std::vector<std::pair<std::string, token_type>> keyword_type_pairs = {
			{COPY, token_type::operator_copy_assignment},
			{MOVE, token_type::operator_move_assignment},
			{EQUAL, token_type::operator_equal},
			{PLUS, token_type::operator_plus},
			{MINUS, token_type::operator_minus},
			{STOP, token_type::keyword_stop},
			{IF, token_type::keyword_if},
			{THEN, token_type::keyword_then},
			{ELSE, token_type::keyword_else},
			{GOTO, token_type::keyword_goto},
		};

		for (const auto& [keyword, type] : keyword_type_pairs) {
			if (this->_line.size() - this->_carriage >= keyword.size() &&
				this->_line.compare(this->_carriage, keyword.size(), keyword) == 0) {
				this->_carriage += keyword.size();
				return token{ type, keyword };
			}
		}

		size_t start = this->_carriage;
		while (!this->eof() && !std::isspace(this->_line[this->_carriage]))
			++this->_carriage;

		std::string text = std::string(this->_line.substr(start, this->_carriage - start));

		if (is_filename(text)) {
			return token{ token_type::filename, text };
		}
		else if (is_literal(text)) {
			return token{ token_type::literal, text };
		}
		else if (is_register(text)) {
			return token{ token_type::variable, text };
		}
		
		return std::nullopt;
	}

	// Реализация расширенного парсерса

	// Конструктор
	extended_register_machine::extended_parser::extended_parser(const std::vector<token>& tokens) noexcept : basic_parser(tokens) {};

	// Парсинг произвольной инструкции
	instruction_ptr extended_register_machine::extended_parser::parse_instruction() {
		if (this->eof())
			throw std::runtime_error("Empty instruction");

		auto current_type = this->preview().type();

		if (current_type == token_type::unknown)
			throw std::runtime_error("Unknown instruction start");


		if (current_type == token_type::keyword_stop) {
			return this->parse_stop_instruction();
		}
		else if (current_type == token_type::keyword_if) {
			return this->parse_condition_instruction();
		}
		else if (current_type == token_type::keyword_goto) {
			return this->parse_goto_assignment_instruction();
		}
		else if (current_type == token_type::variable) {

			auto next_token = this->_tokens[this->_carriage + 1];
			if (next_token.type() == token_type::operator_copy_assignment)
				return this->parse_copy_assignment_instruction();
			else if (next_token.type() == token_type::operator_move_assignment) {
				return this->parse_move_assignment_instruction();
			}
			else {
				throw std::runtime_error("Expected assignment operator after register");
			}
		}

	}

	instruction_ptr extended_register_machine::extended_parser::parse_goto_assignment_instruction() {
		++this->_carriage;

		auto number_token = this->preview();

		if (number_token.type() != token_type::literal)
			throw std::runtime_error("Expected number after '" + GOTO + "'");

		size_t mark = std::stoul(number_token.text());

		++this->_carriage;

		return std::make_unique<goto_instruction>(mark);

	}

	instruction_ptr extended_register_machine::extended_parser::parse_move_assignment_instruction() {
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

		return std::make_unique<move_assignment_instruction>(
			to_register_token.text(),
			from_register_token.text());
	}

	instruction_ptr extended_register_machine::extended_parser::parse_copy_assignment_instruction() {
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

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				copy_assignment_instruction::operation::plus,
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

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				copy_assignment_instruction::operation::minus,
				left_operand_token.text(),
				right_operand_token.text());
		}
		else {
			// Простое присваивание: x <- a, где a — положительное число
			if (left_operand_token.type() == token_type::literal) {
				if (std::stoi(left_operand_token.text()) < 0)
					throw std::runtime_error("Only positive integers allowed for assignment");

				return std::make_unique<copy_assignment_instruction>(
					target_token.text(),
					copy_assignment_instruction::operation::none,
					left_operand_token.text(),
					"");
			}
			else if (left_operand_token.type() == token_type::variable) {
				return std::make_unique<copy_assignment_instruction>(
					target_token.text(),
					copy_assignment_instruction::operation::none,
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
	instruction_ptr basic_register_machine::basic_parser::parse_instruction() {
		if (this->eof())
			throw std::runtime_error("Empty instruction");

		if (this->preview().type() == token_type::keyword_stop)
			return parse_stop_instruction();
		else if (this->preview().type() == token_type::keyword_if)
			return parse_condition_instruction();
		else if (this->preview().type() == token_type::variable)
			return parse_copy_assignment_instruction();
		else
			throw std::runtime_error("Unknown instruction start"); // Никогда не сработает, поскольку
	}

	// Парсинг остановочной инструкции
	instruction_ptr basic_register_machine::basic_parser::parse_stop_instruction() {
		if (this->is_type_match(token_type::keyword_stop)) {
			++this->_carriage;
			return std::make_unique<stop_instruction>();
		}

		throw std::runtime_error("Invalid stop instruction");
	}

	// Парсинг условной инструкции
	instruction_ptr basic_register_machine::basic_parser::parse_condition_instruction() {
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

		return std::make_unique<condition_instruction>(
			register_token.text(),
			std::stoul(goto_true_token.text()),
			std::stoul(goto_false_token.text()));
	}

	// Парсинг инструкции копирующего присваивания
	instruction_ptr basic_register_machine::basic_parser::parse_copy_assignment_instruction() {
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

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				copy_assignment_instruction::operation::plus,
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

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				copy_assignment_instruction::operation::minus,
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

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				copy_assignment_instruction::operation::none,
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

	// Реализация инструкций

	// Конструктор
	instruction::instruction() noexcept {}

	// Возвращает описание инструкции
	const std::string& instruction::description() const noexcept {
		return this->_description;
	}

	// Конструктор
	copy_assignment_instruction::copy_assignment_instruction(const std::string& target_register, const operation& operation, const std::string& left_operand, const std::string& right_operand) noexcept :
		instruction(), _target_register(target_register), _operation(operation), _left_operand(left_operand), _right_operand(right_operand) {
		this->_description = this->_target_register + " " + COPY + " " + this->_left_operand + " " + (this->_operation == operation::plus ? PLUS : this->_operation == operation::minus ? MINUS : " ") + " " + this->_right_operand;
	}

	// Выполнение инструкции копирующего присваивания
	void copy_assignment_instruction::execute(basic_register_machine& brm) {

		++brm._carriage;

		try {
			int value{ 0 };
			switch (this->_operation) {
			case operation::none:
				value = get_value(brm, _left_operand);
				break;
			case operation::plus:
				value = get_value(brm, _left_operand) + get_value(brm, _right_operand);
				break;
			case operation::minus:
				value = get_value(brm, _left_operand) - get_value(brm, _right_operand);
				if (value < 0)
					value = 0;
				break;
			}
			brm._registers[_target_register] = value;
		}
		catch (...) {}
	}

	// Конструктор
	condition_instruction::condition_instruction(const std::string& compared_register, size_t goto_true, size_t goto_false) noexcept :
		instruction(), _compared_register(compared_register), _goto_true(goto_true), _goto_false(goto_false) {
		this->_description = IF + " " + this->_compared_register + " " + EQUAL + " 0 " + THEN + " " + GOTO + " " + std::to_string(this->_goto_true) + " " + ELSE + " " + GOTO + " " + std::to_string(this->_goto_false);
	}
	// Выполнение условной инструкции
	void condition_instruction::execute(basic_register_machine& brm) noexcept {
		if (brm._registers[this->_compared_register] == 0) brm._carriage = this->_goto_true;
		else brm._carriage = this->_goto_false;
	}

	// Конструктор
	stop_instruction::stop_instruction() noexcept : instruction() {
		this->_description = STOP;
	}
	// Выполнение остановочной инструкции
	void stop_instruction::execute(basic_register_machine& brm) noexcept {
		brm._is_stopped = true;
	}

	// Конструктор
	extended_condition_instruction::extended_condition_instruction(const std::string& compared_register, size_t compared_value, size_t goto_true, size_t goto_false) noexcept : _compared_value(compared_value), condition_instruction(compared_register, goto_true, goto_false) {
		this->_description = IF + " " + this->_compared_register + " " + EQUAL + " " + std::to_string(this->_compared_value) + " " + THEN + " " + GOTO + " " + std::to_string(this->_goto_true) + " " + ELSE + " " + GOTO + " " + std::to_string(this->_goto_false);
	}
	// Выполнение расширенной условной инструкции
	void extended_condition_instruction::execute(basic_register_machine& brm) noexcept {
		if (brm._registers[this->_compared_register] == _compared_value) brm._carriage = this->_goto_true;
		else brm._carriage = this->_goto_false;
	}

	// Конструктор
	goto_instruction::goto_instruction(size_t mark) noexcept : instruction(), _mark(mark) {
		this->_description = GOTO + " " + std::to_string(this->_mark);
	}
	// Выполнение инструкции передвижения
	void goto_instruction::execute(basic_register_machine& brm) noexcept {
		brm._carriage = this->_mark;
	}

	//Конструктор
	move_assignment_instruction::move_assignment_instruction(const std::string& to_register, const std::string& from_register) noexcept : instruction(), _to_register(to_register), _from_register(from_register) {
		this->_description = this->_to_register + " " + MOVE + " " + this->_from_register;
	}
	// Выполнение инструкции перемещения
	void move_assignment_instruction::execute(basic_register_machine& brm) {
		++brm._carriage;
		brm._registers[this->_to_register] = brm._registers[this->_from_register];
		brm._registers[this->_from_register] = 0;
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
	void basic_register_machine::load_all_instructions(size_t start_position, int border) {
		std::ifstream ifs(this->_filename);
		ifs.seekg(start_position, border);
		std::string line;

		if (!ifs) // Выброс исключения, если файл не найден
			throw std::runtime_error("Error processing file");

		// Чтение входных регистров
		while (std::getline(ifs, line)) {
			remove_comment(line);
			if (line.empty()) continue;
			else {
				this->parse_input_registers(line);
				break;
			}
		}

		// Чтение строк с инструкциями
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

			if (number.empty())
				throw std::invalid_argument("Filename: " + this->_filename + ", the instructions doesn't have a number");
			if (std::stoi(number) != expected_number)
				throw std::invalid_argument("Filename: " + this->_filename + ", the instructions are not written in sequence");
			try {
				basic_lexer lexer(instruction);
				auto tokens = lexer.tokenize();
				basic_parser parser(tokens);
				auto instr_ptr = parser.parse_instruction();
				this->_instructions.push_back(std::move(instr_ptr));
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Filename: " + this->_filename + ", invalid instruction at line " + std::to_string(expected_number) + ": " + e.what());
			}

			++expected_number;
		}

		// Чтение выходных регистров
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
				std::cout << this->_carriage << ": " << this->_instructions[this->_carriage]->description() << std::endl;
			}

			current_instruction->execute(*this);
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

			if (flag == std::nullopt)
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
				this->load_all_instructions(flag.value());

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
	void extended_register_machine::load_all_instructions(size_t start_position, int border) {
		std::ifstream ifs(this->_filename);
		ifs.seekg(start_position, border);
		std::string line;

		if (!ifs) // Выброс исключения, если файл не найден
			throw std::runtime_error("Filename: " + this->_filename + ", error processing file");


		while (std::getline(ifs, line)) {// Команда композиции не является инструкцией, поэтому пропускаем её
			remove_comment(line);

			if (this->is_valid_input_registers_line(line)) break;
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
				auto instr_ptr = parser.parse_instruction();
				this->_instructions.push_back(std::move(instr_ptr));
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Filename: " + this->_filename + ", invalid instruction at line " + std::to_string(expected_number) + ": " + e.what());
			}

			++expected_number;
		}

		// В последней строке описываются выходные регистры
		this->parse_output_registers(line);
	}

	// Выполнение всех команд
	void extended_register_machine::execute_all_instructions() {

		while (!this->_is_stopped) {
			const auto& current_instruction = this->_instructions[this->_carriage];

			// Вывод текущего состояния РМ
			if (this->_is_verbose) {
				this->println_all_registers(" ");
				std::cout << this->_carriage << ": " << this->_instructions[this->_carriage]->description() << std::endl;
			}

			this->_instructions[this->_carriage]->execute(*this);
		}
	}


	// Обработка всех команд композиции в текущем файла и добавление включаемых файлов в стек
	void extended_register_machine::_include_files(const std::string& filename) {
		std::ifstream ifs(filename, std::ios::binary);
		std::string line;
		if (!ifs) return;

		// 1. Считаем файл с конца вверх, ищем все строки с COMPOSITION
		std::vector<std::string> compositions_from_end;
		bool separator_found = false;

		ifs.seekg(0, std::ios::end);
		auto filesize = ifs.tellg();

		size_t buffer_size = 4096;
		size_t pos = filesize;
		std::string line_buffer;

		while (pos > 0 && !separator_found) {
			size_t read_size = (pos >= buffer_size) ? buffer_size : pos;
			pos -= read_size;
			ifs.seekg(pos);
			std::vector<char> buf(read_size);
			ifs.read(buf.data(), read_size);

			for (int i = read_size - 1; i >= 0; --i) {
				char c = buf[i];
				if (c == '\n') {
					// Обработка строки
					std::reverse(line_buffer.begin(), line_buffer.end());
					remove_comment(line_buffer);
					if (!line_buffer.empty()) {
						if (line_buffer.find(SEPARATOR) != std::string::npos) {
							separator_found = true;
							break;
						}
						// Проверка на COMPOSITION
						if (line_buffer.find(COMPOSITION) != std::string::npos)
							compositions_from_end.push_back(line_buffer.substr(line_buffer.find(COMPOSITION) + COMPOSITION.length() + 1));
					}
					line_buffer.clear();
				}
				else {
					line_buffer.push_back(c);
				}
			}
		}

		// Обработка оставшейся строки после цикла
		if (!line_buffer.empty() && !separator_found) {
			std::reverse(line_buffer.begin(), line_buffer.end());
			remove_comment(line_buffer);
			if (!line_buffer.empty()) {
				if (line_buffer.find(SEPARATOR) != std::string::npos)
					separator_found = true;
				else if (line_buffer.find(COMPOSITION) != std::string::npos)
					compositions_from_end.push_back(line_buffer);
			}
		}

		// Добавляем файлы в стек в обратном порядке
		for (auto it = compositions_from_end.begin(); it != compositions_from_end.end(); ++it)
			_file_stack.push({ *it, std::nullopt });

		// 2. Если нашли разделитель, читаем файл с начала вниз и собираем все строки с COMPOSITION
		if (separator_found) {
			ifs.clear();
			ifs.seekg(0, std::ios::beg);

			std::vector<std::string> compositions_from_start;

			auto last_pos = ifs.tellg();

			while (std::getline(ifs, line)) {

				remove_comment(line);
				if (line.empty()) continue;

				auto composition_position = line.find(COMPOSITION);
				if (composition_position == std::string::npos)
					break;

				auto include_filename = line.substr(composition_position + COMPOSITION.length());
				trim(include_filename);

				compositions_from_start.push_back(include_filename);

				last_pos = ifs.tellg(); // Позиция **после** прочитанной строки
			}

			// Теперь last_pos указывает на позицию после последней строки с COMPOSITION
			this->_file_stack.push({ filename, last_pos });

			// Добавляем файлы в стек в обратном порядке
			for (auto it = compositions_from_start.rbegin(); it != compositions_from_start.rend(); ++it)
				_file_stack.push({ *it, std::nullopt });
		}
	}
}