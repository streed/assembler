#include "Parser.h"
#include "Utilities.h"
#include <iostream>
#include <fstream>
#include <string.h>
#include <ctype.h>

using std::cout;
using std::endl;
using std::fstream;

// PRE: address is defined.
// POST: The RV is a InstructionToken where in the various parts are set to there default values.
//	 And, the address is set to "address"
InstructionToken emptyInstructionToken( unsigned int address )
{
	InstructionToken retVal;

	for( int i = 0; i < 128; i++ )
	{
		retVal.original[i]  = '\0';
		retVal.lable[i]		= '\0';
	}

	for( int i = 0; i < 3; i++ )
		for( int j = 0; j < 128; j++ )
			retVal.params[i][j] = '\0';

	retVal.instruct.instruct.binary = 0;
	retVal.address = address;
	retVal.instruct.type = Types::NONE;
	retVal.hasLable = false;
	retVal.numParams = 0;
	return retVal;
}

// PRE: This function has 'a' and 'b' defined.
// POST: This will return 0 if the two symbol names are equal and nonzero 
//		if they are not.
int compareSymbols( ParseSymbol a, ParseSymbol b )
{
	int ret = 0;
	if( strcmp( a.name, b.name ) != 0 )
		ret = 1;
	return ret;
}

Parser::Parser( char *file )
{
	mSymbols = new List<ParseSymbol>( compareSymbols );
	strcpy( mFileName, file );
	sprintf( mPreProcessedFile, "%s.pre", mFileName );
	sprintf( mOutputFile, "%s.bin", mFileName );
}

#define IS_REG( C ) ( C == '$' )

// PRE: This object is defined.
// POST: The file that is to be parsed will be parsed and the
//       tokens from this file will be printed in the format as descripted by printInstruction.
void Parser::parse()
{
	fstream tFile;
	tFile.open( mPreProcessedFile );

	if( tFile.is_open() )
	{
		unsigned int PC = 0;
		char line[256];
		while( !tFile.eof() )
		{
			tFile.getline( line, 256 );
			InstructionToken token = parseLine( line, PC );
			if( strcmp( token.original, "" ) != 0 )
			{
				PC += 4;
				mTokens.add( token );
				addSymbol( token, PC );
			}
		}
		tFile.close();
		fixAddresses();
		printHexToFile();
	}
	else
	{
		cout << mPreProcessedFile << " could not be opened." << endl;
	}
}

// PRE: Ths object is defined as are token and PC.
// POST: The specific symbol in token will be added to mSymbols.
//		If the symbols to be added was thought to be a label, but is
//		in fact a variable this will be changed.
void Parser::addSymbol( InstructionToken token, uint32_t PC )
{
	if( token.hasLable )
	{
		ParseSymbol symbol;
		symbol.type = Symbols::LABLE;
		symbol.address = PC - 4;
		sprintf( symbol.name, "%s", token.lable );
		Link<ParseSymbol> *t = mSymbols->addUnique( symbol );

		if( t != 0 )
			t->setData( symbol );
	}

	if( token.params[0][0] != '\0' && !IS_REG( token.params[0][0] ) && !isdigit( token.params[0][0] ) )
	{
		ParseSymbol symbol;
		symbol.type = Symbols::VARIABLE;
		symbol.address = 0;
		sprintf( symbol.name, "%s", token.params[0] );
		mSymbols->addUnique( symbol );
	}

	if( token.params[1][0] != '\0' && !IS_REG( token.params[1][0] ) && !isdigit( token.params[1][0] ) )
	{
		ParseSymbol symbol;
		symbol.type = Symbols::VARIABLE;
		symbol.address = 0;
		sprintf( symbol.name, "%s", token.params[1] );
		mSymbols->addUnique( symbol );
	}

	if( token.params[2][0] != '\0' && !IS_REG( token.params[2][0] ) && !isdigit( token.params[2][0] ) )
	{
		ParseSymbol symbol;
		symbol.type = Symbols::VARIABLE;
		symbol.address = 0;
		sprintf( symbol.name, "%s", token.params[2] );
		mSymbols->addUnique( symbol );
	}
}

