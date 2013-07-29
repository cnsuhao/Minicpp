/*********************************************************************
 parser.cpp �����ݹ��½��ؽ������ʽ, ʹ��anonymous_var���ʹ����м���
*********************************************************************/


// Recursive descent parser for integer expressions.
//
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include "mccommon.h"

using namespace std;

// This structure links a library function name
// with a pointer to that function.
struct intern_func_type
{
    char *f_name; // function name
    anonymous_var (*p)();   // pointer to the function
} intern_func[] =
{
    "getchar", call_getchar,
    "putchar", call_putchar,
    "abs", call_abs,
    "rand", call_rand,
    "", 0  // null terminate the list
};

// Keyword lookup table.
// Keywords must be entered lowercase.
// ����ؼ���, ��Ӧһ��tok��id, �����ЩЧ��
struct commands
{
    char command[20];
    token_ireps tok;
} com_table[] =
{
    "if", IF,
    "else", ELSE,
    "for", FOR,
    "do", DO,
    "while", WHILE,
    "bool", BOOL,
    "char", CHAR,
    "short", SHORT,
    "int", INT,
    "long", LONG,
    "float", FLOAT,
    "double", DOUBLE,
    "return", RETURN,
    "switch", SWITCH,
    "break", BREAK,
    "case", CASE,
    "cout", COUT,
    "cin", CIN,
    "endl", ENDL,
    "default", DEFAULT,
    "continue", CONTINUE,
    "true", TRUE,
    "false", FALSE,
	"struct", STRUCT,
    "", END  // mark end of table
};

//eval_exp������֮ǰ��������Ϣ������, ����eval_exp��������, ����ʲô�͵�����Ӧ�Ĵ������.
// Entry point into parser.
void eval_exp(anonymous_var &value)
{
    get_token();

    if(!*token)
    {
        throw InterpExc(NO_EXP);
    }

    //���ڿ�������һ��Ĭ�ϵ�����.
    if(*token == ';')
    {
        value.var_type = BOOL; // empty expression
        value.int_value = false;
        return;
    }

    eval_exp0(value);
    //�����Ѵ�����֮���������token�ٷ���ȥ.
	
    putback(); // return last token read to input stream
}


// Process an assignment expression.
void eval_exp0(anonymous_var &value)
{
	
	//�������ǰ���Ѿ�������get_token��, �����㿴��������ʼ��ʱ��û�е���get_token
	//����ÿһ�ִ���, ��ʵ���������get_token������, ֻ���������Ҫ�ں����ڲ���һ�εĻ�
	// ��������putbackһ�¾Ϳ���
    // temp holds name of var receiving the assignment.
    char temp[MAX_ID_LEN + 1];

    tok_types temp_tok;

    if(token_type == IDENTIFIER)
    {
        if(is_var(token))   // if a var, see if assignment
        {
            strcpy(temp, token);
            temp_tok = token_type;
            get_token();
            if(*token == '=')   // is an assignment
            {
				
				get_token();
				
				if(*token == '{') //ר�Ŵ���һ�¶��ڽṹ��֮��ĸ�ֵ.
				{
					
					putback();
					char type_name[64];
					get_type_by_name(temp, type_name);
					init_struct(type_name, value);
				}
				else
				{
					//printf("putback\n");
					//printf("%s tttttt\n", token);
					//putback();
					eval_exp0(value); // get value to assign
				}
				assign_var(temp, value); // assign the value

                return;
            }
            else   // not an assignment
            {
                putback(); // restore original token
                strcpy(token, temp);
                token_type = temp_tok;
            }
        }
    }
    eval_exp1(value);
}

// Process relational operators.
void eval_exp1(anonymous_var &value)
{
	
    anonymous_var partial_value;
    char op;
    char relops[] =
    {
        LT, LE, GT, GE, EQ, NE, 0
    };

    eval_exp2(value);

    op = *token;
    if(strchr(relops, op))
    {
        get_token();
        eval_exp2(partial_value);

        switch(op)   // perform the relational operation
        {
        case LT:
        {
            int res = cmp(value, partial_value);
            value.var_type = BOOL;
            value.int_value = res < 0;
            break;
        }
        case LE:
        {
            int res = cmp(value, partial_value);
            value.var_type = BOOL;
            value.int_value = res <= 0;
            break;
        }
        case GT:
        {
            int res = cmp(value, partial_value);
            value.var_type = BOOL;
            value.int_value = res > 0;
            break;
        }
        case GE:
        {
            int res = cmp(value, partial_value);
            value.var_type = BOOL;
            value.int_value = res >= 0;
            break;
        }
        case EQ:
        {
            int res = cmp(value, partial_value);

            value.var_type = BOOL;
            value.int_value = (res == 0);
            break;
        }
        case NE:
        {
            int res = cmp(value, partial_value);
            value.var_type = BOOL;
            value.int_value = !(res == 0);
            break;
        }
        }
    }
}

