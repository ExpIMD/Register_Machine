#ifndef __BASIC_REGISTER_MACHINE_
#define __BASIC_REGISTER_MACHINE_

#include <string>
#include <unordered_map>
#include <vector>

using namespace std::string_literals;

// ������� �������� ���� ��

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

// ����� ������� ��
class basic_register_machine {
protected:
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
	basic_register_machine& operator=(const basic_register_machine& other) noexcept;
	// �������� move-������������
	basic_register_machine& operator=(basic_register_machine&& other) noexcept;

	// ����������
	virtual ~basic_register_machine() = default;

	// ������ ��
	virtual void run();

	// �������� ���� ������
	virtual void load_all_commands();
	// ���������� ���� ������
	virtual void execute_all_commands();

	// ������ �������� ���������
	void print_output_registers() const;
protected:

	// �������� ������������ ������� �������� ����������
	virtual bool is_valid_condition_command(const std::string& command) const;
	// �������� ������������ ������� ���������� ������������
	virtual bool is_valid_assignment_command(const std::string& command) const;
	// �������� ������������ ������� ������������ �������
	virtual bool is_valid_stop_command(const std::string& command) const;

	// ���������� ���������� ������������
	virtual void execute_assigment_command(const std::string& command);
	// ���������� �������� ����������
	virtual void execute_condition_command(const std::string& command);
	// ���������� ������������ ����������
	virtual void execute_stop_command(const std::string& command);

	// ������� ����������
	void parse_input_arguments(const std::string& line);
	// ������� ����������� (�������� ���������)
	void parse_output_arguments(const std::string& line);

	// �������� ������ �������� ����� � ������ �� ������
	void trim(std::string& line) const;
	// ��������, ��� � ������ �������� ����������
	bool is_variable(const std::string& line) const;
	// �������� ������������� �������� �� ������
	int get_value(const std::string& line) const;
};

// ����� ����������� ��
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

	// �������� ������������ ������� ������������ �������
	bool is_valid_move_command(const std::string& command) const;
	bool is_valid_goto_command(const std::string& command) const;

	bool is_valid_condition_command(const std::string& command) const override;

	bool is_valid_assignment_command(const std::string& command) const override;

};

#endif