// PRE: This object is defined, this will only be called from parse().
// POST: The addresses in the symbol table for variables will be adjusted.
//		And, mTokens will be updated to reflect the actual memory locations.
void Parser::fixAddresses()
{
	//get length of program code in words.
	//After this length we will add the variables
	//As we come across them.
	uint32_t length = mTokens.length() * 4;
	for( int i = 0; i < mSymbols->length(); i++ )
	{
		ParseSymbol symbol = (*mSymbols)[i]->getData();
		if( symbol.type == Symbols::VARIABLE )
		{
			if( symbol.address == 0 )
			{
				symbol.address = length;
				length += 4;
				(*mSymbols)[i]->setData( symbol );
				InstructionToken zero;
				zero.instruct.instruct.binary = 0;
				mTokens.add( zero );
			}
		}
	}
	
	for( int i = 0; i < mSymbols->length(); i++ )
	{
		ParseSymbol symbol =  (*mSymbols)[i]->getData();
		for( int j = 0; j < mTokens.length(); j++ )
		{
			//check if any of the params match the variable or lable and insert the address there
			//if applicable, since the label/variable can only happen
			//in the second or last portion of the instruction.
			Link<InstructionToken> *token= mTokens[j];
			InstructionToken tok = token->getData();

			switch( token->getData().instruct.instruct.op )
			{
				case LW:
				case SW:
					if( strcmp( tok.params[1], symbol.name ) == 0 )
					{
						tok.instruct.instruct.y = getRegisterCode( "$fp" );
						tok.instruct.instruct.value = symbol.address;
					}
					break;
				case BEQ:
					if( strcmp( tok.params[2], symbol.name ) == 0 )
					{
						int offset = symbol.address - tok.address - 4;
						tok.instruct.instruct.value = offset;
					}
					break;
			}

			token->setData( tok );
		}
	}


}

// PRE: This object is defined.
// POST: The file that was passed to the parser will have been
//		preprocessed.  Which will mean that the nessecary 
//		substitution and instruction expansions will have taken
//		place and they will be output to a file with .pre
//		appended to the original file name.
void Parser::preprocess()
{
	fstream tFileOut;
	fstream tFile;
	tFile.open( mFileName );
	tFileOut.open( mPreProcessedFile, fstream::out | fstream::trunc );

	if( tFile.is_open() && tFileOut.is_open() )
	{
		char line[256];
		while( !tFile.eof() )
		{
			tFile.getline( line, 256 );
			List<char *> list;
			preprocessLine( &list, line );

			for( int i = 0; i < list.length(); i++ )
				tFileOut << list[i]->getData() << endl;
		}
	}
}

// PRE: This object is defined.
// POST: The mTokens list will be printed in HEX to mFileOutput.
//		Where each line contains one word.
void Parser::printHexToFile()
{
	fstream tFile;
	tFile.open( mOutputFile, fstream::out | fstream::trunc );

	if( tFile.is_open() )
	{
		for( int i = 0; i < mTokens.length(); i++ )
		{
			char line[LINE];

			sprintf( line, "%08X\n", mTokens[i]->getData().instruct.instruct.binary );

			tFile << line;
		}
	}
}

// PRE: This object and line is defined. 
// POST: The instruction is returned. If there is an error then the parse sets the proper error messaage and enters a error state.
//			This error state will halt parsing.
InstructionToken Parser::parseLine( char *line, unsigned int address)
{
	ParseStates::ParseState state = ParseStates::START;
	Opcode op = NONE;
	unsigned int charPos = 0;
	InstructionToken retVal = emptyInstructionToken( address );

	while( state != ParseStates::HALT && charPos < strlen( line ) && ( line[charPos] != '\r' || line[charPos] == '\n' ) && line[charPos] != ';' )
	{
		char c = line[charPos];
		retVal.original[charPos] = c;
		switch( state )
		{
			case ParseStates::START:
				getState( c, state );
				break;
			case ParseStates::WHITESPACE:
				if( iswhitespace( c ) )
					charPos++;
				else
					state = ParseStates::START;
				break;
			case ParseStates::ALPHA:
				if( iswhitespace( c ) )
					state = ParseStates::PARAMS;
				else
					parseOpcodeLabel( c, op, state, retVal );

				charPos++;
				break;
			case ParseStates::COMMENT:
				retVal.instruct.type = Types::COMMENT;
				if( c == '\r' || c == '\n' )
					state = ParseStates::HALT;
				else
					charPos++;
				break;
			case ParseStates::PARAMS:
				parseParams( c, op, state, retVal );
				charPos++;
				break;

		}
	}

	retVal.instruct.instruct.op = op;

	finalizeToken( retVal );

	return retVal;
}

// PRE: This object is defined. The current char c and the current state are both defined.
// POST: The next parse state will be in state.
void Parser::getState( char c, ParseStates::ParseState &state )
{
	if( iswhitespace( c ) )
		state = ParseStates::WHITESPACE;
	else if( isalpha( c ) )
		state = ParseStates::ALPHA;
	else if( c == ';' )
		state = ParseStates::COMMENT;
}

