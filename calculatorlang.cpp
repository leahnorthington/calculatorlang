// Free free to use constructs #included below.

#include <iostream>
#include <cstdlib>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <stack>
#include "tokenizer.cpp"

/*****************************************************
Token	        Lexeme(s)
id	            [a-zA-Z_][a-zA-Z0-9_]*
plus_op	        +
negation_op	    -
mult_op	        *
div_op	        /
greater_sign	>
less_sign	    <
greateq_sign	>=
lesseq_sign	    <=
equal_sign	    =
or_op	        or
and_op	        and
not_op	        not
bool_true	    #t
bool_false	    #f
int	            [0-9]+
float	        [0-9]+.[0-9]+
open_paren	    (
closed_paren	)

*************INSERT DISCLOSURES HERE *********
******************************************************/

using namespace std;

/*****************
DEFINE CONSTANTS
******************/

typedef int NUMBER;
typedef int NAME;
const int  NAMELENG = 25;      /* Maximum length of a name */
const int  MAXNAMES = 500;     /* Maximum number of different names */
const int  MAXINPUT = 40000;     /* Maximum length of an input */
const char*   PROMPT = "calc_interpreter> ";
const char*   PROMPT2 = "--> ";
const char  COMMENTCHAR = '%';
const char  ENDOFINPUT = '$';   // use as sentinel to mark end of input
const int   TABCODE = 9;        /* in ASCII */

								/****************
								DECLARE GLOBAL STUFF
								******************/

int stringToInt(string);
string intToString(int);
string charToString(char);

char userinput[MAXINPUT];//array to store the user input
int  inputleng, pos; // indices for tokenizer

int  enterIfScope = 0; // antiquated ... holds input buffer until matching endif
string currentOutput = "Output: No statements";

vector<string> ids; //vector of ids of input
vector<string> lexemes; //vector of lexemes of input

string charToString(char c)
{
	string s(1, c);
	return s;
}

/**********************************
* 
* VALUE CLASS FOR SEMANTIC ANALYSIS
* 
**********************************/
class Value
{
public:
	int integer;
	bool boolean;
	float floating;
    bool isInt;
    bool isFloat;
    bool isBool;

    Value();
    void setInt(int integer);
    void setFloat(float floating);
    void setBool(bool boolean);
    void setisBool(bool isBool);
    void setisInt(bool isInt);
    void setisFloat(bool isFloat);
    void operator=(Value rhsobj);
}; //END class Value

void printValue(Value v);
Value parse(int &pos);
Value parseE(int &pos);

Value::Value()
{
	this->integer = -1;
	this->boolean = -1;
	this->floating = -1.0;
    this-> isBool = 0;
    this-> isFloat = 0;
    this-> isInt = 0;
} //END function Value::Value()
	
void Value::setInt(int i)
{
    this->integer = i;
    this->isInt = true;
} //END function Value::setInt()

void Value::setBool(bool b)
{
    this->boolean = b;
    this->isBool = true;
} //END function Value::setBool()

void Value::setFloat(float f)
{
    this->floating = f;
    this->isFloat = true;
} //END function Value::setFloat()

void Value::setisBool(bool b)
{
    this->isBool = b;
} //END function Value::setisBool()

void Value::setisInt(bool b)
{
    this->isInt = b;
} //END function Value::setisInt()

void Value::setisFloat(bool b)
{
    this->isFloat = b;
} //END function Value::setisFloat()

void printValue(Value v)
{
    if (v.isBool)
    {
        cout << to_string(v.boolean) << endl;
    }
    else if (v.isFloat)
    {
        cout << to_string(v.floating) << endl;
    }
    else if (v.isInt)
    {
        cout << to_string(v.integer) << endl;
    }
    else
    {
        cout << "Value object is empty. No value to print.\n";
    }
} //END function Value::printValue()
    
void Value::operator=(Value rhsobj)
{
    this->setBool(rhsobj.boolean);
    this->setFloat(rhsobj.floating);
    this->setInt(rhsobj.integer);
    this->setisBool(rhsobj.isBool);
    this->setisFloat(rhsobj.isFloat);
    this->setisInt(rhsobj.isInt);
} //END overloaded assignment operator for Value class

