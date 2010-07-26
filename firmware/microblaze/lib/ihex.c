/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include "ihex.h"
#include <ctype.h> //man that pulls in a lot of shit

//this is not safe and you should run isxdigit beforehand
uint8_t asc2nibble(char input) {
	if(input > 'Z') return input - 'W';
	else if(input > '9') return input - '7';
	else return input - '0';
}

int ihex_parse(char input[], ihex_record_t *record) {
	//given a NULL-TERMINATED input string (use gets()) in I16HEX format, write the binary record in record. return 0 on success.

	uint8_t inputlen;
	uint8_t t, i, checksum_calc=0, checksum_read;

	//first check for ":" leading character
	if(input[0] != ':') return -1;

	//then check the string for only valid ASCII ['0'-'F']
	inputlen=1;
	while(input[inputlen]) {
		if( !isxdigit(input[inputlen++]) ) return -2;
	}

	//then read the length.
	record->length = (asc2nibble(input[1]) << 4) + asc2nibble(input[2]);
	if(input[(record->length<<1) + 11] != 0) return -3; //if we're missing a null terminator in the right place

	//then read the address.
	record->addr = (asc2nibble(input[3]) << 12) + (asc2nibble(input[4]) << 8) + (asc2nibble(input[5]) << 4) + asc2nibble(input[6]);

	//then read the record type.
	record->type = (asc2nibble(input[7]) << 4) + asc2nibble(input[8]);
//	if(record->type > 4) return -4;

	//then read the data, which goes from input[9] to input[9+length*2].
	for(i=0; i < record->length; i++) {
		t = 9 + (i<<1);
		record->data[i] = (asc2nibble(input[t]) << 4) + (asc2nibble(input[t + 1]));
		checksum_calc += record->data[i]; //might as well keep a running checksum as we read
	}
	checksum_calc += record->length + record->type + (record->addr >> 8) + (record->addr & 0xFF); //get the rest of the data into that checksum
	checksum_calc = ~checksum_calc + 1;	//checksum is 2's complement

	//now read the checksum of the record
	checksum_read = (asc2nibble(input[9 + (record->length<<1)]) << 4) + asc2nibble(input[10 + (record->length<<1)]);
	if(checksum_calc != checksum_read) return -5; //compare 'em

	return 0;
}