// PRE: This object is defined and instruct is defined.
// POST: The OS will have the contents of the instruction printed in the following format.
//		If the type is a comment then just Comment is printed on a single line.
//		If the type is Label then the following format is followed:
//			<original line>
//			Label: <label name> at address <address>
//		If the type is a instruction then the following format is followed:
//		R-type
//			<original line>
//			Opcode: <opcode in binary>, Reg. X: <binary of reg X>, Reg Y: <binary of reg Y>, Reg. Z: <binary of reg Z>
//		L-type
//			<original line>
//			Opcode: <opcode in binary>, Reg. X: <binary of reg X>, Reg. Y: <binary of reg Y>, Offset: <decimal of value>
//		J-type
//			<original line>
//			Opcode: <opcode in binary>, Reg. X: <binary of reg X>, Variable: <variable name>
void Parser::printInstruction( InstructionToken token )
{
	if( token.instruct.type != Types::COMMENT )
	{
		InstructionUnion instruct = token.instruct.instruct;
		cout << token.original << endl;
		if( token.hasLable )
			cout << "Lable: " << token.lable << " at address " << token.address << endl;
		cout << "Opcode: "; printBinary( instruct.op );
		cout << ", Reg. X: "; printBinary( instruct.x );
		switch( instruct.op )
		{
			case ADD: case NAND:
				cout << ", Reg. Y: "; printBinary( instruct.y );
				cout << ", Reg. Z: "; printBinary( instruct.z );
				break;
			case ADDI:
				cout << ", Reg. Y: "; printBinary( instruct.y );
				cout << ", Offset: "; printBinary( instruct.value );
				break;
			case BEQ:
				cout << ", Reg. Y: "; printBinary( instruct.y );
				cout << ", Offset";
				if( isalpha( token.params[2][0] ) )
					cout << " lable: " << token.params[2];
				else
					cout << ": "; printBinary( instruct.value );
				break;
			case LW: case SW:
				if( token.numParams == 2 )
				{
					cout << ", Reg. Y: "; printBinary( instruct.y );
					cout << ", Offset: " << instruct.value;
				}
				else
					cout << ", Variable: " << token.params[1];
				break;
			case JALR:
				cout << ", Reg. Y: "; printBinary( instruct.y );
				break;
			case HALT: break;
		}
		cout << endl;
	}
}

// PRE: This object is defined and as is value.
// POST: The OS will have the binary representation of value.
void Parser::printBinary( unsigned int value )
{
	unsigned int i;
	i = 1<<3;

	while (i > 0) {
		if (value & i)
			cout << "1";
		else
			cout << "0";
		i >>= 1;
	}
}

// PRE: This object is defined and c is defined as a alpha character.
// POST: The RV is the Opcode value of the suspected op code.
Opcode Parser::getOpcode( char c )
{
	Opcode op = NONE;
	if( c == 'a' )
		op = ADD;
	else if( c == 'n' )
		op = NAND;
	else if( c == 'l' )
		op = LW;
	else if( c == 's' )
		op = SW;
	else if( c == 'b' )
		op = BEQ;
	else if( c == 'j' )
		op = JALR;
	else if( c == 'h' )
		op = HALT;
	else if( c == 'i' )
		op = IN;
	else if( c == 'o' )
		op = OUT;
	return op;
}

// PRE: This object is defined. This will parse out the opcode and if need also the label.
// POST: The contents of token will be updated to reflect the opcode mnumonic and or if there is a lable also
//		 update this value as well.
void Parser::parseOpcodeLabel( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	parseLable( c, op, state, token );
	parseOpcode( c, op, state, token );
}

// PRE: This object, c, op, state, and token are defined.
// POST: The proper action based on the current op will be performed and state and token will have the
//		 nesscary changes made to them.
void Parser::parseOpcode( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	switch( op )
	{
		case NONE:
			op = getOpcode( c );
			break;
		case ADD:
			parseADD( c, op, state, token );
			break;
		case ADDI:
			//This state is handled by ADD automatically. It is here for completeness.
			break;
		case SW:
		case LW:
			parseLW( c, op, state, token );
			break;
		case BEQ:
			parseBEQ( c, op, state, token );
			break;
		case IN:
			parseIN( c, op, state, token );
			break;
		case OUT:
			parseOUT( c, op, state, token );
	}
}

