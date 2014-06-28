/*******************************************************************

Base64 Encoding/Decoding Library

Author: Marcin Kelar ( marcin.kelar@holicon.pl )
 *******************************************************************/
#ifndef BASE64_H
#define BASE64_H

#include <stdio.h>
#include "internal.h"

int b64_encode_string(const char *in, int in_len, char *out, int out_size);
int b64_decode_string(const char *in, char *out, int out_size);
int b64_selftest(void);
#endif
