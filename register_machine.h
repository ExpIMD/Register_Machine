#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

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

using namespace std::string_literals;

// Макросы ключевых слов РМ

#define SEPARATOR ":"s
#define COPY "<-"s
#define MOVE "<<-"s
#define PLUS "+"s
#define MINUS "-"s
#define STOP "stop"s
#define IF "if"s
#define THEN "then"s
#define ELSE "else"s
#define GOTO "goto"s
#define EQUAL "=="s
#define COMPOSITION "call"s
#define COMMENT "#"s

namespace IMD {

	class basic_register_machine;

	// Helper methods

	// Removes leading and trailing whitespace characters from the given string in-place
	void trim(std::string& line) noexcept;

	// Checks if the given string represents a keyword
	bool is_keyword(std::string_view line);
	// Checks if the given string represents a valid register identifier
	bool is_register(std::string_view line) noexcept;

	// Checks if the given string represents a non-negative integer literal
	bool is_non_negative_literal(std::string_view line) noexcept;

	// Checks if the given string represents a negative integer literal
	bool is_negative_literal(std::string_view line) noexcept;

	// Checks if the given string is a valid filename with an extension
	bool is_filename_with_extension(std::string_view line) noexcept;

	// Removes comment from the given string by erasing everything after the comment marker
	void remove_comment(std::string& line) noexcept;

	// Класс базовой РМ
	class basic_register_machine {
	protected:

		// Enumeration of token types
		enum class token_type {
			variable, // Register
			literal, // Non-negative integer literal
			keyword_if, // IF
			keyword_then, // THEN
			keyword_else, // ELSE
			keyword_goto, // GOTO
			operator_copy_assignment, // COPY
			operator_move_assignment, // MOVE
			operator_plus, // PLUS
			operator_minus, // MINUS
			operator_equal, // EQUAL
			keyword_stop, // STOP
			keyword_composition, // COMPOSITION
			file, // Filename with extension
			unknown, // Unknown token
		};

		// Enum of arithmetic operations
		enum class operation {
			none,
			plus,
			minus
		};

		// Instruction class
		class instruction {
		protected:
			// Normalized instruction description
			std::string _description;
		public:
			// Constructor
			explicit instruction() noexcept;

			// Destructor
			virtual ~instruction() = default;

			// Execution of instructions
			virtual void execute(basic_register_machine& brm) = 0;
			// Returns a normalized description of the instruction
			virtual const std::string& description() const noexcept;
		};

		// Copy assignment instruction class
		class copy_assignment_instruction : public instruction {
		protected:
			friend class parser;

		protected:
			// Name of the target register
			std::string _target_register;
			// Operation in expression
			operation _operation;
			// Name of the left operand of expression
			std::string _left_operand;
			// Name of the right operand of expression
			// Empty string when there is no arithmetic operation in the expression
			std::string _right_operand;

		public:
			// Constructor
			explicit copy_assignment_instruction(std::string_view target_register, const operation& operation, std::string_view left_operand, std::string_view right_operand) noexcept;

			// Destructor
			~copy_assignment_instruction() override = default;

			// Executing a copy assignment instruction
			void execute(basic_register_machine& brm) override;
		};

		// Conditional instruction class
		class condition_instruction : public instruction {
		protected:
			// Name of the compared register
			std::string _compared_register;
			// Instruction number when the condition is true
			size_t _goto_true;
			// Instruction number when the condition is false
			size_t _goto_false;

		public:
			// Constructor
			explicit condition_instruction(std::string_view compared_register, size_t goto_true, size_t goto_false) noexcept;

			// Destructor
			~condition_instruction() override = default;

			// Executing a conditional instruction
			void execute(basic_register_machine& brm) noexcept override;
		};

		// Composition instruction class
		class composition_instruction : public instruction {
			std::string _include_filename;
		public:
			// Constructor
			explicit composition_instruction(std::string_view include_filename) noexcept;

			// Destructor
			~composition_instruction() override = default;

			// Executing a composition instruction
			void execute(basic_register_machine& brm) override;
		};

