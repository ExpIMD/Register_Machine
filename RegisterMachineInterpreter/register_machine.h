#ifndef __BASIC_REGISTER_MACHINE_
#define __BASIC_REGISTER_MACHINE_

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

// Класс базовой РМ
class basic_register_machine {
protected:
	// Каретка, хранящая номер текущей инструкции
	size_t _carriage;

	// Словарь регистров (переменных)
	std::unordered_map<std::string, int> _registers;

	// Вектор команд
	std::vector<std::string> _commands;

	// Вектор выходных регистров
	std::vector<std::string> _output_registers;

	// Имя обрабатываемого файла
	std::string _filename;
public:
	// Конструктор
	basic_register_machine(const std::string& filename) noexcept : _filename(filename), _carriage(0), _registers(), _commands(), _output_registers() {}
	// Move-Конструктор
	basic_register_machine(std::string&& filename) noexcept : _filename(std::move(filename)), _carriage(0), _registers(), _commands(), _output_registers() {}

	// Оператор присваивания
	basic_register_machine& operator=(const basic_register_machine& other) noexcept;
	// Оператор move-присваивания
	basic_register_machine& operator=(basic_register_machine&& other) noexcept;

	// Деструктор
	virtual ~basic_register_machine() = default;

	// Запуск РМ
	virtual void run();

	// Загрузка всех команд
	virtual void load_all_commands();
	// Выполнение всех команд
	virtual void execute_all_commands();

	// Печать выходных регистров
	void print_output_registers() const;
protected:

	// Проверка корректности формата условной инструкции
	virtual bool is_valid_condition_command(const std::string& command) const;
	// Проверка корректности формата инструкции присваивания
	virtual bool is_valid_assignment_command(const std::string& command) const;
	// Проверка корректности формата остановочной команды
	virtual bool is_valid_stop_command(const std::string& command) const;

	// Выполнение инструкции присваивания
	virtual void execute_assigment_command(const std::string& command);
	// Выполнение условной инструкции
	virtual void execute_condition_command(const std::string& command);
	// Выполнение остановочной инструкции
	virtual void execute_stop_command(const std::string& command);

	// Парсинг аргументов
	void parse_input_arguments(const std::string& line);
	// Парсинг результатов (выходных регистров)
	void parse_output_arguments(const std::string& line);

	// Удаление лишних пробелов слева и справа от строки
	void trim(std::string& line) const;
	// Проверка, что в строке записана переменная
	bool is_variable(const std::string& line) const;
	// Получает целочисленное значение из строки
	int get_value(const std::string& line) const;
};

// Класс расширенной РМ
class extended_register_machine : public basic_register_machine {
public:
	extended_register_machine(const std::string& filename) : basic_register_machine(filename) {}
	extended_register_machine(std::string&& filename) : basic_register_machine(filename) {}
	~extended_register_machine() = default;

	void execute_all_commands() override;

protected:
	void execute_assigment_command(const std::string& command) override;
	void execute_condition_command(const std::string& command) override;

	void execute_move_command(const std::string& command);
	void execute_goto_command(const std::string& command);

	// Проверка корректности формата перемещающей команды
	bool is_valid_move_command(const std::string& command) const;
	bool is_valid_goto_command(const std::string& command) const;

	bool is_valid_condition_command(const std::string& command) const override;

	bool is_valid_assignment_command(const std::string& command) const override;

};

#endif
