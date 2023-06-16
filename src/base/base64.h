/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_BASE64_H
#define BASE_BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

/*
	Function: base64_encode
		Encodes data as a base64 string (RFC 4648).

	Parameters:
		data - The data to be encoded.
		len - Size of the data to be encoded.

	Returns:
		The base64 encoded data in a newly allocated string buffer.

	Remarks:
		- The string is zero-terminated.
		- The result must be freed after it has been used.
*/
char *base64_encode(const unsigned char *data, unsigned len);

/*
	Function: base64_decode
		Decodes data from a base64 string (RFC 4648).

	Parameters:
		data - Pointer that will receive the decoded data.
		str - The base64 string to be decoded.

	Returns:
		The size of the decoded data.

	Remarks:
		- The string must be zero-terminated.
		- The data is zero-terminated for convenience when decoding strings.
		- The returned size does not include the size of the zero-terminator.
		- Any non-base64-alphabet characters in the string are ignored,
		  including whitespace and misplaced padding characters.
		- The result must be freed after it has been used.
*/
unsigned base64_decode(unsigned char **data, const char *str);

#ifdef __cplusplus
}
#endif

#endif
