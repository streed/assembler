/*
    Parser: Will parse a LC2200 assembly file.

    The enums ensured the use of the correct values and are a more secure over
    using
    #define's to lable the magic numbers.

    The structure of the Instruction is handled through way of a union. This 
    union maps out all the various parts of the instruction over the 32bit 
    binary representation. This allows for direct getting of the values without
    the need for parsing.  The structures are upside down in nature because of
    the way integers are represented in memory. The more natural way of laying 
    out the structures results in the wrong representation.

    by streed
*/

#ifndef __PARSER__
#define __PARSER__

#include <stdint.h>
#include "List.h"

#define LINE 128
#define NUM_PARAMS 3

/*
    Instruction Types -- This is in a namespace because the names overlap the 
    ParseStates
*/
namespace Types
{
    typedef enum __type
    {
        NONE,
        COMMENT,
        INSTRUCTION,
        LABLE
    }Type;
}

namespace Symbols
{
	typedef enum __symbol
	{
		LABLE,
		VARIABLE,
		UNKNOWN
	}Symbol;
}

/*
    Parser States enum -- This is in a namespace because the names overlap the
    Types
*/
namespace ParseStates
{
    typedef enum __parsestate
    {
        START,
        WHITESPACE,
        ALPHA,
        LABLE,
        COMMENT,
        INSTRUCT,
        PARAMS,
        HALT
    }ParseState;
}

/*
    Opcode values - prevents coder error and enforces that these are the only
    valid opcode values. ADD = 0 ... HALT = 7
*/
typedef enum
{
    ADD,
    NAND,
    ADDI,
    LW,
    SW,
    BEQ,
    JALR,
    HALT,
	IN,
	OUT,
    NONE
}Opcode;

static char *OpcodeStrings[] = 
{
	"add", 
	"nand", 
	"addi", 
	"lw", 
	"sw", 
	"beq", 
	"jalr",
	"halt",
	"in",
	"out"
};

#define GetOpCodeString( op ) ( op <= 9 ? OpcodeStrings[op]: "Invalid" )

/*

	ParseSymbol holds the name, type, address

*/
typedef struct __symbol
{
	char name[LINE];
	Symbols::Symbol type;
	uint32_t address;
}ParseSymbol;

// PRE: This function has 'a' and 'b' defined.
// POST: This will return 0 if the two symbol names are equal and nonzero 
//		if they are not.
int compareSymbols( ParseSymbol a, ParseSymbol b );

/*

    Opcode representation in memory.
    The structure is inverted because of the bit endiness.

*/
typedef union _instructionunion
{
    struct
    {
        unsigned z:4;//4 bit value for Z register.
        unsigned :28;//Padding of 28bits.
    };
    struct
    {
        signed value:20;//signed value.
        unsigned :12;//Padding of 12bits.
    };
    struct
    {
        unsigned :20;//Padding of 20bits.
        unsigned y:4;//4 bit value for Y register.
        unsigned x:4;//4 bit value for X register.
        unsigned op:4;//4 bit value for OPCODE.
    };
    uint32_t binary;
}InstructionUnion;

/*
    An instruction contains both its type, and the binary representation. type
    is used by the parser.
*/
typedef struct _instruction
{
    Types::Type type;
    InstructionUnion instruct;
}Instruction;

/*
    This structure makes parsing easier as it stores the text of the various
    parts in seperate arrays.
*/
typedef struct __instructtoken
{
    char original[LINE];//holds the original line of code.
    char lable[LINE];//If there is a lable its name is store here.
    char params[NUM_PARAMS][LINE];//This holds the string values for each param.
    bool hasLable;//If this is true then when this is printed it will print out
                  //the lable as well.
    uint32_t numParams;//Used by the parser to keep track of parameters.
    uint32_t address;//The address.

    Instruction instruct;//Binary representation of this instruction.
}InstructionToken;

class Parser
{
    public:
		// PRE: Default constructor
		// POST: This object will be defined.
		Parser(){};
        // PRE: file is defined.
        // POST: A file handle of "file" will be opened.
        Parser( char *file );

        // PRE: This object is defined. 
		// POST: This happens after the file is preprocess'ed.
		//		It will take the preprocessed file and output a
		//		.bin file that will contain the hex results of the
		//		program.
        void parse();

		// PRE: This object is defined.
		// POST: The file that was passed to the parser will have been
		//		preprocessed.  Which will mean that the nessecary 
		//		substitution and instruction expansions will have taken
		//		place and they will be output to a file with .pre
		//		appended to the original file name.
		void preprocess();

		// PRE: This object is defined.
		// POST: The mTokens list will be printed in HEX to mFileOutput.
		//		Where each line contains one word.
		void printHexToFile();

