#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

#include <regex>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::string_literals;

// Макросы ключевых слов РМ

#define SEPARATOR ":"s
#define ASSIGNMENT "<-"s
#define MOVE "<<-"s
#define PLUS "+"s
#define MINUS "-"s
#define STOP "stop"s
#define IF "if"s
#define ELSE "else"s
#define THEN "then"s
#define GOTO "goto"s
#define EQUAL "=="s
#define RESET "reset"s
#define COMPOSITION "call"s

namespace IMD {

	class basic_register_machine;

	// Удаление лишних пробелов слева и справа от строки
	void trim(std::string& line);

	// Проверка, что в строке записан регистр
	bool is_register(const std::string& line) noexcept;

	// Класс базовой РМ
	class basic_register_machine {
	protected:
		// Класс инструкции
		class instruction {
		protected:
			// Описание
			std::string _description;
			// Регистровая машина
			basic_register_machine& _rm;
		public:
			// Конструктор
			instruction(const std::string& description, basic_register_machine& rm) noexcept;
			// Деструктор
			virtual ~instruction() = default;

			// Возвращает описание инструкции
			const std::string& description() const noexcept;
			// Выполняет инструкцию
			virtual void execute() noexcept = 0;
		};

		// Класс инструкции копирующего присваивания
		class assigment_instruction : public instruction {
		public:
			// Конструктор
			assigment_instruction(const std::string& description, basic_register_machine& rm) noexcept;
			// Деструктор
			~assigment_instruction() override = default;

			// Выполнение инструкции копирующего присваивания
			void execute() noexcept override;
		};

		// Класс условной инструкции
		class condition_instruction : public instruction {
		public:
			// Конструктор
			condition_instruction(const std::string& description, basic_register_machine& rm) noexcept;
			// Деструктор
			~condition_instruction() override = default;

			// Выполнение условной инструкции
			void execute() noexcept override;
		};

		// Класс остановочной инструкции
		class stop_instruction : public instruction {
		public:
			// Конструктор
			stop_instruction(const std::string& description, basic_register_machine& rm) noexcept;
			// Деструктор
			~stop_instruction() override = default;

			// Выполнение остановочной инструкции
			void execute() noexcept override;
		};

		// Класс расширенной условной инструкции
		class extended_condition_instruction : public condition_instruction {
		public:
			// Конструктор
			extended_condition_instruction(const std::string& description, basic_register_machine& rm) noexcept;
			// Деструктор
			~extended_condition_instruction() override = default;

			// Выполнение расширенной условной инструкции
			void execute() noexcept override;
		};

		// Класс инструкции передвижения
		class goto_instruction : public instruction {
		public:
			// Конструктор
			goto_instruction(const std::string& description, basic_register_machine& rm) noexcept;
			// Деструктор
			~goto_instruction() override = default;

			// Выполнение инструкции передвижения
			void execute() noexcept override;
		};

		// Класс инструкции перемещающего присваивания
		class move_instruction : public instruction {
		public:
			// Конструктор
			move_instruction(const std::string& description, basic_register_machine& rm) noexcept;
			// Деструктор
			~move_instruction() override = default;

			// Выполнение инструкции перемещающего присваивания
			void execute() noexcept override;
		};
	protected:
		// Флаг, указывающий остановлена ли РМ
		bool _is_stopped;

		// Каретка, описывающая номер текущей инструкции
		size_t _carriage;

		// Словарь регистров
		std::unordered_map<std::string, int> _registers;

		// Вектор инструкций
		std::vector<std::unique_ptr<instruction>> _instructions;

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

	protected:
		// Загрузка всех инструкций
		virtual void load_all_instructions();
		// Выполнение всех инструкций
		virtual void execute_all_instructions();

		// Проверка корректности формата условной инструкции
		virtual bool is_valid_condition_instruction(const std::string& instruction) const noexcept;
		// Проверка корректности формата инструкции копирующего присваивания
		virtual bool is_valid_assignment_instruction(const std::string& instruction) const noexcept;
		// Проверка корректности формата остановочной команды
		virtual bool is_valid_stop_instruction(const std::string& instruction) const noexcept;
		// Проверка корректности формата строки с входными регистрами
		virtual bool is_valid_input_registers_line(const std::string& instruction) const noexcept;
		// Проверка корректности формата строки с выходными регистрами
		virtual bool is_valid_output_registers_line(const std::string& instruction) const noexcept;

		// Парсинг аргументов
		void parse_input_registers(const std::string& line);
		// Парсинг результатов (выходных регистров)
		void parse_output_registers(const std::string& line);

		// Получает целочисленное значение из строки, в которой может быть описани литерал или регистр
		int get_value(const std::string& line);
	};

	// Класс расширенной РМ
	class extended_register_machine : public basic_register_machine {
		friend class assigment_instruction;
		friend class condition_instruction;
		friend class stop_instruction;
		friend class expended_condition_instruction;
	protected:
		// Стек для управления порядком обработки файлов РМ: пара <имя файла, флаг обработки всех COMPOSITION>
		std::stack<std::pair<std::string, bool>> _file_stack;

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
		void load_all_instructions() override;

		// Выполнение всех инструкций
		void execute_all_instructions() override;

		// Проверка корректности формата условной инструкции
		bool is_valid_condition_instruction(const std::string& instruction) const noexcept override;
		// Проверка корректности формата инструкции копирующего присваивания
		bool is_valid_assignment_instruction(const std::string& instruction) const noexcept override;

		// Проверка корректности формата инструкции перемещения
		bool is_valid_move_instruction(const std::string& instruction) const noexcept;
		// Проверка корректности формата инструкции передвижения
		bool is_valid_goto_instruction(const std::string& instruction) const noexcept;

		// Обработка всех команд композиции в текущем файла и добавление включаемых файлов в стек
		void _include_files(const std::string& filename);
	};
}

#endif
