
// 11 2019, Erik Maldonado

#pragma once

// std
#include <string>

static const unsigned char base64_table[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// assumptions:
//	input: data to be encoded is not empty
//	input_length: length of the data to be encoded
// output: string containing encoded data or empty string on failure
std::string base64_encode(const unsigned char* input, size_t input_length);