/*********************************
LOW-LEVEL HELPER METHODS FOR READING INPUT
*********************************/
void nextchar(char& c) // pulls next char
{
	scanf("%c", &c);
	if (c == COMMENTCHAR)
	{
		while (c != '\n')
			scanf("%c", &c);
	}
} //END function nextchar()

// skips over blanks
int skipblanks(int p)
{
	while (userinput[p] == ' ')
		++p;
	return p;
} //END function skipblanks()

int isDelim(char c) // checks char for particular delimiters
{
	return ((c == '(') || (c == ')') || (c == ' ') || (c == COMMENTCHAR) || (c == '\n'));
} //END function isDelim()

int matches(int s, int leng, char* nm) // lookahead checks input for matching string. Helps to check for keywords.
{
	int i = 0;
	int tester = 0;

	while (i < leng)
	{

		if (userinput[s] != nm[i]) {
			return 0;
		}
		++i;
		++s;
	}
	if (!isDelim(userinput[s]))
		return 0;

	return 1;
} //END function matches(int, int, char)

int matches(int s, int leng, char* nm, bool testing) // lookahead checks input for matching string. Helps to check for keywords.
{
	int i = 0;
	int tester = 0;

	while (i < leng)
	{
		if (userinput[s] != nm[i]) {
			return 0;
		}
		++i;
		++s;
	}


	return 1;
} //END function matches(int, int, char, bool)


 // assures that scanf waits until matching paren {  } is input. Tokenizer will not begin until a match is found.
 // This function is not necessary but allows for easy input of large blocks of code
void readParens()
{
	int parencnt; /* current depth of parentheses */
	char c = ' ';
	parencnt = 1; // '(' just read
	do
	{
		if (c == '\n')
			cout << PROMPT2;
		nextchar(c);
		pos++;
		if (pos == MAXINPUT)
		{
			cout << "User input too long\n";
			exit(1);
		}
		if (c == '\n') {
			userinput[pos] = ' ';
		}
		else
			userinput[pos] = c;

		if (c == '(') // holds the prompt for blocks of code
		{
			++parencnt;
		}	
		if (c == ')')
		{
			parencnt--;		
		}
	} while (parencnt != 0);
} //END function readParens()

// reads input from console and stores in global userinput[]
void readInput()
{
	char  c;
	cout << PROMPT;
	pos = 0;
	do
	{
		++pos;
		if (pos == MAXINPUT)
		{
			cout << "User input too long\n";
			exit(1);
		}
		nextchar(c);
		if (c == '\n')
			userinput[pos] = ' ';// or space
		else
			userinput[pos] = c;
			//cout << "array now has: " << userinput[pos] << "\n";
					
		if (userinput[pos] == '(')  // if pos >6 and matches(pos-1,6,"define")
			readParens();// readDef  ... in def until read key word end
	} while (c != '\n');
	inputleng = pos + 1;
	userinput[pos+1] = ENDOFINPUT; // sentinel
								 // input buffer is now filled, reset pos to 1 (later in reader()) and begin parse
} //END function readInput()


// Reads input until end
void reader()
{
	do
	{
		readInput();
		pos = skipblanks(1); // reset pos for tokenizer
	} while (pos > inputleng-1); // ignore blank lines
} //END function reader()

/*********************************
END OF LOW-LEVEL HELPER METHODS FOR READING INPUT
*********************************/

/**************************************************
**********   Some Tokenizer Helpers below  *********
* Feel free to insert tokenizer code here (or in other file)*
***************************************************/
// some helpers for the lexer, that you may or may not want to use ...

// checks to see if user input at index pos is digit
// helps lexers
int isDigit(int pos)
{
	if ((userinput[pos] < '0') || (userinput[pos] > '9'))
		return 0;
	else
		return 1;
} //END function isDigit()