// Add or subtract two terms.
void eval_exp2(anonymous_var &value)
{
	
    char  op;
    anonymous_var partial_value;
    char okops[] =
    {
        '(', INC, DEC, '-', '+', 0
    };

    eval_exp3(value);

    while((op = *token) == '+' || op == '-')
    {
        get_token();

        if(token_type == DELIMITER &&
                !strchr(okops, *token))
            throw InterpExc(SYNTAX);

        eval_exp3(partial_value);



        switch(op)   // add or subtract
        {
        case '-':
        {
            value = sub(value, partial_value);
            break;
        }
        case '+':
        {
            value = add(value, partial_value);
            break;
        }
        }
    }
}

// Multiply or divide two factors.
void eval_exp3(anonymous_var &value)
{
    char  op;
    anonymous_var partial_value;
    char okops[] =
    {
        '(', INC, DEC, '-', '+', 0
    };

    eval_exp4(value);

    while((op = *token) == '*' || op == '/'
            || op == '%')
    {
        get_token();

        if(token_type == DELIMITER &&
                !strchr(okops, *token))
            throw InterpExc(SYNTAX);

        eval_exp4(partial_value);

        switch(op)   // mul, div, or modulus
        {
        case '*':
        {
            value = mul(value, partial_value);
            break;
        }

        case '/':
        {
            //�жϳ����쳣�ڳ�����������
            value = div(value, partial_value);
            break;
        }
        case '%':
        {
            anonymous_var tmp = div(value, partial_value);
            tmp = mul(partial_value, tmp);
            value = sub(value, tmp);
            break;
        }
        }
    }
}

// Is a unary +, -, ++, or --.
void eval_exp4(anonymous_var &value)
{
    char  op;
    char temp;

    op = '\0';
    if(*token == '+' || *token == '-' ||
            *token == INC || *token == DEC)
    {
        temp = *token;
        op = *token;
        get_token();
        value = find_var(token); //����Ҳ����ṹ��ĳ�Ա����..
        //����ǰ׺++, --Ҫ�ѱ仯��Ӧ����������.
        if(temp == INC)
        {
            anonymous_var tmp_var;
            tmp_var.int_value = 1;
            tmp_var.var_type = value.var_type;
            value = add(value, tmp_var);
            assign_var(token, value); //���tokenҲ�����˶Խṹ���Ա�����Ĵ���.
            get_token();
            return;
        }
        if(temp == DEC)
        {
            anonymous_var tmp_var;
            tmp_var.int_value = 1;
            tmp_var.var_type = value.var_type;
            value = add(value, tmp_var);
            assign_var(token, value);
            get_token();
            return;
        }
    }

    eval_exp5(value);
    if(op == '-')
    {
        neg_var(value);
    }
}

// Process parenthesized expression.
void eval_exp5(anonymous_var &value)
{
    if((*token == '('))
    {
        get_token();

        eval_exp0(value); // get subexpression

        if(*token != ')')
            throw InterpExc(PAREN_EXPECTED);
        get_token();
    }
    else
        atom(value);
}

