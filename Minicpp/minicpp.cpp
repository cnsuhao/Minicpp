/***********************************************************************
minicpp.cpp ������������, ����for, if while switch�ȵ�ʵ��Ҳд��������.
************************************************************************/
#include <iostream>
#include <fstream>
#include <new>
#include <stack>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include "mccommon.h"

using namespace std;

char *prog;  // current execution point in source code
char *p_buf; // points to start of program buffer

// This vector holds info for global variables.
vector<var> global_vars;

// This vector holds info for local variables
// and parameters.
vector<var> local_var_stack;

// This vector holds info about functions.
vector<func_type> func_table;

// Stack for managing function scope.

stack<int> func_call_stack;

vector<struct_type> struct_decls;//�����洢struct���͵Ķ���..

// Stack for managing nested scopes.
//���ε�ջ, �洢���Ǳ�����ѹջ֮ǰջ�Ĵ�С.
stack<int> nest_scope_stack;

char token[MAX_T_LEN + 1]; // current token
tok_types token_type; // token type
token_ireps tok; // internal representation

anonymous_var ret_value; // function return value

bool breakfound = false; // true if break encountered
bool continuefound = false;




int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        cout << "Usage: minicpp <filename>\n";
        return 1;
    }

    // Allocate memory for the program.
    try
    {
        p_buf = new char[PROG_SIZE];
    }
    catch (bad_alloc exc)
    {
        cout << "Could Not Allocate Program Buffer\n";
        return 1;
    }

    // Load the program to execute.
    if(!load_program(p_buf, argv[1])) return 1;

    // Set program pointer to start of program buffer.
    prog = p_buf;

    try
    {
        // Find the location of all functions
        // and global variables in the program.
        prescan();

        // Next, set up the call to main().

        // Find program starting point.
        prog = find_func("main");

        // Check for incorrect or missing main() function.
        if(!prog)
        {
            cout << "main() Not Found\n";
            return 1;
        }

        // Back up to opening (.
        prog--;

        // Set the first token to main
        strcpy(token, "main");

        // Call main() to start interpreting.
        call();
    }
    catch(InterpExc exc)
    {
        sntx_err(exc.get_err());
        return 1;
    }
    catch(bad_alloc exc)
    {
        cout << "Out Of Memory\n";
        return 1;
    }

    return ret_value.int_value;
}

// Load a program.
bool load_program(char *p, char *fname)
{
    int i = 0;

    ifstream in(fname, ios::in | ios::binary);
    if(!in)
    {
        cout << "Cannot Open file.\n";
        return false;
    }

    do
    {
        *p = in.get();
        p++;
        i++;
    }
    while(!in.eof() && i < PROG_SIZE);

    if(i == PROG_SIZE)
    {
        cout << "Program Too Big\n";
        return false;
    }

    // Null terminate the program. Skip any EOF
    // mark if present in the file.
    if(*(p - 2) == 0x1a) *(p - 2) = '\0';
    else *(p - 1) = '\0';

    in.close();

    return true;
}

// Find the location of all functions in the program
// and store global variables.
void prescan()
{
    char *p, *tp;
    char temp[MAX_ID_LEN + 1];
    token_ireps datatype;
    func_type ft;

    // When brace is 0, the current source position
    // is outside of any function.
    int brace = 0;

    p = prog;

    do
    {
        // Bypass code inside functions, brace==0, ��֤����������ȫ��������
        while(brace)
        {
            get_token();
            if(tok == END) //���������һ��, ��Ϊ��"{{"�������ַ���..
			{
				throw InterpExc(UNBAL_BRACES);
			}
			if(0 == strcmp(token, "{")) brace++;
			if(0 == strcmp(token, "}")) brace--;
        }

        tp = prog; // save current position
        get_token();

		if(tok == STRUCT) //�����ﴦ��ṹ�������.
		{
			char* ttp = prog; //������������һ�λ��˵�, ��Ȼ��ô����ʵ�Ѿ�������..
			get_token();
			if(token_type == IDENTIFIER)
			{
				get_token();
				if(0 == strcmp(token, "{"))//�������������һ������, ���������һ������ struct mm a;�����Ķ���
				{
					prog = tp;
					decl_struct_type();
				}
				else
				{
					//printf("%s jackiessss\n", token);
					prog = ttp;
					decl_global();
				}
			}
		}
        else if(is_valid_simple_type(tok) || is_struct_type(token)) // See if global var type or function return type.
        {

            datatype = tok; // save data type
            get_token();

            if(token_type == IDENTIFIER)
            {
                strcpy(temp, token);
                get_token();

                if(0 != strcmp(token, "("))   // must be global var
                {
                    prog = tp; // return to start of declaration
                    decl_global();
                }
                else if(*token == '(')   // must be a function
                {
                    // See if function already defined.
                    for(unsigned i = 0; i < func_table.size(); i++)
                        if(!strcmp(func_table[i].func_name, temp))
                            throw InterpExc(DUP_FUNC);

                    ft.loc = prog;
                    ft.ret_type = datatype;
                    strcpy(ft.func_name, temp);
                    func_table.push_back(ft);

                    do
                    {
                        get_token();
                    }
                    while(0 != strcmp(token, ")")); 
                    // Next token will now be opening curly
                    // brace of function.
                }
                else putback();
            }
        }
        else
        {
            if(0 == strcmp(token, "{")) brace++;
			if(0 == strcmp(token, "}")) brace--;
        }
    }
    while(tok != END);
    if(brace) 
	{
		throw InterpExc(UNBAL_BRACES);
	}
    prog = p;
}

