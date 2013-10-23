/*

Utility functions

*/

bool iswhitespace( char c )
{
	bool retVal = false;
	if( c == '\t' || c == ' ' )
		retVal = true;

	return retVal;
}

int strToInt( char *in )
{

    int out = 0;
	if( in[0] == '0' && in[1] == 'x' )
	{
		in++;
		in++;
		int diff = 'A' - '9' - 1;
    
		while( *in != '\0' )
		{
		     out *= 16;
				
			 if( *in >= 'A' && *in <= 'F' )
				 out += (int)( *in++ - '0' - diff );
			 else
				 out += (int)( *in++ - '0' );
		}
    }
	else
	{
		int sign = 1;
		if( *in == '-' )
		{
		    sign = -1;
		    in++;
		}        
    
		while( *in != '\0' )
		{    
		     out *= 10;
		     out += (unsigned int)( *in++ - '0' );
		}

		out *= sign;
	}
    
    return out;
}