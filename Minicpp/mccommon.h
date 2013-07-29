/************************************************************************/
/* mccommon.h, ͷ�ļ�����������Minicppʹ�õĺ���, ������Ҫʹ�õ�
 * �����޸�ʱ��2012-9-15, ��Ҫ������var.cpp���溯��������				*/
/************************************************************************/

#include <vector>
using namespace std;
const int MAX_T_LEN  = 128;   // max token length
const int MAX_ID_LEN = 31;    // max identifier length
const int PROG_SIZE  = 10000; // max program size
const int NUM_PARAMS = 31;    // max number of parameters

// Enumeration of token types.
enum tok_types { UNDEFTT, DELIMITER, IDENTIFIER,
                 NUMBER, KEYWORD, TEMP, STRING, BLOCK, TYPE
               };

// Enumeration of internal representation of tokens.
//�󲿷�ָ������C++����Ĺؼ���, ��Ȼcout��cinҲ���ӽ�����
enum token_ireps { UNDEFTOK, ARG, BOOL, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE, STRUCT, FALSE, TRUE, SWITCH,
                   CASE, IF, ELSE, FOR, DO, WHILE, BREAK,
                   RETURN, COUT, CIN, END, ENDL, DEFAULT, CONTINUE, NONE
                 };


//��������ö�ٵ�����������ıȽϹ���,ָ������token������, ����block, ����ǲ����ǹؼ��ֵ�,
//����Ŀ������ɾ��ǹؼ���
struct var;
struct struct_type
{
	struct_type()
	{
		type_name[0] = '\0';
		data.clear();
	}
	char type_name[MAX_ID_LEN + 1];
	vector<var> data;
};

//���Ǹ�������var�ṹ��, �洢�˱��������ͺ�ֵ, �����Ǹ�������,
//����python����ı��������õĸ���
struct anonymous_var
{
	token_ireps var_type; // data type, ����ֻ����Щ��������, ����struct
	long int_value;
	double float_value;
	struct_type struct_value;
};

// This structure encapsulates the info
// associated with variables.
struct var
{
    char var_name[MAX_ID_LEN + 1]; // name
    anonymous_var value;
};




// This structure encapsulates function info.
struct func_type
{
    char func_name[MAX_ID_LEN + 1]; // name
    token_ireps ret_type; // return type
    char *loc; // location of entry point in program
};

// Enumeration of two-character operators, such as <=.
//����ע���1��ʼ, �����õ�0�������.
enum double_ops { LT = 1, LE, GT, GE, EQ, NE, LS, RS, INC, DEC };

// These are the constants used when throwing a
// syntax error exception.
//
// NOTE: SYNTAX is a generic error message used when
// nothing else seems appropriate.
enum error_msg
{
    SYNTAX, NO_EXP, NOT_VAR, DUP_VAR, DUP_FUNC,
    SEMI_EXPECTED, UNBAL_BRACES, FUNC_UNDEF,
    TYPE_EXPECTED, RET_NOCALL, PAREN_EXPECTED,
    WHILE_EXPECTED, QUOTE_EXPECTED, DIV_BY_ZERO,
    BRACE_EXPECTED, COLON_EXPECTED, UNSUPPORTED_TYPE, MORE_MEMBER_THAN_EXPECTED
    //������һ��UNSUPPORTED_TYPE, ����Ϊ���ֻ֧��һЩ�򵥵��ڽ�����
};

//������Щ��ȫ�ֱ���, �������ƽ�����, �����Ժ�����ع��Ļ�����parser�����Ҫ��Ա����.
extern char *prog;  // current location in source code
extern char *p_buf; // points to start of program buffer

extern char token[MAX_T_LEN + 1]; // string version of token
extern tok_types token_type; // contains type of token
extern token_ireps tok; // internal representation of token

extern anonymous_var ret_value; // function return value

extern bool breakfound; // true if break encountered
extern bool continuefound;//true if continue encountered

// Exception class for Mini C++.
class InterpExc
{
    error_msg err;
public:
    InterpExc(error_msg e)
    {
        err = e;
    }
    error_msg get_err()
    {
        return err;
    }
};

// Interpreter prototypes.
void prescan();
void decl_global();
void decl_struct_type();//��ɽṹ�������.
void dump_struct_type(); //��������ڿ���̨�������Ŀǰ�Ѿ������Ľṹ�������.
bool is_struct_type(char* type_name); //��ȫ��������struct������, ���ǲ���һ���Ѿ�������struct
bool get_struct_type_by_name(char* struct_name, struct_type& s_type);
void decl_struct();
bool get_member_var(char* p, anonymous_var*& v);
bool get_type_by_name(char* vname, char* type_name);
void call();
void putback();
void decl_local();
void exec_if();
void find_eob();
void exec_for();
void exec_switch();
void get_params();
void get_args();
void exec_while();
void exec_do();
void exec_cout();
void exec_cin();
void assign_var(char *var_name, anonymous_var value);
bool load_program(char *p, char *fname);


anonymous_var find_var(char *s);
void interp();
void func_ret();
char *find_func(char *name);
bool is_var(char *s);
token_ireps find_var_type(char *s);
void find_eol();


// Parser prototypes, ��Щ������Ҫ����parse���ʽ.
void eval_exp(anonymous_var &value);
void eval_exp0(anonymous_var &value);
void eval_exp1(anonymous_var &value);
void eval_exp2(anonymous_var &value);
void eval_exp3(anonymous_var &value);
void eval_exp4(anonymous_var &value);
void eval_exp5(anonymous_var &value);
void atom(anonymous_var &value);
void sntx_err(error_msg error);
void putback();
bool isdelim(char c);
token_ireps look_up(char *s);
anonymous_var find_var(char *s);
tok_types get_token();
int internal_func(char *s);
bool is_var(char *s);
void init_struct(char* struct_name, anonymous_var& value);


// "Standard library" prototypes, �⼸�����������ǵ�����һ��C++�Ŀ⺯��, ��װ��һ��.
anonymous_var call_getchar();
anonymous_var call_putchar();
anonymous_var call_abs();
anonymous_var call_rand();



//������Щ��Ϊ��֧�ֶ������Ͷ����ӵ�һЩ����.
//��var.cpp����ʵ��.
anonymous_var add(anonymous_var &a, anonymous_var &b);
anonymous_var sub(anonymous_var &a, anonymous_var &b);
anonymous_var mul(anonymous_var &a, anonymous_var &b);
anonymous_var div(anonymous_var &a, anonymous_var &b);
void cin_var(anonymous_var &v);
void cout_var(anonymous_var &v);
bool is_valid_simple_type(token_ireps ti);
void init_var(anonymous_var &v);
void neg_var(anonymous_var &v);
bool zero(double x);
void abs_var(anonymous_var &v);
int cmp(anonymous_var &a, anonymous_var &b);
bool is_float_type(token_ireps type);
bool is_int_type(token_ireps type);
bool get_bool_val(anonymous_var &v);
void adaptive_assign_var(anonymous_var &a, anonymous_var &b);