// PRE: this object, c, op, state, and token are defined.
// POST: If c is a : then the proper values are changed in token and state to reflect this.  Token
//		 also gets a copy of the lable value for later recall.
void Parser::parseLable( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( c == ':' )
	{
		op = NONE;
		state = ParseStates::LABLE;
		token.hasLable = true;
		int length = strlen( token.original );
		int lablePos = 0;
		for( int i = 0; i < length; i++ )
		{
			if( iswhitespace( token.original[i] ) )
				continue;
			else
				token.lable[lablePos++] = token.original[i];
		}
		token.lable[lablePos - 1] = '\0';
		state = ParseStates::START;
	}
}

// PRE: This object is defined as are c, op, state, token. 
// POST: Giving the op the proper parsing function is called to parse the correct params. There are four
//		 different functions that can be called.
//		 Three Register opcodes.
//		 Two register and Value opcodes.
//		 One register and a offset of a register opcode..
//		 Two register opcodes.
//		 No register or params and thus require no parsing.
void Parser::parseParams( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	switch( op )
	{
		case ADD: case NAND:
			parseThreeRegisterParams( c, op, state, token );
			break;
		case ADDI: case BEQ:
			parseTwoRegisterValueParams( c, op, state, token );
			break;
		case LW: case SW:
			parseOneRegisterOffsetRegister( c, op, state, token );
			break;
		case JALR:
			parseTwoRegisterParams( c, op, state, token );
			break;
		case IN:
		case OUT:
			parseSingleRegisterParams( c, op, state, token );
		case HALT:
			break;
	}
}

// PRE: This object is defined and c, op, state, and token are also defined.
// POST: The continuation or starting of the parse iff op = ADD | NAND. The values in token and state 
//		 will be reflected accordingly.
void Parser::parseThreeRegisterParams( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	//counting starts at 0 and there is a max of 3 registers.
	if( token.numParams > 2 )
	{
		state = ParseStates::START;
	}//first param
	else if( c == ',' || c == '\r' || c == '\n' )
	{
		token.numParams++;
	}
	else if( !iswhitespace( c ) )//lets just add each register string to there respective array.
		token.params[token.numParams][strlen( token.params[token.numParams] )] = c;
}

// PRE: This object is defined and c, op, state, and token are also defined.
// POST: The continuation or starting of the parse iff op = ADDI | BEQ.
void Parser::parseTwoRegisterValueParams( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	//counting starts at 0 and there is a max of 3 registers.
	if( token.numParams > 2 )
	{
		state = ParseStates::START;
	}//first param
	else if( c == ',' || c == '\r' || c == '\n' )
	{
		token.numParams++;
	}
	else if( !iswhitespace( c ) )//lets just add each register string to there respective array.
		token.params[token.numParams][strlen( token.params[token.numParams] )] = c;
}

// PRE: This object is defined and c, op, state, and token are also defined.
// POST: The continuation or starting of the parse iff op = LW | SW. The values in token and state 
//		 will be reflected accordingly.
void Parser::parseOneRegisterOffsetRegister( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( token.numParams > 2 )
	{
		state = ParseStates::START;
	}
	else if( c == ',' || c == '(' || c == '\r' || c == '\n' )
	{
		token.numParams++;
	}
	else if( !iswhitespace( c ) && c != '(' && c != ')' )//lets just add each register string to there respective array.
		token.params[token.numParams][strlen( token.params[token.numParams] )] = c;
}

// PRE: This object is defined and c, op, state, and token are also defined.
// POST: The continuation or starting of the parse iff op = JALR. The values in token and state 
//		 will be reflected accordingly.
void Parser::parseTwoRegisterParams( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	//counting starts at 0 and there is a max of 3 registers.
	if( token.numParams > 1 )
	{
		state = ParseStates::START;
	}//first param
	else if( c == ',' || iswhitespace( c ) || c == '\r' || c == '\n' )
	{
		token.numParams++;
	}
	else//lets just add each register string to there respective array.
		token.params[token.numParams][strlen( token.params[token.numParams] )] = c;
}

void Parser::parseSingleRegisterParams( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( token.numParams >= 1 )
	{
		state = ParseStates::START;
	}
	else if( c == ',' || iswhitespace( c ) || c == '\r' || c == '\n' )
	{
		state = ParseStates::ALPHA;
	}
	else
		token.params[token.numParams][strlen( token.params[token.numParams ])] = c;
}