// Interpret a single statement or block of code. When
// interp() returns from its initial call, the final
// brace (or a return) in main() has been encountered.

//����interp������һ��С�Ķ�, ���ִ�����������break, ��ô�����Ƴ�interp֮ǰ�ó��������block�Ĵ��붼��һ��, ���ǲ�ִ����
//����, �Ժ����interp�ĳ���Ͳ�����Ϊbreak������������������.
//��interp����, ����{}�����һ���µ����ֿռ�, ����int ��char����declareһ��local����
void interp()
{
	//printf("interp\n");
    anonymous_var value;
    int block = 0;
    char *tmp_prog = NULL;
    //break���������Ŀ����������Ӱ��, ����continue����, ��ֻ�᲻�ñ���ѭ���������䲻ִ��.
    //���ǻ���Ҫά��һ��ȫ�ֵ�continue, ��Ϊ��block��Ҫ֪����block�����ǲ�����continue;
    do
    {
        if(breakfound || continuefound)
        {

            //������Ǹ�{}�����Ŀ�, ��ô����find_eob��������Ե�
            if(block && tmp_prog)
            {

                prog = tmp_prog;
                find_eob();
            }
            else
            {
                //����֪ʶһ�����Ŀ�, ��break����֮ǰ�Ե�����ֺ�
                get_token();

            }
            return;
        }

        token_type = get_token();

		//printf("%s token\n", token);
        //������Щexec_while, exec_while�Ǹ���ǰ����token���������������
        //��eval_expû�й�ϵ.

        // See what kind of token is up.
		if((token_type == IDENTIFIER ||
			*token == INC || *token == DEC) && !is_struct_type(token))
        {
            // Not a keyword, so process expression.
            putback();  // restore token to input stream for
            // further processing by eval_exp()
            eval_exp(value); // process the expression
            //eval_exp��exec_while����ͬ�Ĳ��, ��interp������ǰ���ַ���ʱ��, �ͻ�ݹ������Ӧ�Ĺ���.
            if(0 != strcmp(token, ";")) 
			{
				throw InterpExc(SEMI_EXPECTED);
			}
        }
        else if(token_type == BLOCK) // block delimiter?
        {
			if(0 == strcmp(token, "{"))   // is a block
            {
                putback();
                tmp_prog = prog;
                get_token();
                block = 1; // interpreting block, not statement
                // Record nested scope.
                nest_scope_stack.push(local_var_stack.size());
                //nest_scope_stack����������һ��block��stack��λ��,
                //�û��ָ�ջ.
            }
            else   // is a }, so reset scope and return
            {
                // Reset nested scope.
                local_var_stack.resize(nest_scope_stack.top());
                nest_scope_stack.pop();
                return;
            }
        } 
        else if(is_valid_simple_type(tok) || is_struct_type(token) || tok == STRUCT)
        {
			if(tok != STRUCT) putback(); //���tok ��STRUCT��ô�Ͳ��»�ȥ�Ǹ�token, ��������struct��ʼһ���ṹ�嶨��Ϳ�����..
            decl_local();
        }
        else // is keyword
            switch(tok)
            {
            case RETURN:  // return from function call, ��Ҫ����������ֲ���������, call�������˴���.
                /*if(block)
                {
                	local_var_stack.resize(nest_scope_stack.top());
                	nest_scope_stack.pop();
                }*/
                func_ret();
                return;
            case IF:      // process an if statement
                exec_if();
                break;
            case ELSE:    // process an else statement
                find_eob(); // find end of else block
                // and continue execution
                break;
            case WHILE:   // process a while loop
                exec_while();
                break;
            case DO:      // process a do-while loop
                exec_do();
                break;
            case FOR:     // process a for loop
                exec_for();

                break;
            case BREAK:   // handle break
                breakfound = true;
                // Reset nested scope.
                //����Ҫ����һ���ǲ��Ǵ�һ��block�����break, ��Ϊ�����޸�֮��, for while��ѭ�������ڿ�����
                //һ�������������
                if(block)
                {
                    local_var_stack.resize(nest_scope_stack.top());
                    nest_scope_stack.pop();
                }
                break;
            case CONTINUE:
            {
                continuefound = true;
                if(block)
                {
                    local_var_stack.resize(nest_scope_stack.top());
                    nest_scope_stack.pop();
                }
                break;
            }
            case SWITCH:  // handle a switch statement
                exec_switch();
                break;
            case COUT:    // handle console output
                exec_cout();
                //cout << "breakfuond :" << breakfound << endl;
                break;
            case CIN:     // handle console input
                exec_cin();
                break;
            case END:
                exit(0);

            }
    }
    while (tok != END && block);
    return;
}