// Find value of number, variable, or function.
//����һ��������, ��������, ��Ŀǰֻ����***.***����ʽ, �������ѧ������.
void atom(anonymous_var &value)
{
    int i;
    char temp[MAX_ID_LEN + 1];

    switch(token_type)
    {
    case IDENTIFIER:
        i = internal_func(token);
        if(i != -1)
        {
            // Call "standard library" function.
            value = ((*intern_func[i].p)());
        }
        else if(find_func(token))
        {
            // Call programmer-created function.
            call();
            value = ret_value;//Ŀǰ������ֻ֧��int����ֵ
        }
        else
        {

            //�����ﴦ���˺�׺++, --
            value = find_var(token); // get var's value
            strcpy(temp, token); // save variable name

            // Check for ++ or --.
            get_token();
            if(*token == INC || *token == DEC)
            {
                anonymous_var tmp_val = find_var(temp);
                value = tmp_val;
                if(*token == INC)
                {
                    anonymous_var one;
                    one.int_value = 1;
                    one.var_type = tmp_val.var_type;
                    tmp_val = add(tmp_val, one);
                    assign_var(temp, tmp_val);
                }
                else
                {
                    anonymous_var one;
                    one.int_value = 1;
                    one.var_type = tmp_val.var_type;
                    tmp_val = sub(tmp_val, one);
                    assign_var(temp, tmp_val);
                }

            }
            else putback();
        }

        get_token();
        return;
    case NUMBER: // is numeric constant
        //����Ը�����������������ж�
		//printf("in number %s\n", token);
        if(strchr(token, '.'))
        {

            value.var_type = DOUBLE;
            value.float_value = atof(token);
        }
        else
        {
            value.var_type = INT;
            value.int_value = atoi(token);
        }
        get_token();

        return;

        //char constant
    case DELIMITER: // see if character constant
        if(*token == '\'')
        {
            value.var_type = CHAR;
            value.int_value = *prog;
            prog++;
            if(*prog != '\'')
                throw InterpExc(QUOTE_EXPECTED);

            prog++;
            get_token();

            return ;
        }
        if(*token == ')') return; // process empty expression
        else throw InterpExc(SYNTAX);  // otherwise, syntax error
    case KEYWORD:
    {
        if(0 == strcmp(token, "true"))
        {
            //cout << "jackieddddd" << endl;
            value.var_type = BOOL;
            value.int_value = 1;
        }
        else if(0 == strcmp(token, "false"))
        {
            value.var_type = BOOL;
            value.int_value = 0;
        }
        else
        {
            throw InterpExc(SYNTAX);
        }
        get_token();
        break;
    }
    default:
        throw InterpExc(SYNTAX); // syntax error
    }
}

// Display an error message.
void sntx_err(error_msg error)
{
    char *p, *temp;
    int linecount = 0;

    static char *e[] = //�������ʾ��Ϣ, �Ǹ�ͷ�ļ����涨��Ĵ���flag˳��һ�µ�
    {
        "Syntax error",
        "No expression present",
        "Not a variable",
        "Duplicate variable name",
        "Duplicate function name",
        "Semicolon expected",
        "Unbalanced braces",
        "Function undefined",
        "Type specifier expected",
        "Return without call",
        "Parentheses expected",
        "While expected",
        "Closing quote expected",
        "Division by zero",
        "{ expected (control statements must use blocks)",
        "Colon expected",
        "Unsupported type yet",
		"More struct member that expected"
    };

    // Display error and line number.
    cout << "\n" << e[error];
    p = p_buf;
    while(p != prog)   // find line number of error
    {
        p++;
        if(*p == '\r')
        {
            linecount++;
        }
    }
    cout << " in line " << linecount << endl;

    temp = p;
    while(p > p_buf && *p != '\n') p--;

    // Display offending line.
    while(p <= temp)
        cout << *p++;

    cout << endl;
}

