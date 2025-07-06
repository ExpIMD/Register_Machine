#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

#include <ios>
#include <optional>
#include <regex>
#include <stack>
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
#define ELSE "else"s
#define THEN "then"s
#define GOTO "goto"s
#define EQUAL "=="s
#define COMPOSITION "call"s
#define COMMENT "#"s

namespace IMD {

	class basic_register_machine;

	// Вспомогательные методы

	// Удаление лишних пробелов слева и справа от строки
	void trim(std::string& line);

	// Проверка, что в строке записан регистр
	bool is_register(const std::string& line) noexcept;

	// Описание инструкций

	// Класс инструкции
	class instruction {
	protected:
		// Нормализованное описание
		std::string _description;
	public:
		// Конструктор
		instruction() noexcept;
		// Деструктор
		virtual ~instruction() = default;

		// Выполнение инструкции
		virtual void execute(basic_register_machine& brm) = 0;
		// Возвращает нормализованное описание инструкции
		virtual const std::string& description() const noexcept;
	};

	using instruction_ptr = std::unique_ptr<instruction>;

	// Класс инструкции копирующего присваивания
	class copy_assignment_instruction : public instruction {
	protected:
		friend class parser;

	public:
		enum class operation { none, plus, minus };

	protected:
		std::string _target_register;
		operation _operation; // Тип присваивания: x <- 2, x <- x + 1, x <- x - 1
		std::string _left_operand;
		std::string _right_operand; // Пустая строка при виде x <- a
	public:
		// Конструктор
		copy_assignment_instruction(const std::string& target_register, const operation& operation, const std::string& left_operand, const std::string& right_operand) noexcept;

		// Деструктор
		~copy_assignment_instruction() override = default;

		// Выполнение инструкции копирующего присваивания
		void execute(basic_register_machine& brm) override;
	};

	// Класс условной инструкции
	class condition_instruction : public instruction {
	protected:
		std::string _compared_register;
		size_t _goto_true;
		size_t _goto_false;
	public:
		// Конструктор
		condition_instruction(const std::string& compared_register, size_t goto_true, size_t goto_false) noexcept;

		// Деструктор
		~condition_instruction() override = default;

		// Выполнение условной инструкции
		void execute(basic_register_machine& brm) noexcept override;
	};

	// Класс остановочной инструкции
	class stop_instruction : public instruction {
	public:
		// Конструктор
		stop_instruction() noexcept;
		// Деструктор
		~stop_instruction() override = default;

		// Выполнение остановочной инструкции
		void execute(basic_register_machine& brm) noexcept override;
	};

	// Класс расширенной условной инструкции
	class extended_condition_instruction : public condition_instruction {
	protected:
		size_t _compared_value;
	public:
		// Конструктор
		extended_condition_instruction(const std::string& compared_register, size_t compared_value, size_t goto_true, size_t goto_false) noexcept;
		// Деструктор
		~extended_condition_instruction() override = default;

		// Выполнение расширенной условной инструкции
		void execute(basic_register_machine& brm) noexcept override;
	};

	// Класс инструкции передвижения
	class goto_instruction : public instruction {
	private:
		size_t _mark;
	public:
		// Конструктор
		goto_instruction(size_t mark) noexcept;
		// Деструктор
		~goto_instruction() override = default;

		// Выполнение инструкции передвижения
		void execute(basic_register_machine& brm) noexcept override;
	};

	// Класс инструкции перемещающего присваивания
	class move_assignment_instruction : public instruction {
	protected:
		std::string _to_register;
		std::string _from_register;
	public:
		// Конструктор
		move_assignment_instruction( const std::string& _to_register, const std::string& _from_register) noexcept;
		// Деструктор
		~move_assignment_instruction() override = default;

		// Выполнение инструкции перемещающего присваивания
		void execute(basic_register_machine& brm) override;
	};

	// Перечисление типов токена
	enum class token_type {
		variable, // Регистр (переменная)
		literal, // Литерал неотрицательного целого числа
		keyword_if, // IF
		keyword_then, // THEN
		keyword_else, // ELSE
		keyword_goto, // GOTO
		operator_copy_assignment, // <-
		operator_move_assignment, // !~
		operator_plus, // PLUS
		operator_minus, // MINUS
		operator_equal, // EQUAL
		keyword_stop, // STOP
		keyword_composition, // COMPOSITION
		eof, // END OF FILE OR LINE
		unknown
	};


	// Класс базовой РМ
	class basic_register_machine {
	protected:
		friend class instruction;
		friend class condition_instruction;
		friend class stop_instruction;
		friend class instruction;
		friend class copy_assignment_instruction;
		friend class extended_condition_instruction;
		friend class goto_instruction;
		friend class move_assignment_instruction;

	protected:
		// Класс токена
		class token {
		private:
			// Тип токена
			token_type _type;
			// Контекст токена
			std::string _text;
		public:
			// Конструктор
			token(const token_type& type, const std::string& text) noexcept;

			// Возвращает тип токена
			const token_type& type() const noexcept;
			// Возвращает контекст токена
			const std::string& text() const noexcept;

		};

		// Класс базового лексера
		class basic_lexer {
		protected:
			// Строка с инструкцией
			std::string_view _line;
			// Каретка, описывающая позицию считывания строки с инструкцией
			size_t _carriage;

		public:
			// Конструктор
			basic_lexer(std::string_view line) noexcept;

			// Возвращает вектор токенов
			virtual std::vector<token> tokenize();

		protected:
			// Пропуск пробелов
			void skip_spaces() noexcept;
			// Проверка на конец строки
			bool eof() const noexcept;

			// Возвращает следующий токен
			virtual std::optional<token> next_token();