//����ʹ��map�Ż�.
// Return the entry point of the specified function.
// Return NULL if not found.
char *find_func(char *name)
{
    unsigned i;
	unsigned len = func_table.size();
    for(i = 0; i < len; i++)
        if(!strcmp(name, func_table[i].func_name))
            return func_table[i].loc;

    return NULL;
}

// Declare a global variable, including struct variable.
void decl_global()
{
    token_ireps vartype;
    var v;
    get_token(); // get type
	vartype = tok; // save var type
	char struct_name[64] = "";
	if(is_struct_type(token)) 
	{
		vartype = STRUCT;
		strcpy(struct_name, token);//�鹵�Ǹ��ṹ��, ��ô��������ѽṹ�����͵����ָ���,
								   //�����ʼ����ʱ�����������ֶԽṹ����г�ʼ��
	}
    anonymous_var value;
    // Process comma-separated list.
    do
    {
		get_token(); // get name
		//printf("%s name\n", token);

        v.value.var_type = vartype;
		strcpy(v.var_name, token);
		if(vartype == STRUCT) strcpy(v.value.struct_value.type_name, struct_name);
        init_var(v.value); // init to 0, or init the struct_type considering the struct name.
        
        // See if variable is a duplicate.
		unsigned sz = global_vars.size();
        for(unsigned i = 0; i < sz; i++)
            if(!strcmp(global_vars[i].var_name, token))
                throw InterpExc(DUP_VAR);

        global_vars.push_back(v);

		//printf("%s jjjjj\n", token);
        putback();
        eval_exp(value); //���eval_exp��ʵ�ָ�ֵ, ����valueֻ�Ǹ���Ԫ, ���ǲ���
        get_token();
    }
    while(*token == ',');

    if(*token != ';') 
	{
		throw InterpExc(SEMI_EXPECTED);
	}
}

// Declare a local variable.
void decl_local()
{
	token_ireps vartype;
	var v;
	get_token(); // get type
	//printf("%s local\n", token);
	vartype = tok; // save var type
	char struct_name[64] = "";
	if(is_struct_type(token)) 
	{
		//printf("decl_local, %s\n", token);
		vartype = STRUCT;
		strcpy(struct_name, token);//�鹵�Ǹ��ṹ��, ��ô��������ѽṹ�����͵����ָ���,
		//�����ʼ����ʱ�����������ֶԽṹ����г�ʼ��
	}
	anonymous_var value;

    // Process comma-separated list.
    do
    {
        get_token(); // get var name
		v.value.var_type = vartype;
		strcpy(v.var_name, token);
		if(vartype == STRUCT) strcpy(v.value.struct_value.type_name, struct_name);
		init_var(v.value); // init to 0, or init the struct_type considering the struct name.

        // See if variable is already the name
        // of a local variable in this scope.
        if(!local_var_stack.empty())
            for(int i = local_var_stack.size() - 1;
                    i >= nest_scope_stack.top(); i--)
            {
                if(!strcmp(local_var_stack[i].var_name, token))
                    throw InterpExc(DUP_VAR);
            }

        strcpy(v.var_name, token);
        local_var_stack.push_back(v);
        putback();
        eval_exp(value);//���eval_exp��ʵ�ָ�ֵ, ����valueֻ�Ǹ���Ԫ, ���ǲ���
        get_token();
    }
    while(*token == ',');

    if(*token != ';') throw InterpExc(SEMI_EXPECTED);
}