		// Extended conditional instruction class
		class extended_condition_instruction : public condition_instruction {
		protected:
			// Value to be compared
			size_t _compared_value;
		public:
			// Constructor
			explicit extended_condition_instruction(std::string_view compared_register, size_t compared_value, size_t goto_true, size_t goto_false) noexcept;

			// Destructor
			~extended_condition_instruction() override = default;

			// Executing a extended conditional instruction
			void execute(basic_register_machine& brm) noexcept override;
		};

		// Movement instruction class
		class goto_instruction : public instruction {
		private:
			// Number target instruction
			size_t _target_mark;
		public:
			// Constructor
			explicit goto_instruction(size_t mark) noexcept;

			// Destructor
			~goto_instruction() override = default;

			// Executing a goto instruction
			void execute(basic_register_machine& brm) noexcept override;
		};

		// Move assignment instruction class
		class move_assignment_instruction : public instruction {
		protected:
			// Name of the target register
			std::string _to_register;
			// Name of the source register
			std::string _from_register;
		public:
			// Constructor
			explicit move_assignment_instruction(std::string_view _to_register, std::string_view _from_register) noexcept;

			// Destructor
			~move_assignment_instruction() override = default;

			// Executing move assignment instruction
			void execute(basic_register_machine& brm) override;
		};

		// Stop instruction class
		class stop_instruction : public instruction {
		public:
			// Constructor
			explicit stop_instruction() noexcept;

			// Destructor
			~stop_instruction() override = default;

			// Executing a stop instruction
			void execute(basic_register_machine& brm) noexcept override;
		};

	protected:

		// Token class
		class token {
		private:
			// Token type
			token_type _type;
			// Token content
			std::string_view _text;

		public:
			// Constructor
			explicit token(const token_type& type, std::string_view text) noexcept;

			// Destructor
			~token() noexcept = default;

			// Returns the token type
			const token_type& type() const noexcept;
			// Returns the token content
			std::string_view text() const noexcept;
		};

		// Basic register machine lexer class
		class basic_lexer {
		protected:
			// The string containing the instruction line
			std::string_view _line;
			// The position indicator (carriage) for reading through the line
			size_t _carriage;

		public:
			// Constructor
			explicit basic_lexer(std::string_view line) noexcept;

			// Destructor
			~basic_lexer() noexcept = default;

			// Returns a vector of tokens
			virtual std::vector<token> tokenize();

		protected:
			// Skip spaces
			void skip_spaces() noexcept;
			// Checking for end of line
			bool eof() const noexcept;
			// Returns the next token
			virtual std::optional<token> next_token();
		};

		// Basic register machine parser class
		class basic_parser {
		protected:
			// Token vector
			std::vector<token> _tokens;
			// Position indicator (carriage) for reading the token vector
			size_t _carriage;

		public:
			// Constructor
			explicit basic_parser(const std::vector<token>& tokens) noexcept;

			// Destructor
			~basic_parser() noexcept = default;

			// Checks for the end of a vector
			bool eof() const noexcept;
			// Returns the current token
			const token& preview() const;

			// Returns a unique pointer to the instruction
			virtual std::unique_ptr<instruction> make_instruction();

			// Returns a unique pointer to the stop instruction
			virtual std::unique_ptr<instruction> make_stop_instruction();

			// Returns a unique pointer to the condition instruction
			virtual std::unique_ptr<instruction> make_condition_instruction();

			// Returns a unique pointer to the copy assignment instruction
			virtual std::unique_ptr<instruction> make_copy_assignment_instruction();

			// Checks if the current token type matches the given type
			bool is_type_match(const token_type& type) const noexcept;

		};
	protected:
		// Flag indicating whether the RM is stopped
		bool _is_stopped;

		// Flag indicating the mode of detailed output of the RM work
		bool _is_verbose;

		// Name of the file being processed
		std::string _filename;

		// Каретка, описывающая номер текущей инструкции
		size_t _carriage;

		// Dictionary of registers
		std::unordered_map<std::string, int> _registers;

		// Vector of unique instruction pointers
		std::vector<std::unique_ptr<instruction>> _instructions;

		// Vector of output register names
		std::vector<std::string> _output_registers;