// checks to see if user input at index pos is char
int isChar(int pos)
{
	if ((userinput[pos] >= 'a') && (userinput[pos] <= 'z') ||
		(userinput[pos] >= 'A') && (userinput[pos] <= 'Z') ||
		userinput[pos] == '_')
		return 1;
	else
		return 0;
} //END function isChar()

int isNewLine(int pos) {

	pos = skipblanks(pos);
	if (userinput[pos] == '\n') {
		return 1;
	}
	else {
		return 0;
	}
} //END function isNewLine()

int isParen(int pos)
{
	if ((userinput[pos] == '(') || (userinput[pos] == ')'))
		return 1;
	else
		return 0;
} //END function isParen()

int isArithmeticOp(int pos)
{
    if ((userinput[pos] == '+' || userinput[pos] == '-' || userinput[pos] == '*' || userinput[pos] == '/'))
        return 1;
    else   
        return 0;
} //END function isArithmeticOp()

int isRelationalOp(int pos)
{
    if ((userinput[pos] == '>' || userinput[pos] == '<' || userinput[pos] == '='))
        return 1;
    else   
        return 0;
} //END function isRelationalOp()

int isBool(int pos)
{
    if (userinput[pos] == '#' && (userinput[pos + 1] == 't'
							|| userinput[pos + 1] == 'f'))
        return 1;
    else   
        return 0;
} //END function isBool

int isEOI(int pos)
{
    if (userinput[pos] == '$')
        return 1;
    else   
        return 0;
} //END function isEOI()

int isSpace(int pos)
{
    if (userinput[pos] == ' ')
        return 1;
    else   
        return 0;
} //END function isSpace()

void tokenizeDigit(int &pos)
{
	bool hasDot = false;
	ids.push_back("int");
    string digits = "";
    while(isDigit(pos) == 1 || userinput[pos] == '.')
    {
		if (userinput[pos] == '.')
        {
			if (!hasDot)
			{
				ids.pop_back();
				ids.push_back("float");
				hasDot = true;
			}
			/*if (hasDot)
			{
				tokenizeDigit(pos);
			}*/
		}
        digits.push_back(userinput[pos]);
		pos++;
	}
	pos--; //go back to get the close paren
	lexemes.push_back(digits);
} //END function tokenizeDigit()

void tokenizeChar(int &pos)
{
    string chars = "";
    while(isChar(pos) == 1)
    {
        chars.push_back(userinput[pos]);
        pos++;
    }
    if (chars == "or" || chars == "OR")
    {
        ids.push_back("or_op");
    }
    else if (chars == "not" || chars == "NOT")
    {
        ids.push_back("not_op");
    }
    else if (chars == "and" || chars == "AND")
    {
        ids.push_back("and_op");
    }
    else
    {
        throw string("Tokenizer Error: characters entered are not in input alphabet.");
    }
    lexemes.push_back(chars);
} //END function tokenizeChar()

void tokenizeRelateOp(int &pos)
{
    string relate = "";
    while(isRelationalOp(pos) == 1)
    {
        if (userinput[pos] == '>')
        {
			if (userinput[pos+1] == '=')
			{
				ids.push_back("greatereq_sign");
				relate.push_back(userinput[pos]);
				pos++;
			}
			else {ids.push_back("greater_sign");}
        }
        else if (userinput[pos] == '<')
        {
            if (userinput[pos+1] == '=')
			{
				ids.push_back("lesseq_sign");
				relate.push_back(userinput[pos]);
				pos++;
			}
			else {ids.push_back("less_sign");}
        }
		else if (userinput[pos] == '=')
        {
			ids.push_back("eq_sign");
        }
        relate.push_back(userinput[pos]);
        pos++;
    }
    lexemes.push_back(relate);
} //END function tokenizeRelateOp()

void tokenizeArithOp(int &pos)
{
    string arith = "";
    while(isArithmeticOp(pos) == 1)
    {
        if (userinput[pos] == '+')
        {
            ids.push_back("plus_op");
        }
		if (userinput[pos] == '-')
        {
            ids.push_back("negation_op");
		}
        if (userinput[pos] == '*')
        {
            ids.push_back("mult_op");
        }
        if (userinput[pos] == '/')
        {
            ids.push_back("div_op");
        }
        arith.push_back(userinput[pos]);
        pos++;
    }
    lexemes.push_back(arith);
} //END function tokenizeArithOp()