// Call a function.
void call()
{
    char *loc, *temp;
    int lvartemp;

    // First, find entry point of function.
    loc = find_func(token);

    if(loc == NULL)
        throw InterpExc(FUNC_UNDEF); // function not defined
    else
    {
        // Save local var stack index.
        lvartemp = local_var_stack.size();

        //get_args ��get_params�Ⱥ���� , ������һ���滻
        get_args(); // get function arguments
        temp = prog; // save return location

        func_call_stack.push(lvartemp); // push local var index

        prog = loc; // reset prog to start of function
        get_params(); // load the function's parameters with
        // the values of the arguments

        interp(); // interpret the function

        prog = temp; // reset the program pointer

        if(func_call_stack.empty()) throw InterpExc(RET_NOCALL);

        // Reset local_var_stack to its previous state.

        //�����resize��Ѻ���ĸո�ѹ��ջ�ı���ɾ��.
        local_var_stack.resize(func_call_stack.top());
        func_call_stack.pop();
    }
}

// Push the arguments to a function onto the local
// variable stack.
void get_args()
{
    anonymous_var value, temp[NUM_PARAMS];
    int count = 0;
    var vt;

    count = 0;
    get_token();
    if(*token != '(') throw InterpExc(PAREN_EXPECTED);

    // Process a comma-separated list of values.
    do
    {
        eval_exp(value);
        temp[count] = value; // save temporarily
        get_token();
        count++;
    }
    while(*token == ',');
    count--;

    // Now, push on local_var_stack in reverse order.
    for(; count >= 0; count--)
    {
        vt.value = temp[count];
        local_var_stack.push_back(vt);
    }
}

// Get function parameters.

//�������������ʵ���˴�ʵ�ε��βε�ת������, ����.
void get_params()
{
    var *p;
    int i;

    i = local_var_stack.size() - 1;

    // Process comma-separated list of parameters.
    do
    {
        get_token();
        p = &local_var_stack[i];
        if(*token != ')' )
        {
            if(is_valid_simple_type(tok))
                throw InterpExc(TYPE_EXPECTED);

            p->value.var_type = tok;
            get_token();

            // Link parameter name with argument already on
            // local var stack.
            strcpy(p->var_name, token);
            get_token();
            i--;
        }
        else break;
    }
    while(*token == ',');

    //����������һ��, �����һ���������ǲ���')'
    if(*token != ')') throw InterpExc(PAREN_EXPECTED);
}

// Return from a function.
void func_ret()
{
    anonymous_var value;

    //value = 0;

    // Get return value, if any.
    //Ŀǰ�趨��ֻ֧��int����ֵ.
    eval_exp(value);

    ret_value = value;
}

// Assign a value to a variable.
void assign_var(char *vname, anonymous_var value)
{
	//printf("assign %s\n", vname);
    //first check if it's a member of a struct
    bool b_contains_dots = false;
	int len = strlen(vname);
	for(int i = 0; i < len; i++)
	{
		if(vname[i] == '.') 
		{
			b_contains_dots = true;
			break;
		}
	}
	if(b_contains_dots)
	{
		anonymous_var* vp = NULL;
		get_member_var(vname, vp);
		adaptive_assign_var(*vp, value);
		return;
	}
    // First, see if it's a local variable.
    // 
    if(!local_var_stack.empty())
        for(int i = local_var_stack.size() - 1;
                i >= func_call_stack.top(); i--)
        {
            if(!strcmp(local_var_stack[i].var_name,
                       vname))
            {
                adaptive_assign_var(local_var_stack[i].value, value);
                return;
            }
        }

    // Otherwise, try global vars.
    for(unsigned i = 0; i < global_vars.size(); i++)
        if(!strcmp(global_vars[i].var_name, vname))
        {
            adaptive_assign_var(global_vars[i].value, value);
            //cout << value.float_value << " >>>" << endl;
            return;
        }
	//printf("jackiesteed\n");
    throw InterpExc(NOT_VAR); // variable not found
}