        // PRE: This object and line are defined. lastAddress is either 0 or 
        //      the last
        //        address from the parse.
        // POST: The instruction is returned. If there is an error then 
        //         the parse sets the proper error messaage and enters a error 
        //         state.
        //            This error state will halt parsing.
        InstructionToken parseLine( char *line, uint32_t lastAddress );

		// PRE: This object and line are defined.  The line will be processed,
		//		and if needed will be expanded into the proper format, as we
		//		discussed in class.
		// POST: list will contain the expanded lines if any expansion needs to
		//		happen.  Else it will contain the original line.
		void preprocessLine( List<char *> *list, char *line );

        // PRE: This object is defined and mTokens is defined.
        // POST: The vector<Instruction> mTokens is returned that 
        //         contains the tokens from the parsed file mFile.
        //vector<InstructionToken> *getTokens() const { return mTokens; }

        // PRE: This object is defined.
        // POST: RV is the relevant error_state object representing the current
        //       error state.
        //error_state getLastError();

        // PRE: This object is defined and instruct is defined.
        // POST: The OS will have the contents of the instruction printed in 
        //       the following format.
        //        If the type is a comment then just Comment is printed on a 
        //        single line.
        //        If the type is Label then the following format is followed:
        //            <original line>
        //            Label: <label name> at address <address>
        //        If the type is a instruction then the following format is 
        //        followed:
        //        R-type
        //<original line>
        //Opcode: <opcode binary>, Reg. X: <binary X>, Reg Y: <binary Y>, 
        //                                                   Reg. Z: <binary Z>
        //        L-type
        //<original line>
        //Opcode: <opcode binary>, Reg. X: <binary X>, Reg. Y: <binary Y>, 
        //                                           Offset: <decimal of value>
        //        J-type
        //<original line>
        //Opcode: <opcode binary>, Reg. X: <binary X>, Variable: <var name>
        void printInstruction( InstructionToken token );

    private:
        // PRE: This object is defined and as is value.
        // POST: The OS will have the binary representation of value.
        void printBinary( uint32_t value );

        // PRE: This object is defined and c is defined.
        // POST: Based on the char c the proper opcode is returned, this is a 
        //       base guess on what
        //         the final opcode will be for this instruction.
        Opcode getOpcode( char c );