// PRE: This object is defined and token was gotten from parseLine.
// POST: The token will have its Instruction structure filled in with the relavant information.
void Parser::finalizeToken( InstructionToken &token )
{
	//get the proper register values.
//Define to lessen typing.
#define GETINSTRUCT( t ) t.instruct.instruct
	if( token.instruct.type != Types::COMMENT )
	{
		token.instruct.type = Types::INSTRUCTION;
		switch( GETINSTRUCT( token ).op )
		{
			case ADD: case NAND:
				GETINSTRUCT( token ).x = getRegisterCode( token.params[0] );
				GETINSTRUCT( token ).y = getRegisterCode( token.params[1] );
				GETINSTRUCT( token ).z = getRegisterCode( token.params[2] );
				break;
			case ADDI:
				GETINSTRUCT( token ).x = getRegisterCode( token.params[0] );
				GETINSTRUCT( token ).y = getRegisterCode( token.params[1] );
				GETINSTRUCT( token ).value = strToInt( token.params[2] );
				break;
			case BEQ:
				GETINSTRUCT( token ).x = getRegisterCode( token.params[0] );
				GETINSTRUCT( token ).y = getRegisterCode( token.params[1] );
				break;
			case LW: case SW:
				GETINSTRUCT( token ).x = getRegisterCode( token.params[0] );
				GETINSTRUCT( token ).y = getRegisterCode( token.params[2] );
				GETINSTRUCT( token ).value = strToInt( token.params[1] );
				break;
			case IN: case OUT:
				GETINSTRUCT( token ).x = getRegisterCode( token.params[0] );
			case JALR:
				break;
			case HALT:
				break;
		}
	}
}

// PRE: This object is defined and the registerStr is defined as well.
// POST: The RV is the 4bit code for the specific register.
unsigned int Parser::getRegisterCode( const char *registername )
{
	unsigned int retVal = 0;
	if( registername[0] == '$' )
	{
		switch( registername[1] )
		{
			case 'v':
				if( registername[2] == '0' ) retVal = 0x02; break;
			case 'a':
				switch( registername[2] )
				{
					case 't': retVal = 0x02; break;	case '0': retVal = 0x03; break;
					case '1': retVal = 0x04; break; case '2': retVal = 0x05; break;
				}
				break;
			case 't':
				switch( registername[2] )
				{
					case '0': retVal = 0x06; break;	case '1': retVal = 0x07; break;
					case '2': retVal = 0x08; break;
				}
				break;
			case 's':
				switch( registername[2] )
				{
					case '0': retVal = 0x09; break;	case '1': retVal = 0x0A; break;
					case '2': retVal = 0x0B; break; case 'p': retVal = 0x0D; break;
				}
				break;
			case 'k':
				switch( registername[2] )
				{
					case '0': retVal = 0x0C; break;
				}
				break;
			case 'f':
				switch( registername[2] )
				{
					case 'p': retVal = 0x0E; break;
				}
				break;
			case 'r':
				switch( registername[2] )
				{
					case 'a': retVal = 0x0F; break;
				}
				break;
		}
	}
	return retVal;
}


// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then state remains in its current state and
//		 op remains in ADD, and token is updated accordingly.
void Parser::parseADD( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( c == 'i' )
		op = ADDI;
	else if( c != 'd' && (token.original[strlen( token.original ) - 1] == 'd' || token.original[strlen( token.original ) - 1]== 'a') )
		state = ParseStates::ALPHA;
}
// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then state remains in its current state and
//		 op remains in LW, and token is updated accordingly.
void Parser::parseLW( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( c != 'w' )
		state = ParseStates::ALPHA;
}
//This define just makes things easier to type for these functions.
#define FOLLOWEDBY( a, b ) ( c == a && token.original[strlen( token.original ) - 2] == b )

// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then state remains in its current state and
//		 op remains in BEQ, and token is updated accordingly.
void Parser::parseBEQ( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( !(FOLLOWEDBY( 'e', 'b' ) || FOLLOWEDBY( 'q', 'e' ) ) )
		state = ParseStates::ALPHA;
}

// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then state remains in its current state and
//		 op remains in NAND, and token is updated accordingly.
void parseNAND( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( !(FOLLOWEDBY( 'a', 'n' ) && FOLLOWEDBY( 'n', 'a' ) && FOLLOWEDBY( 'd', 'n' ) ) )
		state = ParseStates::ALPHA;
}

// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then state remains in its current state and
//		 op remains in JALR, and token is updated accordingly.
void Parser::parseJALR( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( !( FOLLOWEDBY( 'a', 'j' ) && FOLLOWEDBY( 'l', 'a' ) && FOLLOWEDBY( 'r', 'l' ) ) )
		state = ParseStates::ALPHA;
}

// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then state remains in its current state and
//		 op remains in HALT, and token is updated accordingly.
void Parser::parseHALT( char c, Opcode &op, ParseStates::ParseState &state, InstructionToken &token )
{
	if( !( FOLLOWEDBY( 'a', 'h' ) && FOLLOWEDBY( 'l', 'a' ) && FOLLOWEDBY( 't', 'l' ) ) )
		state = ParseStates::ALPHA;
}

// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then
//       state remains in its current state and
//         op remains in IN, and token is updated accordingly.
void Parser::parseIN( char c, Opcode &op,
    ParseStates::ParseState &state, InstructionToken &token )
{
	if( !( FOLLOWEDBY( 'n', 'i' ) ) )
		state = ParseStates::ALPHA;
}

// PRE: This object is defined as are c, op, state, token.
// POST: If the current c helps to complete the opcode mnumonic then
//       state remains in its current state and
//         op remains in OUT, and token is updated accordingly.
void Parser::parseOUT( char c, Opcode &op,
    ParseStates::ParseState &state, InstructionToken &token )
{
	if( !( FOLLOWEDBY( 'u', 'o' ) || FOLLOWEDBY( 't', 'u' ) ) )
		state = ParseStates::ALPHA;
}

// PRE: This object and line are defined.  The line will be processed,
//		and if needed will be expanded into the proper format, as we
//		discussed in class.
// POST: A List<char *> will be returned. This list will be merged into
//		the remaining lines before the final pass and assembly happens.
void Parser::preprocessLine( List<char *> *list, char *line )
{
	InstructionToken token = parseLine( line, 0 );

	if( token.instruct.type == Types::INSTRUCTION )
	{
		switch( token.instruct.instruct.op )
		{
			case ADD:
			case NAND:
				preprocessThreeRegister( list, token );
				break;
			case ADDI:
				preprocessTwoRegistersOffsetStore( list, token );
				break;
			case BEQ:
				preprocessTwoRegistersOffset( list, token );
				break;
			case LW:
			case SW:
				preprocessTwoRegister( list, token );
				//preprocessTwoRegistersOffset( list, token );
				break;
			case IN:
			case OUT:
				preprocessSingleRegister( list, token );
				break;
			case JALR:
				preprocessTwoRegister( list, token );
				break;
		}
	}
}

#define IS_REG( C ) ( C == '$' )

// PRE: This object is defined, list and token are defined.
//		list is empty.
// POST: The preprocessing is handled and list contains the new lines.
//		these new lines contain the substitution's and expansions.
void Parser::preprocessThreeRegister( List<char *> *list, InstructionToken token )
{
	//check which params are in fact not registers.
	char *first_param = 0, *second_param = 0, *third_param = 0, *result = 0, *add_str;

	if( !IS_REG( token.params[2][0] ) )
	{
		third_param = new char[LINE];
		sprintf( third_param, "lw $t2, %s", token.params[2] );
	}

	if( !IS_REG( token.params[1][0] ) )
	{
		second_param = new char[LINE];
		sprintf( second_param, "lw $t1, %s", token.params[1] );
	}

	if( !IS_REG( token.params[0][0] ) )
	{
		first_param = new char[LINE];
		sprintf( first_param, "lw $t0, %s", token.params[0] );
		result = new char[LINE];
		sprintf( result, "sw $t0, %s", token.params[0] );
	}

	if( first_param != 0 )
		list->add( first_param );
	if( second_param != 0 )
		list->add( second_param );
	if( third_param != 0 )
		list->add( third_param );
	
	add_str = new char[LINE];

	sprintf( add_str, "%s%s%s %s, %s, %s",( token.hasLable ? token.lable : "" ), ( token.hasLable ? ": " : "" ), 
										   GetOpCodeString( token.instruct.instruct.op ), ( first_param != 0 ? "$t0":token.params[0] ),
										   ( second_param != 0 ? "$t1":token.params[1] ),
										   ( third_param != 0 ? "$t2":token.params[2] ) );
	list->add( add_str );

	if( result != 0 )
		list->add( result );
}

