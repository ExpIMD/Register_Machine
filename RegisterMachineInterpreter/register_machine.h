#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <stack>

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

// Удаление лишних пробелов слева и справа от строки
void trim(std::string& line);

// Класс базовой РМ
class basic_register_machine {
protected:
	// Каретка, описывающая номер текущей инструкции
	size_t _carriage;

	// Словарь регистров
	std::unordered_map<std::string, int> _registers;

	// Вектор инструкций
	std::vector<std::string> _instructions;

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
	basic_register_machine(const std::string& filename, bool is_verbose = false) noexcept : _filename(filename), _is_verbose(is_verbose), _carriage(0), _registers(), _instructions(), _output_registers() {}
	// Move-Конструктор
	basic_register_machine(std::string&& filename, bool is_verbose = false) noexcept : _filename(std::move(filename)), _is_verbose(is_verbose), _carriage(0), _registers(), _instructions(), _output_registers() {}
	
	// Деструктор
	virtual ~basic_register_machine() = default;

	// Оператор присваивания
	basic_register_machine& operator=(const basic_register_machine& other) noexcept;
	// Move-оператор присваивания
	basic_register_machine& operator=(basic_register_machine&& other) noexcept;

	// Запуск РМ
	virtual void run();
	// Сброс РМ
	virtual void reset();

	// Печать входных регистров
	void print_input_registers(const std::string& separator) const;
	// Печать всех регистров
	void print_all_registers(const std::string& separator) const;
	// Печать выходных регистров
	void print_output_registers(const std::string& separator) const;
	// Печать каретки
	void print_carriage(const std::string& separator) const;

protected:
	// Загрузка всех команд
	virtual void load_all_instructions();
	// Выполнение всех инструкций
	virtual void execute_all_instructions();

	// Проверка корректности формата условной инструкции
	virtual bool is_valid_condition_instruction(const std::string& command) const;
	// Проверка корректности формата инструкции присваивания
	virtual bool is_valid_assignment_instruction(const std::string& command) const;
	// Проверка корректности формата остановочной команды
	virtual bool is_valid_stop_instruction(const std::string& command) const;

	// Выполнение инструкции присваивания
	virtual void execute_assigment_instruction(const std::string& command);
	// Выполнение условной инструкции
	virtual void execute_condition_instruction(const std::string& command);
	// Выполнение остановочной инструкции
	virtual void execute_stop_instruction(const std::string& command);

	// Парсинг аргументов
	void parse_input_arguments(const std::string& line);
	// Парсинг результатов (выходных регистров)
	void parse_output_arguments(const std::string& line);

	// Проверка, что в строке записана переменная
	bool is_variable(const std::string& line) const; // TODO: задать стандарт именования переменной

	// Получает целочисленное значение из строки, в которой может быть описани литерал или регистр
	int get_value(const std::string& line);
};

// Класс расширенной РМ
class extended_register_machine : public basic_register_machine {
protected:
	// Стек для управления порядком обработки файлов РМ: пара <имя файла, флаг обработки всех COMPOSITION>
	std::stack<std::pair<std::string, bool>> _file_stack;

public:
	// Конструктор
	extended_register_machine(const std::string& filename, bool is_verbose = false) : basic_register_machine(filename, is_verbose) {}
	// Move-конструктор
	extended_register_machine(std::string&& filename, bool is_verbose = false) : basic_register_machine(filename, is_verbose) {}

	// Деструктор
	~extended_register_machine() = default;

	// Запуск РМ
	void run() override;

protected:
	// Выполнение всех инструкций
	void execute_all_instructions() override;
	void load_all_instructions() override;

	void execute_assigment_instruction(const std::string& command) override;
	void execute_condition_instruction(const std::string& command) override;

	void execute_move_command(const std::string& command);
	void execute_goto_command(const std::string& command);

	// Проверка корректности формата перемещающей команды
	bool is_valid_move_command(const std::string& command) const;
	bool is_valid_goto_command(const std::string& command) const;

	bool is_valid_condition_instruction(const std::string& command) const override;
	bool is_valid_assignment_instruction(const std::string& command) const override;

	void _include_files(const std::string& filename);
};


#endif