// Find the value of a variable, �������Ҫ����, ��Ϊ���������м�����������õ�.
anonymous_var find_var(char *vname)
{
	anonymous_var* vp = NULL;
	if(get_member_var(vname, vp)) 
		return *vp;
    // First, see if it's a local variable.
    if(!local_var_stack.empty())
        for(int i = local_var_stack.size() - 1;
                i >= func_call_stack.top(); i--)
        {
            if(!strcmp(local_var_stack[i].var_name, vname))
                return local_var_stack[i].value;
        }

    // Otherwise, try global vars.
    for(unsigned i = 0; i < global_vars.size(); i++)
        if(!strcmp(global_vars[i].var_name, vname))
            return global_vars[i].value;
	//printf("jackiesteed %s\n", token);
    throw InterpExc(NOT_VAR); // variable not found
}


//�ڴ���if��ʱ��Ҳ������else��ģ��
// Execute an if statement.
void exec_if()
{
    anonymous_var cond;

    eval_exp(cond); // get if expression.

    if(get_bool_val(cond))   // if true, process target of IF
    {
        // Confirm start of block.

        interp();
    }
    else
    {
        // Otherwise skip around IF block and
        // process the ELSE, if present.

        find_eob(); // find start of next line
        get_token();

        if(tok != ELSE)
        {
            // Restore token if no ELSE is present.
            putback();
            return;
        }
        // Confirm start of block.
        get_token();

        if(tok == IF)
        {
            exec_if();
            return;
        }
        putback();
        interp();
    }
}

// Execute a switch statement.
void exec_switch()
{
    anonymous_var sval, cval;
    int brace;

    eval_exp(sval); // Get switch expression.

    // Check for start of block.
    if(*token != '{')
        throw InterpExc(BRACE_EXPECTED);

    // Record new scope.
    nest_scope_stack.push(local_var_stack.size());

    // Now, check case statements.
    for(;;)
    {
        brace = 1;
        // Find a case statement.
        do
        {
            get_token();
            if(*token == '{') brace++;
            else if(*token == '}') brace--;
        }
        while(tok != CASE && tok != END && brace && tok != DEFAULT);

        // If no matching case found, then skip.
        if(!brace) break;


        if(tok == END) throw InterpExc(SYNTAX);
        if(tok == DEFAULT)
        {
            get_token();
            if(*token != ':')
                throw InterpExc(COLON_EXPECTED);
            do
            {
                interp();
                get_token();
                if(*token == '}')
                {
                    putback();
                    break;
                }
                putback();
                //if(*token == '{') brace++;
                //else if(*token == '}') brace--;
            }
            while(!breakfound && tok != END);

            brace = 1;

            // Find end of switch statement.
            while(brace)
            {
                get_token();
                if(*token == '{') brace++;
                else if(*token == '}') brace--;
            }
            breakfound = false;

            break;

        }

        // Get value of the case statement.
        eval_exp(cval);

        // Read and discard the :
        get_token();

        if(*token != ':')
            throw InterpExc(COLON_EXPECTED);

        // If values match, then interpret.
        if(0 == cmp(cval, sval))
        {

            do
            {
                interp();

                get_token();
                if(*token == '}')
                {
                    putback();
                    break;
                }
                putback();
            }
            while(!breakfound && tok != END && brace);

            brace = 1;

            // Find end of switch statement.
            while(brace)
            {
                get_token();
                if(*token == '{') brace++;
                else if(*token == '}') brace--;
            }
            breakfound = false;

            break;
        }
    }
}

// Execute a while loop.
//ͬ�����do while, ���Ҳ��putback while
void exec_while()
{
    anonymous_var cond;
    char *temp;

    putback(); // put back the while
    temp = prog; // save location of top of while loop

    get_token();
    eval_exp(cond); // check the conditional expression

    if(get_bool_val(cond))
        interp(); // if true, interpret
    else   // otherwise, skip to end of loop
    {
        find_eob();
        return;
    }
    continuefound = false;
    if(!breakfound)
        prog = temp; // loop back to top
    else
    {
        breakfound = false;
        return;
    }
}

