#include "register_machine.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <optional>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace IMD {

	// Helper methods

	// Removes leading and trailing whitespace characters from the given string in-place
	void trim(std::string& line) noexcept {
		line.erase(0, line.find_first_not_of(" \t\r\n"));
		if (line.empty()) return;
		line.erase(line.find_last_not_of(" \t\r\n") + 1);
	}

	// Returns a new string_view with leading and trailing whitespace removed from the input string_view
	std::string_view trim(std::string_view line) noexcept {
		size_t start = line.find_first_not_of(" \t\r\n");
		if (start == std::string_view::npos) return {};
		size_t end = line.find_last_not_of(" \t\r\n");

		return line.substr(start, end - start + 1);
	}

	// Checks if the given string represents a valid register identifier
	bool is_register(const std::string& line) noexcept {
		if (line.empty())
			return false;

		if (!std::isalpha(*line.begin()))
			return false; // The first character of the register must be a letter

		for (size_t i{ 1 }; i < line.size(); ++i) // The remaining case characters can be letters or numbers
			if (!std::isalnum(line[i]))
				return false;

		return true;
	}

	// Checks if the given string represents a non-negative integer literal
	bool is_non_negative_literal(const std::string& line) noexcept {
		if (line.empty())
			return false;

		for (size_t i{ 0 }; i < line.size(); ++i) // All characters must be numbers from 0 to 9
			if (!std::isdigit(line[i]))
				return false;

		return true;
	}
	// Checks if the given string represents a negative integer literal
	bool is_negative_literal(const std::string& line) noexcept {
		if (line.size() < 2) // минимум '-' и одна цифра
			return false;
		if (line[0] != '-')
			return false;

		for (size_t i = 1; i < line.size(); ++i)
			if (!std::isdigit(static_cast<unsigned char>(line[i])))
				return false;

		return true;
	}

	// Checks if the given string is a valid filename with an extension
	bool is_filename_with_extension(const std::string& line) noexcept {
		std::filesystem::path p(line);
		std::string filename = p.filename().string();

		size_t dot_position = filename.find_last_of('.');

		if (dot_position == std::string::npos || dot_position == 0 || dot_position == line.size() - 1)
			return false;

		// Check file name (from start to dot)
		for (size_t i{ 0 }; i < dot_position; ++i)
			if (std::isspace(static_cast<unsigned char>(line[i])) || std::iscntrl(static_cast<unsigned char>(line[i])))
				return false;

		// Check file extension (from dot to end)
		for (size_t i{ dot_position + 1 }; i < line.size(); ++i)
			if (std::isspace(static_cast<unsigned char>(line[i])) || std::iscntrl(static_cast<unsigned char>(line[i])))
				return false;

		return true;
	}

	// Removes comment from the given string by erasing everything after the comment marker
	void remove_comment(std::string& line) noexcept {
		auto comment_position = line.find(COMMENT);
		if (comment_position != std::string::npos)
			line.erase(comment_position);
		trim(line);
	}

	// Returns an integer value parsed from a string, which may contain a literal or a register
	int get_value(const basic_register_machine& brm, const std::string& line) {
		if (std::all_of(line.begin(), line.end(), ::isdigit))
			return std::stoi(line);

		if (is_register(line))
			return brm._registers.at(line);

		throw std::runtime_error("The string does not contain a literal or case");
	}

	// Implementation of a token

	// Constructor
	basic_register_machine::token::token(const token_type& type, const std::string& text) noexcept : _type(type), _text(text) {}

	// Returns the token type
	const basic_register_machine::token_type& basic_register_machine::token::type() const noexcept {
		return this->_type;
	}
	// Returns the token content
	const std::string& basic_register_machine::token::text() const noexcept {
		return this->_text;
	}

	// Implementation of the basic register machine lexer

	// Constructor
	basic_register_machine::basic_lexer::basic_lexer(const std::string& line) noexcept : _line(line), _carriage(0) {}

	// Returns a vector of tokens
	std::vector<basic_register_machine::token> basic_register_machine::basic_lexer::tokenize() {
		std::vector<token> tokens{};
		while (true) {
			this->skip_spaces();
			if (this->eof()) break;
			tokens.push_back(*(this->next_token()));

		}

		return tokens;
	}

	// Returns the next token
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

		for (const auto& [keyword, type] : keyword_type_pairs) {
			if (this->_line.size() - this->_carriage >= keyword.size() &&
				this->_line.compare(this->_carriage, keyword.size(), keyword) == 0) {

				size_t next_symbol_position = this->_carriage + keyword.size();
				if (next_symbol_position < this->_line.size() ||
					!std::isspace(static_cast<unsigned char>(this->_line[next_symbol_position]))) {
					this->_carriage += keyword.size();
					return token{ type, keyword };
				}
			}
		}

		size_t start = { this->_carriage };
		while (!this->eof() && !std::isspace(this->_line[this->_carriage]))
			++this->_carriage;
		std::string text = std::string(this->_line.substr(start, this->_carriage - start));

		if (text.empty()) return std::nullopt;

		if (is_non_negative_literal(text))
			return token{ token_type::literal, text };

		if (is_register(text))
			return token{ token_type::variable, text };

		if (is_negative_literal(text))
			throw std::runtime_error("Expected a non-negative integer literal at position: " + std::to_string(this->_carriage));

		throw std::runtime_error("Unknown token at position: " + std::to_string(this->_carriage));
	}

	// Skip spaces
	void basic_register_machine::basic_lexer::skip_spaces() noexcept {
		while (!this->eof() && std::isspace(this->_line[this->_carriage]))
			++this->_carriage;
	}

	// Checking for end of line
	bool basic_register_machine::basic_lexer::eof() const noexcept {
		return this->_carriage >= this->_line.size();
	}

	// Implementation of the extended register machine lexer

	extended_register_machine::extended_lexer::extended_lexer(const std::string& line) noexcept : basic_lexer(line) {}

	// Returns the next token
	std::optional<basic_register_machine::token> extended_register_machine::extended_lexer::next_token() {
		if (this->eof())
			return std::nullopt;

		static const std::vector<std::pair<std::string, token_type>> keyword_type_pairs = {
			{COMPOSITION, token_type::keyword_composition},
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

				size_t next_symbol_position = this->_carriage + keyword.size();
				if (next_symbol_position < this->_line.size() ||
					!std::isspace(static_cast<unsigned char>(this->_line[next_symbol_position]))) {
					this->_carriage += keyword.size();
					return token{ type, keyword };
				}
			}
		}

		size_t start = { this->_carriage };
		while (!this->eof() && !std::isspace(this->_line[this->_carriage]))
			++this->_carriage;
		std::string text = std::string(this->_line.substr(start, this->_carriage - start));

		if (text.empty()) return std::nullopt;

		if (is_filename_with_extension(text))
			return token{ token_type::file, text };

		if (is_non_negative_literal(text))
			return token{ token_type::literal, text };

		if (is_register(text))
			return token{ token_type::variable, text };

		if (is_negative_literal(text))
			throw std::runtime_error("Expected a non-negative integer literal at position: " + std::to_string(this->_carriage));

		throw std::runtime_error("Unknown token at position: " + std::to_string(this->_carriage));
	}

	// Implementation of the extended register machine parser

	// Constructor
	extended_register_machine::extended_parser::extended_parser(const std::vector<token>& tokens) noexcept : basic_parser(tokens) {};

	// Returns a unique pointer to the instruction
	std::unique_ptr<basic_register_machine::instruction> extended_register_machine::extended_parser::make_instruction() {
		if (this->eof())
			throw std::runtime_error("Empty instruction");

		auto current_type = this->preview().type();

		if (current_type == token_type::unknown)
			throw std::runtime_error("Unknown instruction found");

		if (current_type == token_type::keyword_composition)
			return this->make_composition_command();

		if (current_type == token_type::keyword_stop)
			return this->make_stop_instruction();

		if (current_type == token_type::keyword_if)
			return this->make_condition_instruction();

		if (current_type == token_type::keyword_goto)
			return this->make_goto_assignment_instruction();


		if (current_type == token_type::variable) {
			auto next_token = this->_tokens.at(this->_carriage + 1);
			if (next_token.type() == token_type::operator_copy_assignment)
				return this->make_copy_assignment_instruction();
			else if (next_token.type() == token_type::operator_move_assignment) {
				return this->make_move_assignment_instruction();
			}
			else
				throw std::runtime_error("Expected assignment operator after register");
		}

		return NULL;
	}

	std::unique_ptr<basic_register_machine::instruction> extended_register_machine::extended_parser::make_composition_command() {
		if (this->_tokens.size() > 2)
			throw std::runtime_error("An unexpected part of the COMPOSITION command");

		++this->_carriage;

		auto include_filename_token = this->preview();
		if (include_filename_token.type() == token_type::file) {
			auto include_filename = include_filename_token.text();
			++this->_carriage;
			return std::make_unique<composition_instruction>(include_filename_token.text());
		}
		throw std::runtime_error("");
	}

	std::unique_ptr<basic_register_machine::instruction> extended_register_machine::extended_parser::make_goto_assignment_instruction() {
		++this->_carriage;

		auto number_token = this->preview();

		if (number_token.type() == token_type::literal) {
			size_t mark = std::stoul(number_token.text());

			++this->_carriage;

			return std::make_unique<goto_instruction>(mark);
		}
		throw std::runtime_error("Expected number after '" + GOTO + "'");

	}

	std::unique_ptr<basic_register_machine::instruction> extended_register_machine::extended_parser::make_move_assignment_instruction() {
		auto to_register_token = this->preview();

		++this->_carriage;

		// Processing the target register
		if (to_register_token.type() != token_type::variable)
			throw std::runtime_error("Expected register at start of move assignment"s);

		// Handling the copy assignment operator
		if (!this->is_type_match(token_type::operator_move_assignment))
			throw std::runtime_error("Expected '"s + MOVE + "' after target register"s);

		++this->_carriage;

		// Processing the source register
		auto from_register_token = this->preview();

		if (from_register_token.type() != token_type::variable)
			throw std::runtime_error("Expected register after '"s + MOVE + "'"s);

		return std::make_unique<move_assignment_instruction>(
			to_register_token.text(),
			from_register_token.text());
	}

	std::unique_ptr<basic_register_machine::instruction> extended_register_machine::extended_parser::make_copy_assignment_instruction() {
		auto target_token = this->preview();

		++this->_carriage;

		// Processing the target register
		if (target_token.type() != token_type::variable)
			throw std::runtime_error("Expected register at start of copy assignment");

		// Handling the copy assignment operator
		if (!this->is_type_match(token_type::operator_copy_assignment))
			throw std::runtime_error("Expected '"s + COPY + "' after target register"s);

		++this->_carriage;

		// Left operand processing
		auto left_operand_token = this->preview();

		if (left_operand_token.type() != token_type::variable && left_operand_token.type() != token_type::literal)
			throw std::runtime_error("Expected register or literal after '"s + COPY + "'"s);

		++this->_carriage;

		// Processing an arithmetic operation
		if (this->is_type_match(token_type::operator_plus)) { // Found a plus
			++this->_carriage;
			auto right_operand_token = this->preview();
			++this->_carriage;
			if (right_operand_token.type() != token_type::literal && right_operand_token.type() != token_type::variable)
				throw std::runtime_error("Expected number or literal after '" + PLUS "'"s);

			if (right_operand_token.type() == token_type::literal && std::stoi(right_operand_token.text()) < 0)
				throw std::runtime_error("Only positive integers allowed for subtraction"s);

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				basic_register_machine::operation::plus,
				left_operand_token.text(),
				right_operand_token.text());
		}

		if (this->is_type_match(token_type::operator_minus)) { // Found a minus
			++this->_carriage;

			auto right_operand_token = this->preview();

			++this->_carriage;

			if (right_operand_token.type() != token_type::literal && right_operand_token.type() != token_type::variable) // TODO: странно обрабатывает отрицательные числа
				throw std::runtime_error("Expected number or literal after '" + MINUS "'"s);

			if (right_operand_token.type() == token_type::literal && std::stoi(right_operand_token.text()) < 0)
				throw std::runtime_error("Only positive integers allowed for subtraction"s);

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				basic_register_machine::operation::minus,
				left_operand_token.text(),
				right_operand_token.text());
		}

		if (left_operand_token.type() == token_type::literal) {
			if (std::stoi(left_operand_token.text()) < 0)
				throw std::runtime_error("Only positive integers allowed for copy assignment"s);

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				basic_register_machine::operation::none,
				left_operand_token.text(),
				"");
		}
		else if (left_operand_token.type() == token_type::variable) {
			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				basic_register_machine::operation::none,
				left_operand_token.text(),
				"");
		}
		else throw std::runtime_error("Expected number or literal in '" + COPY + "'"s);
	}

	// Implementation of the basic register machine parser

	// Constructor
	basic_register_machine::basic_parser::basic_parser(const std::vector<token>& tokens) noexcept : _tokens(tokens), _carriage(0) {}

	// Checks for the end of a vector
	bool basic_register_machine::basic_parser::eof() const noexcept {
		return this->_carriage >= this->_tokens.size();
	}

	// Returns the current token
	const basic_register_machine::token& basic_register_machine::basic_parser::preview() const {
		return this->_tokens[this->_carriage];
	}

	// Returns a unique pointer to the instruction
	std::unique_ptr<basic_register_machine::instruction> basic_register_machine::basic_parser::make_instruction() {
		if (this->eof())
			throw std::runtime_error("Empty instruction");

		if (this->preview().type() == token_type::keyword_stop)
			return this->make_stop_instruction();

		if (this->preview().type() == token_type::keyword_if)
			return this->make_condition_instruction();

		if (this->preview().type() == token_type::variable)
			return this->make_copy_assignment_instruction();

		throw std::runtime_error("Unknown instruction start");
	}

	// Returns a unique pointer to the stop instruction
	std::unique_ptr<basic_register_machine::instruction> basic_register_machine::basic_parser::make_stop_instruction() {
		if (this->is_type_match(token_type::keyword_stop)) {
			++this->_carriage;
			return std::make_unique<stop_instruction>();
		}

		throw std::runtime_error("Invalid stop instruction");
	}

	// Returns a unique pointer to the condition instruction
	std::unique_ptr<basic_register_machine::instruction> basic_register_machine::basic_parser::make_condition_instruction() {
		++this->_carriage;

		// Processing the compared register
		auto register_token = this->preview();
		if (register_token.type() != token_type::variable)
			throw std::runtime_error("Expected register after if");

		++this->_carriage;

		// Handling the equality comparison operator
		if (!this->is_type_match(token_type::operator_equal))
			throw std::runtime_error("Expected '" + EQUAL + "' after register");

		++this->_carriage;

		// Processing the compared value
		if (!this->is_type_match(token_type::literal) || this->preview().text() != "0")
			throw std::runtime_error("Expected number 0 after '" + EQUAL + "'");

		++this->_carriage;

		// Processing the THEN keyword
		if (!this->is_type_match(token_type::keyword_then))
			throw std::runtime_error("Expected '" + THEN + "'");

		++this->_carriage;

		// Processing the GOTO keyword
		if (!this->is_type_match(token_type::keyword_goto))
			throw std::runtime_error("Expected '" + GOTO + "' after '" + THEN + "'");

		++this->_carriage;

		// Processing the TRUE_MARKER keyword
		auto goto_true_token = this->preview();
		if (!this->is_type_match(token_type::literal))
			throw std::runtime_error("Expected number after '" + GOTO + "'");

		++this->_carriage;

		// Processing the ELSE keyword
		if (!this->is_type_match(token_type::keyword_else))
			throw std::runtime_error("Expected 'else'");

		++this->_carriage;

		// Processing the GOTO keyword
		if (!this->is_type_match(token_type::keyword_goto))
			throw std::runtime_error("Expected '" + GOTO + "' after '" + ELSE + "'");

		++this->_carriage;

		// Processing the FALSE_MARKER keyword
		auto goto_false_token = this->preview();
		if (!this->is_type_match(token_type::literal))
			throw std::runtime_error("Expected number after '" + GOTO + "'");

		return std::make_unique<condition_instruction>(
			register_token.text(),
			std::stoul(goto_true_token.text()),
			std::stoul(goto_false_token.text()));
	}

	// Returns a unique pointer to the copy assignment instruction
	std::unique_ptr<basic_register_machine::instruction> basic_register_machine::basic_parser::make_copy_assignment_instruction() {
		auto target_token = this->preview();

		++this->_carriage;

		// Processing the target register
		if (target_token.type() != token_type::variable)
			throw std::runtime_error("Expected register at start of copy assignment");

		// Handling the Copy Assignment Operator
		if (!this->is_type_match(token_type::operator_copy_assignment))
			throw std::runtime_error("Expected '"s + COPY + "' after target register"s);

		++this->_carriage;

		// Left operand processing
		auto left_operand_token = this->preview();

		if (left_operand_token.type() != token_type::variable && left_operand_token.type() != token_type::literal)
			throw std::runtime_error("Expected register after '"s + COPY + "'"s);
		if (left_operand_token.type() == token_type::variable && left_operand_token.text() != target_token.text()) // Левый операнд должен совпадать с целевым регистром
			throw std::runtime_error("Left operand must be the same register as target in addition");

		++this->_carriage;

		// Let's check if there is an operation
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
				basic_register_machine::operation::plus,
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
				basic_register_machine::operation::minus,
				left_operand_token.text(),
				right_operand_token.text());
		}
		else {
			// Simple assignment: x <- a, where a is a non-negative integer
			if (left_operand_token.type() != token_type::literal)
				throw std::runtime_error("Simple assignment only allows positive integer literals");

			// Checking if a number is negative
			if (std::stoi(left_operand_token.text()) < 0)
				throw std::runtime_error("Only positive integers allowed for assignment");

			return std::make_unique<copy_assignment_instruction>(
				target_token.text(),
				basic_register_machine::operation::none,
				left_operand_token.text(),
				"");
		}
	}

	// Check if the type of the current instruction matches the given type
	bool basic_register_machine::basic_parser::is_type_match(const token_type& type) const noexcept {
		if (!this->eof() && this->_tokens[this->_carriage].type() == type)
			return true;
		return false;
	}

	// Implementation of instructions

	// Constructor
	basic_register_machine::instruction::instruction() noexcept {}

	// Returns a normalized description of the instruction
	const std::string& basic_register_machine::instruction::description() const noexcept {
		return this->_description;
	}

	// Constructor
	basic_register_machine::copy_assignment_instruction::copy_assignment_instruction(const std::string& target_register, const operation& operation, const std::string& left_operand, const std::string& right_operand) noexcept :
		instruction(), _target_register(target_register), _operation(operation), _left_operand(left_operand), _right_operand(right_operand) {
		this->_description = this->_target_register + " " + COPY + " " + this->_left_operand + " " + (this->_operation == operation::plus ? PLUS : this->_operation == operation::minus ? MINUS : " ") + " " + this->_right_operand;
	}

	// Executing a copy assignment instruction
	void basic_register_machine::copy_assignment_instruction::execute(basic_register_machine& brm) {

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

	// Constructor
	basic_register_machine::condition_instruction::condition_instruction(const std::string& compared_register, size_t goto_true, size_t goto_false) noexcept :
		instruction(), _compared_register(compared_register), _goto_true(goto_true), _goto_false(goto_false) {
		this->_description = IF + " " + this->_compared_register + " " + EQUAL + " 0 " + THEN + " " + GOTO + " " + std::to_string(this->_goto_true) + " " + ELSE + " " + GOTO + " " + std::to_string(this->_goto_false);
	}
	// Executing a conditional instruction
	void basic_register_machine::condition_instruction::execute(basic_register_machine& brm) noexcept {
		if (brm._registers[this->_compared_register] == 0) brm._carriage = this->_goto_true;
		else brm._carriage = this->_goto_false;
	}

	// Constructor
	basic_register_machine::stop_instruction::stop_instruction() noexcept : instruction() {
		this->_description = STOP;
	}
	// Executing a stop instruction
	void basic_register_machine::stop_instruction::execute(basic_register_machine& brm) noexcept {
		brm._is_stopped = true;
	}

	// Constructor
	basic_register_machine::extended_condition_instruction::extended_condition_instruction(const std::string& compared_register, size_t compared_value, size_t goto_true, size_t goto_false) noexcept : _compared_value(compared_value), condition_instruction(compared_register, goto_true, goto_false) {
		this->_description = IF + " " + this->_compared_register + " " + EQUAL + " " + std::to_string(this->_compared_value) + " " + THEN + " " + GOTO + " " + std::to_string(this->_goto_true) + " " + ELSE + " " + GOTO + " " + std::to_string(this->_goto_false);
	}
	// Executing a extended conditional instruction
	void basic_register_machine::extended_condition_instruction::execute(basic_register_machine& brm) noexcept {
		if (brm._registers[this->_compared_register] == _compared_value) brm._carriage = this->_goto_true;
		else brm._carriage = this->_goto_false;
	}

	// Constructor
	basic_register_machine::goto_instruction::goto_instruction(size_t mark) noexcept : instruction(), _target_mark(mark) {
		this->_description = GOTO + " " + std::to_string(this->_target_mark);
	}
	// Executing a goto instruction
	void basic_register_machine::goto_instruction::execute(basic_register_machine& brm) noexcept {
		brm._carriage = this->_target_mark;
	}

	// Constructor
	extended_register_machine::composition_instruction::composition_instruction(const std::string& include_filename) noexcept : instruction(), _include_filename(include_filename) {
		this->_description = COMPOSITION + " " + this->_include_filename;
	}

	// Executing a composition instruction
	void extended_register_machine::composition_instruction::execute(basic_register_machine& brm) {
		auto erm = dynamic_cast<extended_register_machine*>(&brm);
		if (erm == NULL)
			throw std::runtime_error("erasd");

		erm->_file_stack.push({ this->_include_filename, std::nullopt });
	}

	// Constructor
	basic_register_machine::move_assignment_instruction::move_assignment_instruction(const std::string& to_register, const std::string& from_register) noexcept : instruction(), _to_register(to_register), _from_register(from_register) {
		this->_description = this->_to_register + " " + MOVE + " " + this->_from_register;
	}
	// Executing move assignment instruction
	void basic_register_machine::move_assignment_instruction::execute(basic_register_machine& brm) {
		++brm._carriage;
		brm._registers[this->_to_register] = brm._registers[this->_from_register];
		brm._registers[this->_from_register] = 0;
		return;
	}

	// Implementation of the basic register machine

	// Constructor
	basic_register_machine::basic_register_machine(const std::string& filename, bool is_verbose) noexcept : _filename(filename), _is_verbose(is_verbose), _carriage(0), _registers(), _instructions(), _output_registers(), _is_stopped(false) {}

	// Launch of RM
	void basic_register_machine::run() {
		this->load_all_instructions();

		for (const auto& x : this->_input_registers) { // Запрос ввода значения для входных регистров
			std::cout << "Введите значения для " << x << ": ";
			std::cin >> this->_registers[x];
		}

		this->execute_all_instructions();
		this->print_output_registers(" "); // Вывод выходных регистров
	}

	// Reset RM
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

	// Print input registers separated by a separator without a new line
	void basic_register_machine::print_input_registers(const std::string& separator) const noexcept {
		for (const auto& reg : this->_input_registers) std::cout << reg << ": " << this->_registers.at(reg) << separator;
	}
	// Print input registers separated by a separator and go to a new line
	void basic_register_machine::println_input_registers(const std::string& separator) const noexcept {
		basic_register_machine::print_input_registers(separator);
		std::cout << std::endl;
	}

	// Print all registers separated by a separator without a new line
	void basic_register_machine::print_all_registers(const std::string& separator) const noexcept {
		for (const auto& [x, y] : this->_registers) std::cout << x << ": " << y << separator;
	}
	// Print all registers separated by a separator and go to a new line
	void basic_register_machine::println_all_registers(const std::string& separator) const noexcept {
		basic_register_machine::print_all_registers(separator);
		std::cout << std::endl;
	}

	// Print output registers separated by a separator without a new line
	void basic_register_machine::print_output_registers(const std::string& separator) const noexcept {
		for (const auto& x : this->_output_registers)
			std::cout << x << ": " << this->_registers.at(x) << separator;
	}
	// Print output registers separated by a separator and go to a new line
	void basic_register_machine::println_output_registers(const std::string& separator) const noexcept {
		basic_register_machine::print_output_registers(separator);
		std::cout << std::endl;
	}

	// Print carriage separated by a separator without a new line
	void basic_register_machine::print_carriage() const noexcept {
		std::cout << "carriage: " << this->_carriage;
	}
	// Print carriage separated by a separator and go to a new line
	void basic_register_machine::println_carriage() const noexcept {
		basic_register_machine::print_carriage();
		std::cout << std::endl;
	}

	// Load all instuctions
	void basic_register_machine::load_all_instructions(std::streampos start_position, std::ios_base::seekdir border) {
		std::ifstream ifs(this->_filename);
		ifs.seekg(start_position, border);
		std::string line;

		if (!ifs)
			throw std::runtime_error("Filename: " + this->_filename + ". Error processing file");

		while (std::getline(ifs, line)) {
			remove_comment(line);
			if (!line.empty()) break;
		}

		this->parse_input_registers(line); // Processing input registers

		size_t expected_number{ 0 }; // Instructions must be numbered sequentially
		while (std::getline(ifs, line)) { // Processing instructions

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
				throw std::invalid_argument("Filename: " + this->_filename + ". Expected an instruction with the number " + std::to_string(expected_number));

			if (!is_non_negative_literal(number))
				throw std::invalid_argument("Filename: " + this->_filename + ". The instruction must be numbered with a non-negative integer");

			if (std::stoi(number) != expected_number)
				throw std::invalid_argument("Filename: " + this->_filename + ". Instructions must be numbered sequentially");

			try {
				basic_lexer lexer(instruction);
				auto tokens = lexer.tokenize();
				basic_parser parser(tokens);
				auto instruction_pointer = parser.make_instruction();
				this->_instructions.push_back(std::move(instruction_pointer));
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Filename: " + this->_filename + ". Invalid instruction at line " + std::to_string(expected_number) + ": " + e.what());
			}

			++expected_number;
		}

		this->parse_output_registers(line); // Processing output registers

		// There should be no extra entries after the output registers
		while (std::getline(ifs, line)) {
			remove_comment(line);
			if (!line.empty())
				throw std::invalid_argument("Filename: " + this->_filename + ". There should be no extra entries after the output registers");
		}
	}

	// Follow all instructions
	void basic_register_machine::execute_all_instructions() {
		while (!this->_is_stopped) {

			if (this->_carriage >= this->_instructions.size())
				throw std::runtime_error("Filename: " + this->_filename + ". The register machine is stuck in a loop");

			const auto& current_instruction = this->_instructions[this->_carriage];

			if (this->_is_verbose) { // Print the current state of the register machine
				this->println_all_registers();
				std::cout << this->_carriage << ": " << this->_instructions[this->_carriage]->description() << std::endl;
			}

			current_instruction->execute(*this);
		}
	}

	// Parsing input registers
	void basic_register_machine::parse_input_registers(const std::string& line) {
		std::string variable;
		std::istringstream iss(line);

		while (iss >> variable) {
			if (!is_register(variable))
				throw std::runtime_error("Filename: " + this->_filename + ". The input register string contains a non-register value");

			this->_input_registers.push_back(variable);
			this->_registers.try_emplace(variable, 0);
		}
	}

	// Parsing output registers
	void basic_register_machine::parse_output_registers(const std::string& line) {
		std::string variable;
		std::istringstream iss(line);
		while (iss >> variable) {
			if (!is_register(variable))
				throw std::runtime_error("Filename: " + this->_filename + ". The output register string contains a non-register value");

			this->_output_registers.push_back(variable);
			this->_registers.try_emplace(variable, 0);
		}
	}

	// Implementation of an extended register machine

	// Constructor
	extended_register_machine::extended_register_machine(const std::string& filename, bool is_verbose) noexcept : basic_register_machine(filename, is_verbose), _file_stack() {}

	// Launch of RM
	void extended_register_machine::run() {
		this->_include_files(_filename); // Processing all instructions of the composition from the source file

		while (!this->_file_stack.empty()) { // Traverse all included files
			auto [file, read_position] = this->_file_stack.top(); // Retrieve the top-level file and the current reading position
			this->_file_stack.pop();

			if (read_position == std::nullopt) // Processing all instructions of the composition from the top-level file
				this->_include_files(file);
			else {
				std::vector<int> results{}; // Vector of intermediate results

				for (const auto& x : this->_output_registers)
					results.push_back(this->_registers[x]);
				auto is_verbose = this->_is_verbose;

				// Updating the register machine state before a new run
				this->reset();
				this->_filename = file;
				this->_is_verbose = is_verbose;
				this->load_all_instructions(read_position.value());

				if (!results.empty()) { // If there are intermediate results, pass them into the input registers

					if (results.size() < this->_input_registers.size()) // Check the correspondence between the number of inputs and outputs of the connected register machines
						throw std::runtime_error("Filename: " + this->_filename + ". Not enough input values for arguments");

					size_t index{ 0 };
					for (const auto& x : this->_input_registers) {
						this->_registers[x] = results[index];
						++index;
					}
				}
				else { // If there are no intermediate results (first run), prompt the user to enter values for the input registers
					for (const auto& x : this->_input_registers) {
						std::cout << "Введите значения для " << x << ": ";
						std::cin >> this->_registers[x];
					}
				}

				if (this->_is_verbose)
					std::cout << this->_filename << std::endl;

				this->execute_all_instructions(); // Executing the instructions of the top-level file
			}
		}
		this->print_output_registers(" "); // After executing all files, display the values of the output registers
	}

	// Load all instructions
	void extended_register_machine::load_all_instructions(std::streampos start_position, std::ios_base::seekdir border) {
		std::ifstream ifs(this->_filename);

		if (!ifs)
			throw std::runtime_error("Filename: " + this->_filename + ". Error processing file");

		ifs.seekg(start_position, border);
		std::string line;

		while (std::getline(ifs, line)) { // Skipping blank lines between composition instructions and input registers
			remove_comment(line);
			if (!line.empty()) break;
		}

		this->parse_input_registers(line); // Processing input registers

		// Processing all instuctions
		size_t expected_number{ 0 }; // Instructions must be numbered sequentially
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
				throw std::invalid_argument("Filename: " + this->_filename + ". Expected an instruction with the number " + std::to_string(expected_number));

			if (!is_non_negative_literal(number))
				throw std::invalid_argument("Filename: " + this->_filename + ". The instruction must be numbered with a non-negative integer");

			if (std::stoi(number) != expected_number)
				throw std::invalid_argument("Filename: " + this->_filename + ". Instructions must be numbered sequentially");

			try {
				extended_lexer lexer(instruction);
				auto tokens = lexer.tokenize();
				extended_parser parser(tokens);
				auto instr_ptr = parser.make_instruction();
				this->_instructions.push_back(std::move(instr_ptr));
			}
			catch (const std::exception& e) {
				throw std::runtime_error("Filename: " + this->_filename + ". Invalid instruction at line " + std::to_string(expected_number) + ": " + e.what());
			}

			++expected_number;
		}

		// Processing output registers
		this->parse_output_registers(line);
	}

	// Follow all instuctions
	void extended_register_machine::execute_all_instructions() {

		while (!this->_is_stopped) {
			const auto& current_instruction = this->_instructions[this->_carriage];

			if (this->_carriage >= this->_instructions.size())
				throw std::runtime_error("Filename: " + this->_filename + ". The register machine is stuck in a loop");

			if (this->_is_verbose) { // Print the current state of the register machine
				this->println_all_registers(" ");
				std::cout << this->_carriage << ": " << this->_instructions[this->_carriage]->description() << std::endl;
			}

			this->_instructions[this->_carriage]->execute(*this);
		}
	}

	// Process all composition insturctions in the given file and add the included files to the stack
	void extended_register_machine::_include_files(const std::string& filename) {
		std::ifstream input_file(filename, std::ios::binary);
		if (!input_file)
			return;

		constexpr size_t READ_BUFFER_SIZE = 4096; // 4 KB - standard read block

		std::vector<std::unique_ptr<basic_register_machine::instruction>> composition_instructions{};

		bool composition_block_ended = false;

		// Determining the file size
		input_file.seekg(0, std::ios::end);
		std::streamoff file_size = input_file.tellg();

		// Buffer for accumulating characters of the line that we read from the end of the file
		std::string reversed_line_buffer;
		std::streamoff read_position{ file_size }; // Current reading position pointer

		// Reading a file from the end
		// We go through the file in blocks by READ_BUFFER_SIZE, moving backwards
		while (read_position > 0 && !composition_block_ended) {
			// We determine the size of the current block (if there is a piece smaller than READ_BUFFER_SIZE left, we read it entirely)
			size_t current_chunk_size = (read_position >= READ_BUFFER_SIZE) ? READ_BUFFER_SIZE : static_cast<size_t>(read_position);
			read_position -= current_chunk_size;
			input_file.seekg(read_position);

			// Reading a block of data into a buffer
			std::vector<char> read_buffer(current_chunk_size);
			input_file.read(read_buffer.data(), current_chunk_size);

			// We go through the block from the end to the beginning (since we read the file from the end)
			for (int i = static_cast<int>(current_chunk_size) - 1; i >= 0; --i) {
				char ch = read_buffer[i];
				if (ch == '\n') {
					// End of line encountered - reverse the accumulated buffer, since the characters were added in reverse order
					std::reverse(reversed_line_buffer.begin(), reversed_line_buffer.end());
					remove_comment(reversed_line_buffer);
					if (!reversed_line_buffer.empty()) {
						try {
							extended_lexer lexer(reversed_line_buffer);
							auto tokens = lexer.tokenize();
							extended_parser parser(tokens);
							auto instr_ptr = parser.make_instruction();
							if (instr_ptr == NULL)
								break;

							composition_instructions.push_back(std::move(instr_ptr));
						}
						catch (...) {
							composition_block_ended = true;
							break;
						}
					}
					reversed_line_buffer.clear();
				}
				else reversed_line_buffer.push_back(ch);
			}
		}

		// Process the last row if it has not been processed
		if (!reversed_line_buffer.empty() && !composition_block_ended) {
			std::reverse(reversed_line_buffer.begin(), reversed_line_buffer.end());
			remove_comment(reversed_line_buffer);
			if (!reversed_line_buffer.empty()) {
				try {
					extended_lexer lexer(reversed_line_buffer);
					auto tokens = lexer.tokenize();
					extended_parser parser(tokens);
					auto instr_ptr = parser.make_instruction();
					if (instr_ptr)
						composition_instructions.push_back(std::move(instr_ptr));
				}
				catch (...) {
					composition_block_ended = true;
				}
			}
		}


		for (auto it = composition_instructions.rbegin(); it != composition_instructions.rend(); ++it)
			(*it)->execute(*this);

		composition_instructions.clear();

		if (composition_block_ended) {
			input_file.clear(); // Resetting the flow flags
			input_file.seekg(0, std::ios::beg);

			std::string line;
			std::streampos position_after_block = input_file.tellg();

			while (std::getline(input_file, line)) {
				remove_comment(line);
				if (line.empty())
					continue;

				try {
					extended_lexer lexer(line);
					auto tokens = lexer.tokenize();
					extended_parser parser(tokens);
					auto instr_ptr = parser.make_instruction();
					if (instr_ptr == NULL)
						break;

					composition_instructions.push_back(std::move(instr_ptr));
					position_after_block = input_file.tellg();
				}
				catch (...) {
					break;
				}
			}
			this->_file_stack.push({ filename, position_after_block });
		}

		for (auto it = composition_instructions.rbegin(); it != composition_instructions.rend(); ++it)
			(*it)->execute(*this);
	}
}