// PRE: This object is defined, list and token are defined.
//		list is empty.
// POST: The preprocessing is handled and list contains the new lines.
//		these new lines contain the substitution's and expansions.
void Parser::preprocessTwoRegister( List<char *> *list, InstructionToken token )
{
	if( token.numParams < 1 )
	{
		char *lw_str = new char[LINE];
		char *jalr_str = new char[LINE];
		sprintf( lw_str, "lw $k0, %s", token.params[0] );
		sprintf( jalr_str, "%s%sjalr $k0, $ra", ( token.hasLable ? token.lable : "" ), ( token.hasLable ? ":" : "" ) );
		list->add( lw_str );
		list->add( jalr_str );
	}
	else
	{
		char *instruct, *first_param = 0, *second_param = 0, *result_str;

		if( token.instruct.instruct.op == LW )
		{
			if( !IS_REG( token.params[0][0] ) )
			{
				first_param = new char[LINE];
				sprintf( first_param, "lw $t0, %s", token.params[0] );
				result_str = new char[LINE];
				sprintf( result_str, "sw $t0, %s", token.params[0] );
				list->add( first_param );
			}

			if( !IS_REG( token.params[1][0] ) )
			{
				second_param = new char[LINE];
				sprintf( second_param, "lw $t1, %s", token.params[1] );
				list->add( second_param );
			}

			instruct = new char[LINE];

			sprintf( instruct, "%s%s%s %s %s",( token.hasLable ? token.lable : "" ), ( token.hasLable ? ": " : "" ), 
										   GetOpCodeString( token.instruct.instruct.op ), ( first_param != 0 ? "$t0":token.params[0] ),
										   ( second_param != 0 ? "$t1":token.params[1] ) );
			list->add( instruct );

			if( result_str != 0 )
				list->add( result_str );
		}
		else if( token.instruct.instruct.op == SW )
		{
			if( !IS_REG( token.params[0][0] ) )
			{
				first_param = new char[LINE];
				sprintf( first_param, "lw $t0, %s", token.params[0] );
				result_str = new char[LINE];
				list->add( first_param );
			}

			if( !IS_REG( token.params[1][0] ) )
			{
				second_param = new char[LINE];
				sprintf( second_param, "lw $t1, %s", token.params[1] );
				list->add( second_param );
				sprintf( result_str, "sw $t1, %s", token.params[0] );
			}

			instruct = new char[LINE];

			sprintf( instruct, "%s%s%s %s %s",( token.hasLable ? token.lable : "" ), ( token.hasLable ? ": " : "" ), 
										   GetOpCodeString( token.instruct.instruct.op ), ( first_param != 0 ? "$t0":token.params[0] ),
										   ( second_param != 0 ? "$t1":token.params[1] ) );

			list->add( instruct );

			if( result_str != 0 )
				list->add( result_str );
		}
	}
}

// PRE: This object is defined, list and token are defined.
//		list is empty.
// POST: The preprocessing is handled and list contains the new lines.
//		these new lines contain the substitution's and expansions.
void Parser::preprocessTwoRegistersOffset( List<char *> *list, InstructionToken token )
{
	char *second_param = 0, *first_param = 0;

	if( !IS_REG( token.params[1][0] ) )
	{
		second_param = new char[LINE];
		sprintf( second_param, "lw $t2, %s", token.params[1] );
	}

	if( !IS_REG( token.params[0][0] ) )
	{
		first_param = new char[LINE];
		sprintf( first_param, "lw $t1, %s", token.params[0] );
	}

	if( first_param != 0 )
		list->add( first_param );
	if( second_param != 0 )
		list->add( second_param );
	
	char *add_str = new char[LINE];
	sprintf( add_str, "%s%s%s %s, %s, %s",( token.hasLable ? token.lable : "" ), ( token.hasLable ? ": " : "" ), 
										   GetOpCodeString( token.instruct.instruct.op ), ( first_param != 0 ? "$t0":token.params[0] ),
										   ( second_param != 0 ? "$t1":token.params[1] ),
										   token.params[2] );
	list->add( add_str );
}

// PRE: This object is defined, list and token are defined.
//		list is empty.
// POST: The preprocessing is handled and list contains the new lines.
//		these new lines contain the substitution's and expansions.
void Parser::preprocessTwoRegistersOffsetStore( List<char *> *list, InstructionToken token )
{
	char *second_param = 0, *first_param = 0, *result_str = 0;

	if( !IS_REG( token.params[1][0] ) )
	{
		second_param = new char[LINE];
		sprintf( second_param, "lw $t1, %s", token.params[1] );
	}

	if( !IS_REG( token.params[0][0] ) )
	{
		first_param = new char[LINE];
		sprintf( first_param, "lw $t0, %s", token.params[0] );
		result_str = new char[LINE];
		sprintf( result_str, "sw $t0, %s", token.params[0] );
	}

	if( first_param != 0 )
		list->add( first_param );
	if( second_param != 0 )
		list->add( second_param );
	
	char *add_str = new char[LINE];
	sprintf( add_str, "%s%s%s %s, %s, %s",( token.hasLable ? token.lable : "" ), ( token.hasLable ? ": " : "" ), 
										   GetOpCodeString( token.instruct.instruct.op ), ( first_param != 0 ? "$t0":token.params[0] ),
										   ( second_param != 0 ? "$t1":token.params[1] ),
										   token.params[2] );
	list->add( add_str );

	if( result_str != 0 )
		list->add( result_str );
}