// Execute a do loop.

//����: exec_do����������������do��ʱ��Ż����, ���
//��exec_do���õ�ʱ��, do���token�Ѿ�����������,
//��exec_do����Ҫ����Ҫ����ִ�е�ʱ����prog��λ��do, ��ô�͵��ڳ���ʼputbackһ��.
void exec_do()
{
    anonymous_var cond;
    char *temp;

    // Save location of top of do loop.
    putback(); // put back do
    temp = prog;

    get_token(); // get start of loop block

    // Confirm start of block.
    get_token();
    if(*token != '{')
        throw InterpExc(BRACE_EXPECTED);
    putback();

    interp(); // interpret loop

    // Check for break in loop.
    if(breakfound)
    {
        breakfound = false;
        get_token();
        if(tok != WHILE) throw InterpExc(WHILE_EXPECTED);
        eval_exp(cond); // check the loop condition
        return;
    }
    if(continuefound)
    {
        continuefound = false;
        prog = temp;
        return;
    }

    get_token();
    if(tok != WHILE) throw InterpExc(WHILE_EXPECTED);

    eval_exp(cond); // check the loop condition

    // If true loop; otherwise, continue on.

    if(get_bool_val(cond)) prog = temp;
}

// Execute a for loop.
//����for�Ͳ�����while��do while����, ����Ҫ����ѭ����ʱ��λprogָ����, ��Ϊfor
//��λ�Ļ�, ��ʼ��Ҳ���Ÿ�λ��, ����for(int i= 0; i< 12; i++)�����iҲ����0
void exec_for()
{
    anonymous_var cond;
    char *temp, *temp2;
    int paren ;

    //for_local��������ǲ�����for()�ڲ��������±���, �����, �ͻ�����µ�������
    bool for_local = false;

    get_token(); // skip opening (
    get_token();

    if(is_valid_simple_type(tok) || is_struct_type(token) || tok == STRUCT)//��ǰ�����token�Ǹ����͹ؼ���, �����ͻᴥ��һ���ֲ�������
    {
        if(tok != STRUCT)putback();
        nest_scope_stack.push(local_var_stack.size());
        for_local = true;
        decl_local();
    }
    else
    {
        eval_exp(cond); // initialization expression
    }

    //�����decl_local��eval_exp��������token, �Ѿ�����������
    if(*token != ';') throw InterpExc(SEMI_EXPECTED);

    prog++; // get past the ;
    temp = prog;

    for(;;)
    {
        // Get the value of the conditional expression.
        eval_exp(cond);

        if(*token != ';') throw InterpExc(SEMI_EXPECTED);
        prog++; // get past the ;
        temp2 = prog;

        // Find start of for block.
        paren = 1;
        while(paren)
        {
            get_token();
            if(*token == '(') paren++;
            if(*token == ')') paren--;
        }


        // If condition is true, interpret
        //���ڴ�for()���濪ʼinterpret
        //
        if(get_bool_val(cond))
        {
            //continueֻ��interp�����ִ��������, �����������Ӱ��.
            interp();
            //cout << prog << endl;

        }
        else   // otherwise, skip to end of loop
        {
            find_eob();
            if(for_local)
            {
                local_var_stack.resize(nest_scope_stack.top());
                nest_scope_stack.pop();
            }
            return;
        }
        if(breakfound)
        {
            breakfound = false;
            if(for_local)
            {
                local_var_stack.resize(nest_scope_stack.top());
                nest_scope_stack.pop();
            }
            return;
        }
        if(continuefound)
        {
            continuefound = false;
        }


        prog = temp2; // go to increment expression

        // Check for break in loop.



        // Evaluate the increment expression.
        eval_exp(cond);

        prog = temp; // loop back to top
    }

}

// Execute a cout statement.
void exec_cout()
{
    anonymous_var val;

    get_token();
    if(*token != LS) throw InterpExc(SYNTAX);
    do
    {
        get_token();

        if(token_type == STRING)
        {
            // Output a string.
            cout << token;
        }
        else if(tok == ENDL)
        {
            cout << endl;
        }
        else
        {
            //cout << token << " :---" << endl;
            putback();
            eval_exp(val);
            //cout << val.float_value << "<<<" << endl;
            cout_var(val);
        }

        get_token();
    }
    while(*token == LS); //<<

    if(*token != ';') throw InterpExc(SEMI_EXPECTED);
}

