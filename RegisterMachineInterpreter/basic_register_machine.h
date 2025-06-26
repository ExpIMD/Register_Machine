#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

#include <optional>
#include <string>
#include <unordered_map>


#define SEPARATOR ":"
#define ASSIGNMENT "<-"
#define PLUS "+"
#define MINUS "-"
#define STOP "stop"
#define IF "if"
#define ELSE "else"
#define THEN "then"
#define GOTO "goto"
#define EQUAL "=="

class basic_register_machine {
private:
	size_t _carriage; // Каретка
	std::unordered_map<std::string, int> _registers; // Переменные
	std::vector<std::string> _commands; // Список комманд
	std::vector<std::string> _output_registers; // Выходные регистры
	std::string _filename; // Имя файла
public:
	basic_register_machine(const std::string& filename) : _filename(filename), _carriage(0), _registers() {}
	~basic_register_machine() = default;

	// Загрузка всех команд
	void load_commands();
	// Выполнение всех команд
	void execute_commands();
	// Запуск РМ
	void run();

	void print_output_registers() const;
private:

	// Проверка корректности условной инструкции
	bool is_valid_condition_command(const std::string& command) const;
	// Проверка корректности инструкции присваивания
	bool is_valid_assignment_command(const std::string& command) const;
	// Проверка корректности формата остановочной команды
	bool is_valid_stop_command(const std::string& command) const;


	// Парсинг аргументов
	void parse_input_arguments(const std::string& line);
	// Парсинг результатов (выходных параметров)
	void parse_output_arguments(const std::string& line);
	// Выполнение инструкции присваивания
	void assigment_command(const std::string& command);
	// Выполнение условной инструкции
	void condition_command(const std::string& command);
	// Выполнение остановочной инструкции
	void stop_command(const std::string& command);
	// Удаление лишних пробелов слева и справа от строки
	void trim(std::string& line) const;
};




#endif // !__REGISTER_MACHINE_