        // PRE: This object is defined. This will parse out the opcode and if 
        //      need also the label.
        // POST: The contents of token will be updated to reflect the opcode
        //       mnumonic and or if there is a lable also
        //         update this value as well.
        void parseOpcodeLabel( char c, Opcode &op, 
            ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined. The current char c and the current 
        //      state are both defined.
        // POST: The next parse state will be in state.
        void getState( char c, ParseStates::ParseState &state );

        // PRE: This object, c, op, state, and token are defined.
        // POST: The proper action based on the current op will be performed
        //       and state and token    will have the
        //         nesscary changes made to them.
        void parseOpcode( char c, Opcode &op, 
            ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined as are c, op, state, token. 
        // POST: Giving the op the proper parsing function is called to parse
        //       the correct params. There are four
        //         different functions that can be called.
        //         Three Register opcodes.
        //         Two register and Value opcodes.
        //         One register and a offset of a register opcode..
        //         Two register opcodes.
        //         No register or params and thus require no parsing.
        void parseParams( char c, Opcode &op, 
            ParseStates::ParseState &state, InstructionToken &token );

        // PRE: this object, c, op, state, and token are defined.
        // POST: If c is a : then the proper values are changed in token and
        //       state to reflect this.  Token
        //         also gets a copy of the lable value for later recall.
        void parseLable( char c, Opcode &op,
             ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined and c, op, state, and token are also 
        //      defined.
        // POST: The continuation or starting of the parse iff op = ADD | NAND.
        //       The values in token and state 
        //         will be reflected accordingly.
        void parseThreeRegisterParams( char c, Opcode &op,
             ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined and c, op, state, and token are also 
        //      defined.
        // POST: The continuation or starting of the parse iff op = ADDI | BEQ.
        void parseTwoRegisterValueParams( char c, Opcode &op,
             ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined and c, op, state, and token are also 
        //      defined.
        // POST: The continuation or starting of the parse iff op = LW | SW. 
        //       The values in token and state 
        //         will be reflected accordingly.
        void parseOneRegisterOffsetRegister( char c, Opcode &op,
             ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined and c, op, state, and token are also 
        //      defined.
        // POST: The continuation or starting of the parse iff op = JALR. 
        //       The values in token and state 
        //         will be reflected accordingly.
        void parseTwoRegisterParams( char c, Opcode &op,
             ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined and c, op, state, and token are also 
        //      defined.
        // POST: The continuation or starting of the parse iff op = IN|OUT. 
        //       The values in token and state 
        //         will be reflected accordingly.
		void parseSingleRegisterParams( char c, Opcode &op, 
			 ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined and token was gotten from parseLine.
        // POST: The token will have its Instruction structure filled in with 
        //       the relavant information.
        void finalizeToken( InstructionToken &token );

        // PRE: This object is defined and the registerStr is defined as well.
        // POST: The RV is the 4bit code for the specific register.
        uint32_t getRegisterCode( const char *registername );

        /*
            Opcode specific parsing functions.
        */
        // PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete teh opcode mnumonic then 
        //       state remains in its current state and
        //       op remains in ADD, and token is updated accordingly. The 
        //       values in token and state 
        //         will be reflected accordingly.
        void parseADD( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete the opcode mnumonic then 
        //       state remains in its current state and
        //         op remains in LW, and token is updated accordingly.
        void parseLW( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete the opcode mnumonic then 
        //       state remains in its current state and
        //         op remains in BEQ, and token is updated accordingly.
        void parseBEQ( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete the opcode mnumonic then 
        //       state remains in its current state and
        //         op remains in NAND, and token is updated accordingly.
        void parseNAND( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete the opcode mnumonic then 
        //       state remains in its current state and
        //         op remains in JALR, and token is updated accordingly.
        void parseJALR( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

        // PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete the opcode mnumonic then
        //       state remains in its current state and
        //         op remains in HALT, and token is updated accordingly.
        void parseHALT( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

		// PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete the opcode mnumonic then
        //       state remains in its current state and
        //         op remains in IN, and token is updated accordingly.
        void parseIN( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

		// PRE: This object is defined as are c, op, state, token.
        // POST: If the current c helps to complete the opcode mnumonic then
        //       state remains in its current state and
        //         op remains in OUT, and token is updated accordingly.
        void parseOUT( char c, Opcode &op,
         ParseStates::ParseState &state, InstructionToken &token );

		// PRE: This object is defined, list and token are defined.
		//		list is empty.
		// POST: The preprocessing is handled and list contains the new lines.
		//		these new lines contain the substitution's and expansions.
		void preprocessThreeRegister( List<char *> *list, InstructionToken token );

		// PRE: This object is defined, list and token are defined.
		//		list is empty.
		// POST: The preprocessing is handled and list contains the new lines.
		//		these new lines contain the substitution's and expansions.
		void preprocessTwoRegister( List<char *> *list, InstructionToken token );

		// PRE: This object is defined, list and token are defined.
		//		list is empty.
		// POST: The preprocessing is handled and list contains the new lines.
		//		these new lines contain the substitution's and expansions.
		void preprocessTwoRegistersOffset( List<char *> *list, InstructionToken token );

		// PRE: This object is defined, list and token are defined.
		//		list is empty.
		// POST: The preprocessing is handled and list contains the new lines.
		//		these new lines contain the substitution's and expansions.
		void preprocessTwoRegistersOffsetStore( List<char *> *list, InstructionToken token );

		// PRE: This object is defined, list and token are defined.
		//		list is empty.
		// POST: The preprocessing is handled and list contains the new lines.
		//		these new lines contain the substitution's and expansions.
		void preprocessSingleRegister( List<char *> *list, InstructionToken token );

		// PRE: Ths object is defined as are token and PC.
		// POST: The specific symbol in token will be added to mSymbols.
		//		If the symbols to be added was thought to be a label, but is
		//		in fact a variable this will be changed.
		void addSymbol( InstructionToken token, uint32_t PC );

		// PRE: This object is defined, this will only be called from parse().
		// POST: The addresses in the symbol table for variables will be adjusted.
		//		And, mTokens will be updated to reflect the actual memory locations.
		void fixAddresses();

        char mFileName[256];
		char mPreProcessedFile[256];
		char mOutputFile[256];

		List<ParseSymbol> *mSymbols;
		List<InstructionToken> mTokens;
};

/*
	
	Below are the test cases, they are only included when TESTING is defined. 
	This can be done via the command line compile argument, or by doing.
	make test
	./test
*/
#ifdef TESTING
// Tests if the parser is able to parse IN correctly.
void testParserIN();
// Tests if the parser is able to parse OUT correctly.
void testParserOUT();
// Tests if the parser is able to parse BEQ correctly.
void testParserBEQ();
// Tests if the parser handles labels correctly.
void testParserLabel();
// Tests the preprocessing on the IN instruction when its second param
// is a variable.
void testParserSingleRegisterReplacementINX();
// Tests the preprocessing on the OUT instruction when its second param
// is a variable.
void testParserSingleRegisterReplacementOUTX();
// Tests two register instructions where the second register is a variable.
void testParserTwoRegisterReplacementX();
// Tests single variable replace on three register instructions.
void testParserVariableRegisterReplacementX();
// Tests double variable replace on three register instructions.
void testParserVariableRegisterReplacementXY();
// Tests triple variable replace on three register instructions.
void testParserVariableRegisterReplacementXYZ();
// Tests the handling of two register instruction with offset.
void testParserTwoRegisterReplacementOffset();
#endif

#endif
