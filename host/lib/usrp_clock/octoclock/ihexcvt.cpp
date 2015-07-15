/*  IHexCvt - Intel HEX File <=> Binary Converter (C++)
    Copyright (C) 2014  Ali Nakisaee

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.


You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.*/

//Include needed stuff from C++
#include <iostream>
#include <fstream>
#include <string>

//... and also from C
#include <stdio.h>

#include <boost/filesystem.hpp>

#include <uhd/exception.hpp>
#include "ihexcvt.hpp"

//Avoid repeating 'std::':
using namespace std;

//The following function reads a hexadecimal number from a text file.
template <class T>
static bool ReadValueFromHex(ifstream& InputFile, T& outCh, unsigned char* ApplyChecksum)
{
	char V, L;
	T X = 0;
	outCh = 0;

	//Get the characters one by one.
	//Remember: These values are big-endian.
	//Remember: Every two hex characters (0-9/A-F) indicate ONE byte.
	for (size_t i = 0; i < 2 * sizeof(T); i++)
	{
		InputFile.get( V );
		if (InputFile.fail())
			return false;

		X <<= 4;
		if (V >= '0' && V <= '9')
			L = (V - '0');
		else if (V >= 'a' && V <= 'f')
			L = (V - 'a' + 10);
		else if (V >= 'A' && V <= 'F')
			L = (V - 'A' + 10);
		else
			return false;
		X |= L;

		//Apply this character to the checksum
		if (ApplyChecksum && i % 2 == 1) *ApplyChecksum += X & 0xFF;
	}

	//Return...
	outCh = X;
	return true;
}

//The following function writes a hexadecimal number from a text file.
template <class T>
static bool WriteHexValue(ofstream& OutFile, T Value, unsigned char* CalcChecksum)
{
	unsigned char V0 = 0;
	char C;

	//Remember: These values are big-endian.
	for (size_t i = 0; i < sizeof(T); i++)
	{
		//Get byte #i from the value.
		V0 = (Value >> ((sizeof(T) - i - 1) * 8)) & 0xFF;

		//Extract the high nibble (4-bits)
		if ((V0 & 0xF0) <= 0x90)
			C = (V0 >> 4) + '0';
		else
			C = (V0 >> 4) + ('A' - 10);
		OutFile.put( C );

		//Extract the low nibble (4-bits)
		if ((V0 & 0xF) <= 0x9)
			C = (V0 & 0xF) + '0';
		else
			C = (V0 & 0xF) + ('A' - 10);
		OutFile.put( C );

		//Calculate the checksum
		if (CalcChecksum) *CalcChecksum += V0;
	}
	return true;
}

//Skip any incoming whitespaces
static void SkipWhitespace(ifstream& InputFile)
{
	for (;;)
	{
		char C;
		InputFile.get(C);
		if (InputFile.eof() || InputFile.fail()) break;
		if (!(C == '\n' || C == '\r' || C == ' ' || C == '\t' || C == '\v'))
		{
			InputFile.putback(C);
			break;
		}
	}
}

//The function responsible for conversion from HEX files to BINary.
void Hex2Bin(const char* SrcName, const char* DstName, bool IgnoreChecksum)
{
	ifstream Src(SrcName);
	if (Src.bad())
	{
        throw uhd::runtime_error("Could not convert Intel .hex file to binary.");
	}

	ofstream Dst(DstName, ios_base::binary);
	if (Dst.bad())
	{
        throw uhd::runtime_error("Could not convert Intel .hex file to binary.");
	}

	char Ch;
	int LineIdx = 1;

	unsigned char ByteCount;
	unsigned short AddressLow;
	unsigned short Extra;
	unsigned long ExtraL;
	unsigned long AddressOffset = 0;
	unsigned char RecordType;
	unsigned char Data[255];
	unsigned char CurChecksum;
	unsigned char FileChecksum;
	bool EOFMarker = false;
	bool EOFWarn = false;
	
	for ( ;; )
	{
		Src.get(Ch);
		if (Src.eof())
			break;
		if (EOFMarker && !EOFWarn)
		{
            throw uhd::runtime_error("Could not convert Intel .hex file to binary.");
		}
		if (Ch != ':') goto genericErr;

		CurChecksum = 0;
		if (!ReadValueFromHex( Src, ByteCount, &CurChecksum )) goto genericErr;
		if (!ReadValueFromHex( Src, AddressLow, &CurChecksum )) goto genericErr;
		if (!ReadValueFromHex( Src, RecordType, &CurChecksum )) goto genericErr;

		switch (RecordType)
		{
		case 0x00: //Data record
			for (int i = 0; i < ByteCount; i++)
				if (!ReadValueFromHex( Src, Data[i], &CurChecksum )) goto genericErr;
			break;
		case 0x01: //End Marker
			if ( ByteCount != 0 )
			{
				goto onErrExit;
			}
			EOFMarker = true;
			break;
		case 0x02: //Extended Segment Address
			if ( ByteCount != 2 || AddressLow != 0 )
			{
				goto onErrExit;
			}
			if (!ReadValueFromHex( Src, Extra, &CurChecksum )) goto genericErr;
			AddressOffset = (unsigned long)Extra << 4;
			break;
		case 0x03: //Start Segment Address
			if ( ByteCount != 4 || AddressLow != 0 )
			{
				goto onErrExit;
			}
			if (!ReadValueFromHex( Src, ExtraL, &CurChecksum )) goto genericErr;
			break;
		case 0x04: //Extended Linear Address
			if ( ByteCount != 2 || AddressLow != 0 )
			{
				goto onErrExit;
			}
			if (!ReadValueFromHex( Src, Extra, &CurChecksum )) goto genericErr;
			AddressOffset = (unsigned long)Extra << 16;
			break;
		case 0x05: //Start Linear Address
			if ( ByteCount != 4 || AddressLow != 0 )
			{
				goto onErrExit;
			}
			if (!ReadValueFromHex( Src, ExtraL, &CurChecksum )) goto genericErr;
			break;
		}
		
		//Verify checksum
		CurChecksum = (~(CurChecksum & 0xFF) + 1) & 0xFF;
		if (!ReadValueFromHex( Src, FileChecksum, NULL )) goto genericErr;
		if (CurChecksum != FileChecksum)
		{
			if (!IgnoreChecksum) goto onErrExit;
		}

		//Put Data
		if (RecordType == 0x00)
		{
			Dst.seekp( AddressLow + AddressOffset );
			for (int i = 0; i < ByteCount; i++)
			{
				Dst.put( Data[i] );
			}
		}

		//Skip any white space
		SkipWhitespace( Src );

		LineIdx++;
	}

	Dst << flush;
	Dst.close();

	return;

genericErr:
    throw uhd::runtime_error("Invalid Intel .hex file detected.");

onErrExit:
	Dst.close();
	Src.close();
    boost::filesystem::remove(DstName);
    throw uhd::runtime_error("Could not convert Intel .hex file to binary.");
}
