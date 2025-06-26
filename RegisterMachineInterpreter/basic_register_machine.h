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
	size_t _carriage; // �������
	std::unordered_map<std::string, int> _registers; // ����������
	std::vector<std::string> _commands; // ������ �������
	std::vector<std::string> _output_registers; // �������� ��������
	std::string _filename; // ��� �����
public:
	basic_register_machine(const std::string& filename) : _filename(filename), _carriage(0), _registers() {}
	~basic_register_machine() = default;

	// �������� ���� ������
	void load_commands();
	// ���������� ���� ������
	void execute_commands();
	// ������ ��
	void run();

	void print_output_registers() const;
private:

	// �������� ������������ �������� ����������
	bool is_valid_condition_command(const std::string& command) const;
	// �������� ������������ ���������� ������������
	bool is_valid_assignment_command(const std::string& command) const;
	// �������� ������������ ������� ������������ �������
	bool is_valid_stop_command(const std::string& command) const;


	// ������� ����������
	void parse_input_arguments(const std::string& line);
	// ������� ����������� (�������� ����������)
	void parse_output_arguments(const std::string& line);
	// ���������� ���������� ������������
	void assigment_command(const std::string& command);
	// ���������� �������� ����������
	void condition_command(const std::string& command);
	// ���������� ������������ ����������
	void stop_command(const std::string& command);
	// �������� ������ �������� ����� � ������ �� ������
	void trim(std::string& line) const;
};




#endif // !__REGISTER_MACHINE_