void tokenizeBool(int &pos)
{
    string rBool = "";
    rBool.push_back(userinput[pos]); //the #
    if (userinput[pos+1] == 't')
    {
        ids.push_back("bool_true");
    }
    if (userinput[pos+1] == 'f')
    {
        ids.push_back("bool_false");
    }
	rBool.push_back(userinput[pos+1]); //the t or f
	pos++;
	lexemes.push_back(rBool);
} //END function tokenizeBool()

void tokenizeEOI(int &pos)
{
	string endChar = "";
	endChar.push_back(userinput[pos]);
	ids.push_back("end_of_input");
	lexemes.push_back(endChar);
} //END function tokenizeEOI()

void tokenizeParen(int &pos)
{
	string p = "";
	p.push_back(userinput[pos]);
	if (p == "(") 
	{
		ids.push_back("open_paren");
		lexemes.push_back(p);
	}	
	else if (p == ")")
	{
		ids.push_back("closed_paren");
		lexemes.push_back(p);			
	}
} //END function tokenizeParen()

//Debugging function, prints out the first 50 characters of UserInput
void printUserInput() {
	cout << "START" << endl;
	for (int i = 0; i < 50; i++) {
		cout << userinput[i];
	}
	cout << endl << "END" << endl;
} //END function printUserInput()

void detect()
{
    for (int i = 1; i <= inputleng; i++)
    {
		if (isDigit(i) == 1)
        {
            tokenizeDigit(i);
        }
        else if (isChar(i) == 1)
        {
            tokenizeChar(i);
        }
        else if (isArithmeticOp(i) == 1)
        {
            tokenizeArithOp(i);
        }
        else if (isRelationalOp(i) == 1)
        {
            tokenizeRelateOp(i);
        }
        else if (isBool(i) == 1)
        {
			tokenizeBool(i);
		}
		else if (isEOI(i) == 1)
		{
			tokenizeEOI(i);
		}
		else if (isParen(i) == 1)
		{
			tokenizeParen(i);
		}
		else if (isSpace(i) == 1)
		{
			skipblanks(i);
		}
		else
        {
			cout << "This input character is not in the alphabet!\n";
            cout << "The input characeter is: " << userinput[i] << "\n";
            string error = "Tokenizer Error: invalid input!";
            throw string(error);   
        }
    }
} //END function detect()

void printTokens()
{                
	cout << "\nYou have created " << lexemes.size() << " tokens.\n";	
	for (int i = 0; i < ids.size(); i++)
    {
		cout << "\nThe token " << lexemes[i];
        cout << " is a " << ids[i] << ".\n";
    }
} //END function printTokens

void cleanVecs()
{
	ids.clear();
	lexemes.clear();
} //END function clearVecs()

/**************************************
****** BEGIN PARSER CODE **************
**************************************/

