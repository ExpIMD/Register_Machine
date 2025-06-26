
#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

#include <unordered_map>
#include <string>
#include <optional>

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

class register_machine {
private:
	size_t _carriage; // Каретка
	std::unordered_map<std::string, int> _registers; // Переменные
	std::vector<std::pair<std::string, std::string>> _commands; // Список комманд
	std::string _filename; // Имя файла
public:
	register_machine(const std::string& filename) : _filename(filename), _carriage(0), _registers() {}
	~register_machine() = default;

	void load_commands(); // Загрузка всех команд файла

	void run(); // Выполнение кода

	void execute_commands(); // Выполняет текущую комманду

	void print_commands() const;
	void print_registers() const;
private:
	void parse_input_arguments(const std::string& line);
	void assigment_command(const std::string& command);
	void condition_command(const std::string& command);
	void stop_command();
	void trim(std::string& line) const;
	
};




#endif // !__REGISTER_MACHINE_