		// Vector of input register names
		std::vector<std::string> _input_registers;

	public:
		// Constructor
		basic_register_machine(const std::string& filename, bool is_verbose = false) noexcept;

		// Copy constructor
		basic_register_machine(const basic_register_machine&) = delete;
		// Assignment operator
		basic_register_machine& operator=(const basic_register_machine&) = delete;

		// Destructor
		~basic_register_machine() = default;

		// Launch of RM
		virtual void run();
		// Reset PM
		virtual void reset();

		// Print input registers separated by a separator without a new line
		void print_input_registers(const std::string& separator = " ") const noexcept;
		// Print input registers separated by a separator and go to a new line
		void println_input_registers(const std::string& separator = " ") const noexcept;

		// Print all registers separated by a separator without a new line
		void print_all_registers(const std::string& separator = " ") const noexcept;
		// Print all registers separated by a separator and go to a new line
		void println_all_registers(const std::string& separator = " ") const noexcept;

		// Print output registers separated by a separator without a new line
		void print_output_registers(const std::string& separator = " ") const noexcept;
		// Print output registers separated by a separator and go to a new line
		void println_output_registers(const std::string& separator = " ") const noexcept;

		// Print carriage separated by a separator without a new line
		void print_carriage() const noexcept;
		// Print carriage separated by a separator and go to a new line
		void println_carriage() const noexcept;

		// Returns an integer value parsed from a string, which may contain a literal or a register
		friend int get_value(const basic_register_machine& brm, const std::string& line);

	protected:
		// Load all instructions
		virtual void load_all_instructions(std::pair<std::streampos, std::streampos> barier = {0, 0}, std::ios_base::seekdir border = std::ios::beg);
		// Follow all instructions
		virtual void execute_all_instructions();

		// Parsing input registers
		void parse_input_registers(const std::string& line);
		// Parsing output registers
		void parse_output_registers(const std::string& line);
	};

	// Extended register machine class
	class extended_register_machine : public basic_register_machine {
	protected:
		friend class composition_instruction;

	protected:

		// Extended register machine lexer class
		class extended_lexer : public basic_lexer {
		public:
			// Constructor
			explicit extended_lexer(std::string_view line) noexcept;

			// Destructor
			~extended_lexer() = default;

			// Returns the next token
			std::optional<token> next_token() override;
		};

		// Extended register machine parser class
		class extended_parser : public basic_parser {
		public:
			// Constructor
			explicit extended_parser(const std::vector<token>& tokens) noexcept;

			// Destructor
			~extended_parser() = default;

			// Returns a unique pointer to the instruction
			std::unique_ptr<instruction> make_instruction() override;

			// Returns a unique pointer to the copy assignment instruction
			std::unique_ptr<instruction> make_copy_assignment_instruction() override;

			// Returns a unique pointer to the move assignment instruction
			virtual std::unique_ptr<instruction> make_move_assignment_instruction();

			// Returns a unique pointer to the goto instruction
			virtual std::unique_ptr<instruction> make_goto_assignment_instruction();

			// Returns a unique pointer to the composition instruction
			virtual std::unique_ptr<instruction> make_composition_command();
		};

	protected:
		// Stack for controlling the order of processing RM files: pair <file name, position to continue reading from>
		std::stack<std::pair<std::string, std::pair<std::optional<std::streampos>, std::optional<std::streampos>>>> _file_stack;

	public:
		// Constructor
		extended_register_machine(const std::string& filename, bool is_verbose = false) noexcept;

		// Copy constructor
		extended_register_machine(const extended_register_machine&) = delete;
		// Copy assignment operator
		extended_register_machine& operator=(const extended_register_machine&) = delete;

		// Destructor
		~extended_register_machine() = default;

		// Launch of RM
		void run() override;

	protected:
		// Load all instruction
		void load_all_instructions(std::pair<std::streampos, std::streampos> barier = {0, 0}, std::ios_base::seekdir border = std::ios::beg) override;

		// Follow all instructions
		void execute_all_instructions() override;

		// Process all composition instructions in the current file and add included files to the stack
		void _include_files(const std::string& filename);
	};
}

#endif
