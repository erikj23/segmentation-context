 
// Base64 encoding (RFC1341)
// Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
// 
// This software may be distributed under the terms of the BSD license.

// 12 2016, Gaspard Petit: slightly modified to return a std::string 
// instead of a buffer allocated with malloc.

// 11 2019, Erik Maldonado: 
//	moved constants to .h file 
//	changed naming conventions to match project
// source: https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

// std
#include <string>

// custom
#include "base64.h"

// assumptions:
//	input: data to be encoded is not empty
//	input_length: length of the data to be encoded
// output: string containing encoded data or empty string on failure
std::string base64_encode(const unsigned char *input, size_t input_length)
{
	unsigned char *p_output, *p_position;
	const unsigned char *p_end, *p_input;
	
	// 3-byte blocks to 4-byte
	size_t output_length = 4 * ((input_length + 2) / 3);
	
	// return empty string on potential integer overflow
	if (output_length < input_length) return std::string();

	std::string output;
	output.resize(output_length);
	p_output = (unsigned char*)&output[0];

	p_end = input + input_length;
	p_input = input;
	p_position = p_output;

	while (p_end - p_input >= 3)
	{
		*p_position++ = base64_table[p_input[0] >> 2];
		*p_position++ = base64_table[((p_input[0] & 0x03) << 4) | (p_input[1] >> 4)];
		*p_position++ = base64_table[((p_input[1] & 0x0f) << 2) | (p_input[2] >> 6)];
		*p_position++ = base64_table[p_input[2] & 0x3f];
		p_input += 3;
	}

	if (p_end - p_input)
	{
		*p_position++ = base64_table[p_input[0] >> 2];
		if (p_end - p_input == 1)
		{
			*p_position++ = base64_table[(p_input[0] & 0x03) << 4];
			*p_position++ = '=';
		}
		else
		{
			*p_position++ = base64_table[((p_input[0] & 0x03) << 4) | (p_input[1] >> 4)];
			*p_position++ = base64_table[(p_input[1] & 0x0f) << 2];
		}
		*p_position++ = '=';
	}
	return output;
}