// Get a token.
tok_types get_token()
{

    char *temp;

    token_type = UNDEFTT;
    tok = UNDEFTOK;

    temp = token;
    *temp = '\0';

    // Skip over white space.
    while(isspace(*prog) && *prog) ++prog;

    // Skip over newline.
    while(*prog == '\r')
    {
        ++prog;
        ++prog;
        // Again, skip over white space.
        while(*prog && isspace(*prog)) ++prog;
    }

    // Check for end of program.
    if(*prog == '\0')
    {
        *token = '\0';
        tok = END;
        return (token_type = DELIMITER);
    }

    // Check for block delimiters.
    if(strchr("{}", *prog))
    {
        *temp = *prog;
        temp++;
        *temp = '\0';
        prog++;
        return (token_type = BLOCK);
    }

    // Look for comments.
    if(*prog == '/')
        if(*(prog + 1) == '*') // is a /* comment
        {
            prog += 2;

            //���ѭ���ܸ���
            do   // find end of comment
            {
                while(*prog != '*') prog++;
                prog++;
            }
            while (*prog != '/');
            prog++;
            return (token_type = DELIMITER);
        }
        else if(*(prog + 1) == '/')   // is a // comment
        {
            prog += 2;
            // Find end of comment.
            while(*prog != '\r' && *prog != '\0') prog++;
            if(*prog == '\r') prog += 2;
            return (token_type = DELIMITER);
        }

    // Check for double-ops.
	//<By�ŷ�ѩ>����û�д��� *= �������...
    if(strchr("!<>=+-", *prog))
    {
        switch(*prog)
        {
        case '=':
            if(*(prog + 1) == '=')
            {
                prog++;
                prog++;
                *temp = EQ;
                temp++;
                *temp = EQ;
                temp++;
                *temp = '\0';
            }
            break;
        case '!':
            if(*(prog + 1) == '=')
            {
                prog++;
                prog++;
                *temp = NE;
                temp++;
                *temp = NE;
                temp++;
                *temp = '\0';
            }
            break;
        case '<':
            if(*(prog + 1) == '=')
            {
                prog++;
                prog++;
                *temp = LE;
                temp++;
                *temp = LE;
            }
            else if(*(prog + 1) == '<')
            {
                prog++;
                prog++;
                *temp = LS;
                temp++;
                *temp = LS;
            }
            else
            {
                prog++;
                *temp = LT;
            }
            temp++;
            *temp = '\0';
            break;
        case '>':
            if(*(prog + 1) == '=')
            {
                prog++;
                prog++;
                *temp = GE;
                temp++;
                *temp = GE;
            }
            else if(*(prog + 1) == '>')
            {
                prog++;
                prog++;
                *temp = RS;
                temp++;
                *temp = RS;
            }
            else
            {
                prog++;
                *temp = GT;
            }
            temp++;
            *temp = '\0';
            break;
        case '+':
            if(*(prog + 1) == '+')
            {
                prog++;
                prog++;
                *temp = INC;
                temp++;
                *temp = INC;
                temp++;
                *temp = '\0';
            }
            break;
        case '-':
            if(*(prog + 1) == '-')
            {
                prog++;
                prog++;
                *temp = DEC;
                temp++;
                *temp = DEC;
                temp++;
                *temp = '\0';
            }
            break;
        }

        if(*token) return(token_type = DELIMITER);
    }

    // Check for other delimiters.
    if(strchr("+-*^/%=;:(),'", *prog))
    {
        *temp = *prog;
        prog++;
        temp++;
        *temp = '\0';
        return (token_type = DELIMITER);
    }

    // Read a quoted string.
    if(*prog == '"')
    {
		//printf("in string jackie\n");
        prog++;
        while(*prog != '"' && *prog != '\r' && *prog)
        {
            // Check for \n escape sequence.
            if(*prog == '\\')
            {
                if(*(prog + 1) == 'n')
                {
                    prog++;
                    *temp++ = '\n';
                }
            }
            else if((temp - token) < MAX_T_LEN)
                *temp++ = *prog;

            prog++;
        }
        if(*prog == '\r' || *prog == 0)
            throw InterpExc(SYNTAX);
        prog++;
        *temp = '\0'; //temp��token�Ļ���ָ��..
		//printf("%s string\n", token);
        return (token_type = STRING);
    }

    // Read an integer number, or float
    //�������ڻ�û����ṹ�����, ����ֱ�������ж�'.'���ǿ��Ե�, ������������, Ҫ�ǵ�~
    if(isdigit(*prog) || *prog == '.')
    {
        while(isdigit(*prog) || *prog == '.') //!isdelim(*prog), �����ԭ�����ж�..
        {
            if((temp - token) < MAX_ID_LEN)
                *temp++ = *prog;
            prog++;
        }
        *temp = '\0';
        return (token_type = NUMBER);
    }

    // Read identifier or keyword.
    if(isalpha(*prog))
    {
        while(!isdelim(*prog))
        {
            if((temp - token) < MAX_ID_LEN)
                *temp++ = *prog;
            prog++;
        }
        token_type = TEMP;
    }

    *temp = '\0';

    // Determine if token is a keyword or identifier.
    if(token_type == TEMP)
    {
        tok = look_up(token); // convert to internal form
        if(tok) token_type = KEYWORD; // is a keyword
        else token_type = IDENTIFIER;
    }
	//���ǲ�Ҫ�������������, ��Ϊstruct�ؼ��ֱ����tok����STRUCT
	//if(token_type == IDENTIFIER && is_struct_type(token)) 
	//{
	//	//printf("jackie struct %s\n", token);
	//	tok = STRUCT;
	//}
	
    // Check for unidentified character in file.
    if(token_type == UNDEFTT)
        throw InterpExc(SYNTAX);

    return token_type;
}

// Return a token to input stream.
void putback()
{
    char *t;
    t = token;
    for(; *t; t++) prog--;
}

// Look up a token's internal representation in the
// token table.
token_ireps look_up(char *s)
{
    int i;

    // See if token is in table.
    for(i = 0; *com_table[i].command; i++)
    {
        if(!strcmp(com_table[i].command, s))
            return com_table[i].tok;
    }

    return UNDEFTOK; // unknown command
}

// Return index of internal library function or -1 if
// not found.
int internal_func(char *s)
{
    int i;

    for(i = 0; intern_func[i].f_name[0]; i++)
    {
        if(!strcmp(intern_func[i].f_name, s))  return i;
    }
    return -1;
}

// Return true if c is a delimiter.
bool isdelim(char c)
{
    if(strchr(" !:;,+-<>'/*%^=()", c) || c == 9 ||
            c == '\r' || c == 0) return true;
    return false;
}