// Execute a cin statement.
void exec_cin()
{
    token_ireps vtype;

    get_token();
    if(*token != RS) throw InterpExc(SYNTAX);

    do
    {
        get_token();
        if(token_type != IDENTIFIER)
            throw InterpExc(NOT_VAR);

        vtype = find_var_type(token);
        anonymous_var tmp;
        tmp.var_type = vtype;

        cin_var(tmp);
        assign_var(token, tmp);
        get_token();
    }
    while(*token == RS); //RS ��>>

    if(*token != ';') throw InterpExc(SEMI_EXPECTED);
}


// Find the end of a block.
//#����find_eob���߼�������һ���޸�, ���ⲿ��֤���õ���ȷ
//�����ʼ����{, ��ô�ʹ���һ��block, ����͵���find_eol����һ��;���.
void find_eob()
{
    int brace;

    get_token();
    //cout << token <<  " find_eob" <<endl;
    if(*token != '{')
    {
        putback();
        find_eol();
        return ;
    }

    brace = 1;

    do
    {
        get_token();
        //cout << token <<  " find_eob" <<endl;
        if(*token == '{') brace++;
        else if(*token == '}') brace--;
    }
    while(brace && tok != END);

    if(tok == END) throw InterpExc(UNBAL_BRACES);
}

void find_eol()
{
    do
    {
        get_token();
    }
    while (*token != ';' && tok != END);

    if(tok == END) throw InterpExc(SYNTAX);
}

// Determine if an identifier is a variable. Return
// true if variable is found; false otherwise.
bool is_var(char *vname)
{
	anonymous_var* vp;
	if(get_member_var(vname, vp)) return true;
    // See if vname a local variable.
    if(!local_var_stack.empty())
        for(int i = local_var_stack.size() - 1;
                i >= func_call_stack.top(); i--)
        {
            if(!strcmp(local_var_stack[i].var_name, vname))
                return true;
        }

    // See if vname is a global variable.
    for(unsigned i = 0; i < global_vars.size(); i++)
        if(!strcmp(global_vars[i].var_name, vname))
            return true;

    return false;
}

// Return the type of variable.
token_ireps find_var_type(char *vname)
{
    // First, see if it's a local variable.
    if(!local_var_stack.empty())
        for(int i = local_var_stack.size() - 1;
                i >= func_call_stack.top(); i--)
        {
            if(!strcmp(local_var_stack[i].var_name, vname))
                return local_var_stack[i].value.var_type;
        }

    // Otherwise, try global vars.
    for(unsigned i = 0; i < global_vars.size(); i++)
        if(!strcmp(global_vars[i].var_name, vname))
            return local_var_stack[i].value.var_type;

    return UNDEFTOK;
}

void decl_struct_type()
{
	struct_type type;
	get_token();
	if(strcmp(token, "struct") != 0)
		throw InterpExc(SYNTAX);
	get_token();
	strcpy(type.type_name, token);

	get_token();

	if(*token != '{')
		throw InterpExc(SYNTAX);

	while(true)
	{
		get_token();

		if(strcmp(token, "}") == 0) break;
		var member_var;
		member_var.value.var_type = tok;

		do
		{
			get_token();
			strcpy(member_var.var_name, token);
			init_var(member_var.value); 
			type.data.push_back(member_var);
			get_token();
		}while(*token == ',');

		if(*token != ';') throw InterpExc(SYNTAX);
	}
	get_token();
	if(*token != ';') throw InterpExc(SYNTAX);
	struct_decls.push_back(type);

	dump_struct_type();
}

void dump_struct_type()
{
	char simple_type_names[7][8] = {"bool", "char", "short", "int", "long", "float", "double"};
	int len = struct_decls.size();
	for(int i = 0; i < len; i++)
	{
		printf("struct %s\n", struct_decls[i].type_name);
		printf("{\n");
		struct_type& t = struct_decls[i];
		int sz = t.data.size();
		for(int j = 0; j < sz; j++)
		{
			printf("\t%s %s;\n", simple_type_names[t.data[j].value.var_type - BOOL], t.data[j].var_name);
		}
		printf("};\n");
	}
}