// PRE: This object is defined, list and token are defined.
//		list is empty.
// POST: The preprocessing is handled and list contains the new lines.
//		these new lines contain the substitution's and expansions.
void Parser::preprocessSingleRegister( List<char *> *list, InstructionToken token )
{
	if( !IS_REG( token.params[0][0] ) )
	{
		char *lw_str = new char[LINE];
		char *inout_str = new char[LINE];
		sprintf( lw_str, "lw $t0, %s", token.params[0] );
		sprintf( inout_str, "%s%s%s $t0",( token.hasLable ? token.lable : "" ), ( token.hasLable ? ":" : "" ), GetOpCodeString( token.instruct.instruct.op ) );
		list->add( lw_str );
		list->add( inout_str );

		if( token.instruct.instruct.op == IN )
		{
			char *sw_str = new char[LINE];
			sprintf( sw_str, "sw $t0, %s", token.params[0] );
			list->add( sw_str );
		}
	}
}


#ifdef TESTING
#include <assert.h>

void testParserIN()
{
	Parser p;
	InstructionToken token = p.parseLine( "in $a2", 0 );

	assert( token.instruct.instruct.op == IN );
}

void testParserOUT()
{
	Parser p;
	InstructionToken token = p.parseLine( "out $a2", 0 );
	
	assert( token.instruct.instruct.op == OUT );
}

void testParserBEQ()
{
	Parser p;
	InstructionToken token = p.parseLine( "beq $a0, $a1, $a2", 0 );
	assert( token.instruct.instruct.op == BEQ );
}

void testParserLabel()
{
	Parser p;
	InstructionToken token = p.parseLine( "addlabel: add $a1, $a1, $t0", 0 );
	assert( token.hasLable );
	assert( token.instruct.instruct.op == ADD );
}

void testParserSingleRegisterReplacementOUTX()
{
	Parser p;
	List<char *> lines;
	p.preprocessLine( &lines, "out x" );

	assert( strcmp( lines[0]->getData(), "lw $t0, x" ) == 0 );
	assert( strcmp( lines[1]->getData(), "out $t0" ) == 0 );
}

void testParserSingleRegisterReplacementINX()
{
	Parser p;
	List<char *> lines;
	p.preprocessLine( &lines, "in x" );

	assert( strcmp( lines[0]->getData(), "lw $t0, x" ) == 0 );
	assert( strcmp( lines[1]->getData(), "in $t0" ) == 0 );
	assert( strcmp( lines[2]->getData(), "sw $t0, x" ) == 0 );
}

void testParserTwoRegisterReplacementOffset()
{
	Parser p;
	List<char *> lines;
	p.preprocessLine( &lines, "lw $v0, y" );
}

void testParserTwoRegisterReplacementX()
{
	Parser p;
	List<char *> lines;
	p.preprocessLine( &lines, "jalr x" );

	assert( strcmp( lines[0]->getData(), "lw $k0, x" ) == 0 );
	assert( strcmp( lines[1]->getData(), "jalr $k0, $ra" ) == 0 );
}

void testParserVariableRegisterReplacementX()
{
	Parser p;
	List<char *> lines;
	p.preprocessLine( &lines, "add $a0, $a1, x" );

	assert( strcmp( lines[0]->getData(), "lw $t2, x" ) == 0 );
	assert( strcmp( lines[1]->getData(), "add $a0, $a1, $t2" ) == 0 );
}

void testParserVariableRegisterReplacementXY()
{
	Parser p;
	List<char *> lines;
	p.preprocessLine( &lines, "add $a0, x, y" );

	assert( strcmp( lines[0]->getData(), "lw $t1, x" ) == 0 );
	assert( strcmp( lines[1]->getData(), "lw $t2, y" ) == 0 );
	assert( strcmp( lines[2]->getData(), "add $a0, $t1, $t2" ) == 0 );
}

void testParserVariableRegisterReplacementXYZ()
{
	Parser p;
	List<char *> lines;
	p.preprocessLine( &lines, "add x, y, z" );

	assert( strcmp( lines[0]->getData(), "lw $t0, x" ) == 0 );
	assert( strcmp( lines[1]->getData(), "lw $t1, y" ) == 0 );
	assert( strcmp( lines[2]->getData(), "lw $t2, z" ) == 0 );
	assert( strcmp( lines[3]->getData(), "add $t0, $t1, $t2" ) == 0 );
	assert( strcmp( lines[4]->getData(), "sw $t0, x" ) == 0 );
}

#endif