			// Парсинг регистра или ключевого слова
			virtual token parse_register_or_keyword();

			// Парсинг литерала
			virtual token parse_literal();

			// Парсинг оператора копирующего присваивания
			virtual token parse_operator_copy_assignment();

			// Парсинг оператора сравнения на равенство
			virtual token parse_operator_equal();
		};
		class basic_parser {
		protected:
			std::vector<token> _tokens;
			size_t _carriage;
		public:
			explicit basic_parser(const std::vector<token>& tokens) noexcept;

			bool eof() const noexcept;
			// Возвращает текущий токен
			const token& preview() const;

			virtual instruction_ptr parse_instruction(basic_register_machine& rm);

			virtual instruction_ptr parse_stop_instruction(basic_register_machine& rm);

			virtual instruction_ptr parse_condition_instruction(basic_register_machine& rm);

			virtual instruction_ptr parse_copy_assignment_instruction(basic_register_machine& rm);

			bool is_type_match(const token_type& type) const noexcept;

		};
	protected:
		// Флаг, указывающий остановлена ли РМ
		bool _is_stopped;

		// Каретка, описывающая номер текущей инструкции
		size_t _carriage;

		// Словарь регистров
		std::unordered_map<std::string, int> _registers;

		// Вектор инструкций
		std::vector<instruction_ptr> _instructions;

		// Вектор выходных регистров
		std::vector<std::string> _output_registers;

		// Вектор входных регистров
		std::vector<std::string> _input_registers;

		// Имя обрабатываемого файла
		std::string _filename;

		// Флаг, указывающий режим подробного вывода работы РМ
		bool _is_verbose;

	public:
		// Конструктор
		basic_register_machine(const std::string& filename, bool is_verbose = false) noexcept;

		basic_register_machine(const basic_register_machine&) = delete;
		basic_register_machine& operator=(const basic_register_machine&) = delete;

		// Деструктор
		virtual ~basic_register_machine() = default;

		// Запуск РМ
		virtual void run();
		// Сброс РМ
		virtual void reset();

		// Печать входных регистров без перехода на новую строку
		void print_input_registers(const std::string& separator = " ") const noexcept;
		// Печать входных регистров с переходом на новую строку
		void println_input_registers(const std::string& separator = " ") const noexcept;

		// Печать всех регистров без перехода на новую строку
		void print_all_registers(const std::string& separator = " ") const noexcept;
		// Печать всех регистров с переходом на новую строку
		void println_all_registers(const std::string& separator = " ") const noexcept;

		// Печать выходных регистров без перехода на новую строку
		void print_output_registers(const std::string& separator = " ") const noexcept;
		// Печать выходных регистров с переходом на новую строку
		void println_output_registers(const std::string& separator = " ") const noexcept;

		// Печать каретки без перехода на новую строку
		void print_carriage(const std::string& separator = " ") const noexcept;
		// Печать каретки с переходом на новую строку
		void println_carriage(const std::string& separator = " ") const noexcept;

		// Получает целочисленное значение из строки, в которой может быть описани литерал или регистр
		friend int get_value(const basic_register_machine& brm, const std::string& line);

	protected:
		// Загрузка всех инструкций
		virtual void load_all_instructions(size_t start_position = 0, int border = std::ios::beg);
		// Выполнение всех инструкций
		virtual void execute_all_instructions();

		// Проверка корректности формата строки с входными регистрами
		virtual bool is_valid_input_registers_line(const std::string& instruction) const noexcept;
		// Проверка корректности формата строки с выходными регистрами
		virtual bool is_valid_output_registers_line(const std::string& instruction) const noexcept;

		// Парсинг аргументов
		void parse_input_registers(const std::string& line);
		// Парсинг результатов (выходных регистров)
		void parse_output_registers(const std::string& line);
	};

	// Класс расширенной РМ
	class extended_register_machine : public basic_register_machine {
	protected:

		class extended_lexer : public basic_lexer {
		public:
			// Конструктор
			explicit extended_lexer(const std::string& line) noexcept;

			// Парсинг оператора копирующего присваивания
			virtual token parse_operator_move_assignment();

			// 
			std::optional<token> next_token() override;
		};
		class extended_parser : public basic_parser {
		public:
			explicit extended_parser(const std::vector<token>& tokens) noexcept;

			instruction_ptr parse_instruction(basic_register_machine& rm) override;

			virtual instruction_ptr parse_move_assignment_instruction(basic_register_machine& rm);
			virtual instruction_ptr parse_goto_assignment_instruction(basic_register_machine& rm);
			instruction_ptr parse_copy_assignment_instruction(basic_register_machine& rm) override;
		};

	protected:
		// Стек для управления порядком обработки файлов РМ: пара <имя файла, флаг обработки всех COMPOSITION>
		std::stack<std::pair<std::string, std::optional<size_t>>> _file_stack;

	public:
		// Конструктор
		extended_register_machine(const std::string& filename, bool is_verbose = false) noexcept;

		extended_register_machine(const extended_register_machine&) = delete;
		extended_register_machine& operator=(const extended_register_machine&) = delete;

		// Деструктор
		~extended_register_machine() = default;


		// Запуск РМ
		void run() override;

	protected:
		// Загрузка всех инструкций
		void load_all_instructions(size_t start_position = 0, int border = std::ios::beg) override;

		// Выполнение всех инструкций
		void execute_all_instructions() override;

		// Проверка корректности формата команды композицииэ
		bool is_valid_composition_command(const std::string& command) const noexcept;

		// Обработка всех команд композиции в текущем файла и добавление включаемых файлов в стек
		void _include_files(const std::string& filename);
	};
}

#endif