/*************************************
BNF Grammar for Calculator:
program -> E
(1) E -> (Op E E)
(2) E -> T
(3) E -> F
(4) E -> B
(5) E -> empty string
(6) Op -> or|and|=|>|>=|<|<=|+|-|/|*|not
(7) T -> 0|1|....|9
(8) F -> T.T
(9) B -> #f
(10) B -> #t
**************************************/
Value eval(string op, Value val1, Value val2)
{
    Value result = val1; //string representation
    if (val1.isFloat && val2.isFloat)
    {
        if (op == "+")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating + val2.floating);
        }
        else if (op == "-")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating - val2.floating);
        }
        else if (op == "*")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating * val2.floating);
        }
        else if (op == "/")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating / val2.floating);
        }
        else if (op == "<")
        {
            result.setisBool(true);
            if (val1.floating < val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "<=")
        {
            result.setisBool(true);            
            if (val1.floating <= val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">")
        {
            result.setisBool(true); 
            if (val1.floating > val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">=")
        {
            result.setisBool(true); 
            if (val1.floating >= val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "=")
        {
            result.setisBool(true); 
            if (val1.floating == val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
    }
    else if (val1.isInt && val2.isInt)
    {
        if (op == "+")
        {
            result.setisInt(true);
            result.setInt(val1.integer + val2.integer);
        }
        else if (op == "-")
        {
            result.setisInt(true);
            result.setInt(val1.integer - val2.integer);
        }
        else if (op == "*")
        {
            result.setisInt(true);
            result.setInt(val1.integer * val2.integer);
        }
        else if (op == "/")
        {
            result.setisInt(true);
            result.setInt(val1.integer / val2.integer);
        }
        else if (op == "<")
        {
            result.setisBool(true); 
            if (val1.integer < val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "<=")
        {
            result.setisBool(true); 
            if (val1.integer <= val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">")
        {
            result.setisBool(true); 
            if (val1.integer > val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">=")
        {
            result.setisBool(true); 
            if (val1.integer >= val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "=")
        {
            result.setisBool(true); 
            if (val1.integer == val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
    }
    else if (val1.isInt && val2.isFloat)
    {
        if (op == "+")
        {
            result.setisFloat(true);
            result.setFloat(val1.integer + val2.floating);
        }
        else if (op == "-")
        {
            result.setisFloat(true);            
            result.setFloat(val1.integer - val2.floating);
        }
        else if (op == "*")
        {
            result.setisFloat(true);
            result.setFloat(val1.integer * val2.floating);
        }
        else if (op == "/")
        {
            result.setisFloat(true);
            result.setFloat(val1.integer / val2.floating);
        }
        else if (op == "<")
        {
            result.setisBool(true); 
            if (val1.integer < val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "<=")
        {
            result.setisBool(true); 
            if (val1.integer <= val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">")
        {
            result.setisBool(true); 
            if (val1.integer > val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">=")
        {
            result.setisBool(true); 
            if (val1.integer >= val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "=")
        {
            result.setisBool(true); 
            if (val1.integer == val2.floating)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
    }
    else if (val1.isFloat && val2.isInt)
    {
        if (op == "+")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating + val2.integer);
        }
        else if (op == "-")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating - val2.integer);
        }
        else if (op == "*")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating * val2.integer);
        }
        else if (op == "/")
        {
            result.setisFloat(true);
            result.setFloat(val1.floating / val2.integer);
        }
        else if (op == "<")
        {
            result.setisBool(true); 
            if (val1.floating < val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "<=")
        {
            result.setisBool(true); 
            if (val1.floating <= val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">")
        {
            result.setisBool(true); 
            if (val1.floating > val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == ">=")
        {
            result.setisBool(true); 
            if (val1.floating >= val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "=")
        {
            result.setisBool(true); 
            if (val1.floating == val2.integer)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
    }     
    else if (val1.isBool == 1 && val2.isBool == 1)
    {
        if (op == "and")
        {
            result.setisBool(true);             
            if (val1.boolean == val2.boolean)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
        else if (op == "or")
        {
            result.setisBool(true); 
            if (val1.boolean == 1 || val2.boolean == 1)
            {
                result.setBool(true);
            }
            else
            {
                result.setBool(false);
            }
        }
    }
    else if (val1.isBool == 1 || val2.isBool == 1)
    {
        if (op == "not")
        {
            if (val1.isBool)
            {
                result.setisBool(true); 
                result.boolean = !val1.boolean;
            }
            else if (val2.isBool)
            {
                result.setisBool(true); 
                result.boolean = !val2.boolean;
            }
        }
    }
	return result;
} //END function eval()

bool parseOp(int &pos){
    bool isValid = true;

        if(ids.at(pos) == "int" || ids.at(pos) == "float" ||
           ids.at(pos) == "bool_true" || ids.at(pos) == "bool_false" ||
           ids.at(pos) == "end_of_input"){
            
            isValid = false;
            string error = "Parser Error: incorrect syntax!";
            throw string(error);
        }
    return isValid;
} //END function parseOp()

void parseCParens(int &pos){
    if(ids.at(pos) != "closed_paren"){
        cout << "\nSyntax error. Expected closed parens but got "
        << ids.at(pos) << endl;
    }
} //END function parseCParens()

Value parseInt(int pos)
{
    Value v;    
    v.setisInt(true);
    int test = stoi(lexemes.at(pos));
    /*if (lexemes.at(pos-1) == "-" && lexemes.at(pos-2) != "(")
    {
        cout << "you have tried to declare a negative number\n";
        test *= -1;
    }*/
    v.setInt(test);
	return v;
} //END function parseInt()

Value parseFloat(int pos)
{
    Value v;    
    v.setisFloat(true);
	float test = stof(lexemes.at(pos));
    v.setFloat(test);
	return v;
} //END function parseFloat()

Value parseBool(int pos)
{
    Value v;
    v.setisBool(true);    
    if (ids.at(pos) == "bool_true")
    {
        v.setBool(true);
    }
    else 
    {
        v.setBool(false);
    }
	return v;
} //END function parseBool()

Value parseE(int &pos)
{
    if(ids.at(pos) == "int")
	{
        return parseInt(pos);
	}
	else if(ids.at(pos) == "float")
	{
        return parseFloat(pos);
	}
	else if(ids.at(pos) == "bool_true" || ids.at(pos) == "bool_false")
	{
        return parseBool(pos);
    }
	else if (ids.at(pos) == "open_paren")
	{
		return parse(pos);
	}
	else
	{
        Value e;
        return e;
        throw string("Parser Error: invalid syntax.\n");        
	}
} //END function parseE()

Value parse(int &pos){
    Value v1;
	Value v2;
	string op;
	if (ids.at(pos) == "open_paren")
	{
        pos++;
		if(parseOp(pos))
		{				
            op = lexemes.at(pos);
            pos++;
		}
		else
        {
            Value v;
            return v;    
        }

        v1 = parseE(pos);
		pos++;
        v2 = parseE(pos);  
		pos++;
		//parseCParens(pos);
    }
	else if (ids.at(pos) == "closed_paren")
	{
        Value f = eval(op, v1, v2);
        return f;
	}
	else if(ids.at(pos) == "EOI")
	{
        Value e;
        return e;
	}
	else
	{
        if (lexemes.at(pos) != "quit")
        {
            cout << "EOI parse error\n";
        }
        //throw string("Parser Error: invalid syntax.\n");        
    }
    Value final = eval(op, v1, v2);
    return final;
} //END function parse()

/************************
****** main **************
**************************/
int main()
{
	int quittingtime = 0;

	while (!quittingtime)  
	{
		reader(); // read input and store globally to userinput
		
		if (matches(pos, 4, "quit"))    // quit, rather than use "matches" you can replace this with a condition for the "quit" token
			quittingtime = 1;
		else  
		{
			if (userinput[inputleng] == ENDOFINPUT) {
				//cout << "Input read into buffer, ready to tokenize." << endl;
				try 
				{
                    detect();
                    //printTokens();
				    int pos = 0;				
                    printValue(parse(pos));
                    cleanVecs();
                    std::cin.clear();
                    std::fill_n(userinput, inputleng, 0);
				}
				catch (string e) {
                    cout << e << endl;
                    cleanVecs();
                    std::cin.clear();
                    std::fill_n(userinput, inputleng, 0);
				}
			}
			else
			{
				cout << "Input Error: token found but end of input expected:  " << userinput[inputleng] << endl;
			}
        }

		/************************
		******* Print Input *******
		*************************/ // you will remove this and instead print the token list
		//userinput[inputleng] = '\0'; // for printing
		/*cout << "Number of Characters read:  " << inputleng << endl;
		cout << "Input:  ";
			for (int i = 0; i <= inputleng;  i++)// mimic input to confirm userinput array is populated.
				cout << userinput[i]; 
		cout << endl;*/

	}// while

	return 0;
} //END function main()