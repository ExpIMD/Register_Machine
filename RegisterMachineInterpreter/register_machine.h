#ifndef __REGISTER_MACHINE_
#define __REGISTER_MACHINE_

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <stack>

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
#define COMPOSITION "call"s

// �������� ������ �������� ����� � ������ �� ������
void trim(std::string& line);

// ����� ������� ��
class basic_register_machine {
protected:
	// �������, ����������� ����� ������� ����������
	size_t _carriage;

	// ������� ���������
	std::unordered_map<std::string, int> _registers;

	// ������ ����������
	std::vector<std::string> _instructions;

	// ������ �������� ���������
	std::vector<std::string> _output_registers;

	// ������ ������� ���������
	std::vector<std::string> _input_registers;

	// ��� ��������������� �����
	std::string _filename;

	// ����, ����������� ����� ���������� ������ ������ ��
	bool _is_verbose;

public:
	// �����������
	basic_register_machine(const std::string& filename, bool is_verbose = false) noexcept : _filename(filename), _is_verbose(is_verbose), _carriage(0), _registers(), _instructions(), _output_registers() {}
	// Move-�����������
	basic_register_machine(std::string&& filename, bool is_verbose = false) noexcept : _filename(std::move(filename)), _is_verbose(is_verbose), _carriage(0), _registers(), _instructions(), _output_registers() {}
	
	// ����������
	virtual ~basic_register_machine() = default;

	// �������� ������������
	basic_register_machine& operator=(const basic_register_machine& other) noexcept;
	// Move-�������� ������������
	basic_register_machine& operator=(basic_register_machine&& other) noexcept;

	// ������ ��
	virtual void run();
	// ����� ��
	virtual void reset();

	// ������ ������� ���������
	void print_input_registers(const std::string& separator) const;
	// ������ ���� ���������
	void print_all_registers(const std::string& separator) const;
	// ������ �������� ���������
	void print_output_registers(const std::string& separator) const;
	// ������ �������
	void print_carriage(const std::string& separator) const;

protected:
	// �������� ���� ������
	virtual void load_all_instructions();
	// ���������� ���� ����������
	virtual void execute_all_instructions();

	// �������� ������������ ������� �������� ����������
	virtual bool is_valid_condition_instruction(const std::string& command) const;
	// �������� ������������ ������� ���������� ������������
	virtual bool is_valid_assignment_instruction(const std::string& command) const;
	// �������� ������������ ������� ������������ �������
	virtual bool is_valid_stop_instruction(const std::string& command) const;

	// ���������� ���������� ������������
	virtual void execute_assigment_instruction(const std::string& command);
	// ���������� �������� ����������
	virtual void execute_condition_instruction(const std::string& command);
	// ���������� ������������ ����������
	virtual void execute_stop_instruction(const std::string& command);

	// ������� ����������
	void parse_input_arguments(const std::string& line);
	// ������� ����������� (�������� ���������)
	void parse_output_arguments(const std::string& line);

	// ��������, ��� � ������ �������� ����������
	bool is_variable(const std::string& line) const; // TODO: ������ �������� ���������� ����������

	// �������� ������������� �������� �� ������, � ������� ����� ���� ������� ������� ��� �������
	int get_value(const std::string& line);
};

// ����� ����������� ��
class extended_register_machine : public basic_register_machine {
protected:
	// ���� ��� ���������� �������� ��������� ������ ��: ���� <��� �����, ���� ��������� ���� COMPOSITION>
	std::stack<std::pair<std::string, bool>> _file_stack;

public:
	// �����������
	extended_register_machine(const std::string& filename, bool is_verbose = false) : basic_register_machine(filename, is_verbose) {}
	// Move-�����������
	extended_register_machine(std::string&& filename, bool is_verbose = false) : basic_register_machine(filename, is_verbose) {}

	// ����������
	~extended_register_machine() = default;

	// ������ ��
	void run() override;

protected:
	// ���������� ���� ����������
	void execute_all_instructions() override;
	void load_all_instructions() override;

	void execute_assigment_instruction(const std::string& command) override;
	void execute_condition_instruction(const std::string& command) override;

	void execute_move_command(const std::string& command);
	void execute_goto_command(const std::string& command);

	// �������� ������������ ������� ������������ �������
	bool is_valid_move_command(const std::string& command) const;
	bool is_valid_goto_command(const std::string& command) const;

	bool is_valid_condition_instruction(const std::string& command) const override;
	bool is_valid_assignment_instruction(const std::string& command) const override;

	void _include_files(const std::string& filename);
};


#endif
