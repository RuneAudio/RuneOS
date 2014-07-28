#ifndef _MD5_H_
#define _MD5_H_
/* MD5.H - header file for MD5C.C
 * Id: md5.h,v 1.6.2.1 1998/02/18 02:28:14 jkh Exp $
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/* Base functions from original file */
/* MD5 context. */
typedef struct MD5Context {
  unsigned int state[4];	/* state (ABCD) */
  unsigned int count[2];	/* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];	/* input buffer */
} MD5_CTX;

extern void   MD5Init (MD5_CTX *);
extern void   MD5Update (MD5_CTX *, const unsigned char *, unsigned int);
extern void   MD5Final (unsigned char [16], MD5_CTX *);

/* Convenience / utilities */
#include <string>
using std::string;
extern void MD5Final(string& digest, MD5_CTX *);
extern string& MD5String(const string& data, string& digest);
extern string& MD5HexPrint(const string& digest, string& xdigest);
extern string& MD5HexScan(const string& xdigest, string& digest);
#endif /* _MD5_H_ */
