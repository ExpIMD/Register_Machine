
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

class register_machine {
private:
	size_t _carriage; // �������
	std::unordered_map<std::string, int> _registers; // ����������
	std::vector<std::pair<std::string, std::string>> _commands; // ������ �������
	std::string _filename; // ��� �����
public:
	register_machine(const std::string& filename) : _filename(filename), _carriage(0), _registers() {}
	~register_machine() = default;

	void load_commands(); // �������� ���� ������ �����

	void run(); // ���������� ����

	void execute_command(); // ��������� ������� ��������

	void print_commands() const;
	void print_registers() const;
private:
	void parsing_input_arguments(const std::string& line);
	void assigment_command(const std::string& command);
	void trim(std::string& line) const;
	void stop_command();
};




#endif // !__REGISTER_MACHINE_
