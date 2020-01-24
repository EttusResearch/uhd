//PURPOSE: C test bench for floating point converter IQ_to_FLOAT

#include <cstdlib>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <bitset>
#include <cmath>
#include "math.h"
#include <iomanip>

using namespace std;
//INITIAL TESTING PURPOSES: Use if you want to print individual bits	
	template <typename T>

	void print_bits(T n) {
		T mask = 1 << (sizeof(T)*8-1);
		while (mask) {
			cout << ((mask & n) ? "1" : "0");
			mask >>= 1;
		}
	cout << endl;
	}

	int main()	{
		

		FILE *convFile;
		FILE *newFile;


		
		


		convFile = fopen("iq_to_float_input.txt", "w");
		newFile =  fopen("iq_to_float_output.txt", "w");
        //iterate through test cases
    
        for (signed int i = -0x8000; i <= 0x7FFF; i++)   {

		float end = float(i*exp2(-15));
        

		unsigned int n = *(reinterpret_cast<unsigned int*>(&end));

        //IN CASE YOU NEED TO LOOK AT SPECIFIC EXPONENT, FRAC, ETC VALUES
        //ACTIVATE BY UNCOMMENTING
        /*

		unsigned int signed_bit = n>>31;
		
		unsigned int exp =  ((n>>23) &0xFF);
		
		unsigned int frac =  (n &0x7FFFFF);

		cout << "end: " << end << endl;
		cout << "n: " << hex << n << endl;



		cout << "signed bit:" << hex << signed_bit << endl;
		cout << "exp: " << hex <<  exp << endl;
		cout << "fract: " << hex << frac << endl;

		float f = *(float*)&n;
		cout << "f" << f << endl;
*/		
		
	//	print_bits<unsigned short>(start);
	//	print_bits<unsigned int>(n);
	    unsigned int something = i;
        something &= 0xFFFF;

		fprintf(convFile, "%x\n",something);
		fprintf(newFile, "%x\n",n);
        }

	fclose(convFile);
	fclose(newFile);


	return 0;
} 
	 

	