bool is_struct_type(char* type_name)
{
	int sz = struct_decls.size();
	for(int i = 0; i < sz; i++) if(0 == strcmp(type_name, struct_decls[i].type_name)) return true;
	return false;
}


bool get_member_var(char* vname, anonymous_var*& v)
{
	bool b_contains_dots = false;
	char* p = vname;
	while(*p)
	{
		if(*p == '.')
		{
			b_contains_dots = true;
			break;
		}
		p++;
	}
	if(!b_contains_dots) return false;
	char struct_name[64];
	char member_name[64];//����û��̫��ٶ�, �����㾡���������ܳ��ı�������..
	strcpy(struct_name, vname);
	int len = strlen(struct_name);
	for(int i = 0; i < len; i++)
	{
		if(struct_name[i] == '.')
		{
			struct_name[i] = '\0';
			break;
		}
	}
	for(int i = 0; i < len; i++)
	{
		if(vname[i] == '.')
		{
			strcpy(member_name, vname + i + 1);
			break;
		}
	}
	// See if vname a local variable.
	if(!local_var_stack.empty())
		for(int i = local_var_stack.size() - 1;
			i >= func_call_stack.top(); i--)
		{
			if(local_var_stack[i].value.var_type == STRUCT && strcmp(struct_name, local_var_stack[i].var_name) == 0)
			{
				vector<var>& data= local_var_stack[i].value.struct_value.data;
				int len = data.size();
				for(int j = 0; j < len; j++)
				{
					if(0 == strcmp(data[j].var_name, member_name))
					{
						v = &(data[j].value);
						return true;
					}
				}
			}
		}

		// See if vname is a global variable.
		unsigned sz = global_vars.size();
		for(unsigned i = 0; i < sz; i++)
		{
			if(global_vars[i].value.var_type == STRUCT && strcmp(global_vars[i].var_name, struct_name) == 0)
			{
				vector<var>& data = global_vars[i].value.struct_value.data;
				int len = data.size();
				for(int j = 0; j < len; j++)
				{
					if(0 == strcmp(data[j].var_name, member_name))
					{
						v = &(data[j].value);
						return true;
					}
				}
			}
		}

		return false;
}
void init_struct(char* struct_name, anonymous_var& value)
{
	get_token();
	if(*token != '{') throw InterpExc(SYNTAX);
	value.var_type = STRUCT;
	strcpy(value.struct_value.type_name, struct_name);
	var member_var;
	struct_type tp;
	get_struct_type_by_name(struct_name, tp);
	int sz = tp.data.size();
	
	int i = 0;
	do 
	{
		
		eval_exp(member_var.value);
		strcpy(member_var.var_name, tp.data[i].var_name);
		
		//adaptive_assign_var(value.struct_value.data[i].value, member_var);
		value.struct_value.data.push_back(member_var);
		
		get_token();
		if(*token != ',') break;
		i++;
		if(i >= sz)
			throw InterpExc(MORE_MEMBER_THAN_EXPECTED);
	} while (true);
	if(*token != '}') throw InterpExc(SYNTAX);
	get_token();
	if(*token != ';') throw InterpExc(SYNTAX);
}

//ͨ�����������ĳ���ṹ���ʵ��.
bool get_struct_type_by_name(char* struct_name, struct_type& s_type)
{
	int sz = struct_decls.size();
	for(int i = 0; i < sz; i++)
	{
		if(0 == strcmp(struct_name, struct_decls[i].type_name)) 
		{
			s_type = struct_decls[i];
			return true;
		}
	}
	return false;
}


//�������ĿǰֻΪ�ṹ�����, �Ȳ��������int, double�ȵ�����...
bool get_type_by_name(char* vname, char* type_name)
{
	// See if vname a local variable.
	if(!local_var_stack.empty())
		for(int i = local_var_stack.size() - 1;
			i >= func_call_stack.top(); i--)
		{
			if(!strcmp(local_var_stack[i].var_name, vname))
			{
				strcpy(type_name, local_var_stack[i].value.struct_value.type_name);
				return true;
			}
		}

		// See if vname is a global variable.
	for(unsigned i = 0; i < global_vars.size(); i++)
		if(!strcmp(global_vars[i].var_name, vname))
		{
			strcpy(type_name, global_vars[i].value.struct_value.type_name);
			return true;
		}

		return false;
}