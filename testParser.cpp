#include "Parser.h"
#include <iostream>

using namespace std;

/*int main()
{
	Parser p("");

/*
	      add $v0, $a1, $s0
; this is a comment
one:  lw $a0, 20($a1) ; comment at the end of an instruction
two:  beq $a0, $a1, done
      lw $a2, x
*/
	InstructionToken a;// = p.parseLine( ";comment test", 0);
	//p.printInstruction( a.instruct );

	a = p.parseLine( "		add $v0, $a1, $s0", 0);
	p.printInstruction( a );
	a = p.parseLine( "addi $v0, $t0, 25", 4 );
	p.printInstruction( a );
	a = p.parseLine( "one: lw $a0, -20($a1);shouldn't do anything", 8 );
	p.printInstruction( a );
	a = p.parseLine( "beq $a0, $a1, done", 12 );
	p.printInstruction( a );
	a = p.parseLine( "two: lw $a2, x", 16 );
	p.printInstruction( a );

	int n = 0;
	int b = 1;
	n += 1 == b;

	return 0;
}*/