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

	// Класс инструкции
	class instruction {
	protected:
		std::string _description;
		basic_register_machine& _rm;
	public:
		instruction(const std::string& description, basic_register_machine& rm) : _description(description), _rm(rm) {}
		virtual ~instruction() = default;
		const std::string& description() const noexcept;
		virtual void execute() noexcept = 0;
	};

	// Класс инструкции присваивания
	class assigment_instruction : public instruction {
	public:
		assigment_instruction(const std::string& description, basic_register_machine& rm) : instruction(description, rm) {}
		void execute() noexcept override;

	};

	class condition_instruction : public instruction {
	public:
		condition_instruction(const std::string& description, basic_register_machine& rm) : instruction(description, rm) {}
		void execute() noexcept override;
	};

	class stop_instruction : public instruction {
	public:
		stop_instruction(const std::string& description, basic_register_machine& rm) : instruction(description, rm) {}
		void execute() noexcept override;
	};

	class extended_condition_instruction : public condition_instruction {
	public:
		extended_condition_instruction(const std::string& description, basic_register_machine& rm) : condition_instruction(description, rm) {}
		void execute() noexcept override;
	};

	class goto_instruction : public instruction {
	public:
		goto_instruction(const std::string& description, basic_register_machine& rm) : instruction(description, rm) {}
		void execute() noexcept override;
	};

	class move_instruction : public instruction {
	public:
		move_instruction(const std::string& description, basic_register_machine& rm) : instruction(description, rm) {}
		void execute() noexcept override;
	};

	// Класс базовой РМ
	class basic_register_machine {
		friend class assigment_instruction;
		friend class condition_instruction;
		friend class stop_instruction;
		friend class goto_instruction;
		friend class extended_condition_instruction;
		friend class move_instruction;
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
		// Проверка корректности формата инструкции присваивания
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
		// Проверка корректности формата инструкции присваивания
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
