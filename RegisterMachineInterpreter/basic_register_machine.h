#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

#include <optional>
#include <string>
#include <unordered_map>

using namespace std::string_literals;

#define SEPARATOR ":"s
#define ASSIGNMENT "<-"s
#define PLUS "+"s
#define MINUS "-"s
#define STOP "stop"s
#define IF "if"s
#define ELSE "else"s
#define THEN "then"s
#define GOTO "goto"s
#define EQUAL "=="s


class basic_register_machine {
private:
	// �������, �������� ����� ������� ����������
	size_t _carriage;
	// ������� ��������� (����������)
	std::unordered_map<std::string, int> _registers;
	// ������ ������
	std::vector<std::string> _commands;
	// ������ �������� ���������
	std::vector<std::string> _output_registers;
	// ��� ��������������� �����
	std::string _filename;
public:
	// �����������
	basic_register_machine(const std::string& filename) noexcept : _filename(filename), _carriage(0), _registers(), _commands(), _output_registers() {}
	// Move-�����������
	basic_register_machine(std::string&& filename) noexcept : _filename(std::move(filename)), _carriage(0), _registers(), _commands(), _output_registers() {}

	// �������� ������������
	basic_register_machine& operator=(const basic_register_machine& other) noexcept {
		if (this != &other) {
			this->_carriage = other._carriage;
			this->_registers = other._registers;
			this->_commands = other._commands;
			this->_output_registers = other._output_registers;
			this->_filename = other._filename;
		}

		return *this;
	}
	// �������� move-������������
	basic_register_machine& operator=(basic_register_machine&& other) noexcept {
		this->_carriage = std::move(other._carriage);
		this->_registers = std::move(other._registers);
		this->_commands = std::move(other._commands);
		this->_output_registers = std::move(other._output_registers);
		this->_filename = std::move(other._filename);
		return *this;
	}

	// ����������
	~basic_register_machine() = default;

	// ������ ��
	void run();

	// �������� ���� ������
	void load_all_commands();
	// ���������� ���� ������
	void execute_all_commands();

	// ������ �������� ���������
	void print_output_registers() const;
private:

	// �������� ������������ ������� �������� ����������
	bool is_valid_condition_command(const std::string& command) const;
	// �������� ������������ ������� ���������� ������������
	bool is_valid_assignment_command(const std::string& command) const;
	// �������� ������������ ������� ������������ �������
	bool is_valid_stop_command(const std::string& command) const;

	// ������� ����������
	void parse_input_arguments(const std::string& line);
	// ������� ����������� (�������� ���������)
	void parse_output_arguments(const std::string& line);

	// ���������� ���������� ������������
	void execute_assigment_command(const std::string& command);
	// ���������� �������� ����������
	void execute_condition_command(const std::string& command);
	// ���������� ������������ ����������
	void execute_stop_command(const std::string& command);

	// �������� ������ �������� ����� � ������ �� ������
	void trim(std::string& line) const;
};




#endif // !__REGISTER_MACHINE_
