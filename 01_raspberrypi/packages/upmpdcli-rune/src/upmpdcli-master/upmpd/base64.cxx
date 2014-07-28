/* Copyright (C) 2005 J.F.Dockes
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <stdio.h>

#include <sys/types.h>
#include <cstring>
#include <string>
using std::string;

#include "base64.hxx"

#undef DEBUG_BASE64 
#ifdef DEBUG_BASE64
#define DPRINT(X) fprintf X
#else
#define DPRINT(X)
#endif

// This is adapted from FreeBSD's code, quite modified for performance.
// Tests on a Mac pro 2.1G with a 166MB base64 file
//
// The original version used strchr to lookup the base64 value from
// the input code:
//   real    0m13.053s user  0m12.574s sys   0m0.471s
// Using a direct access, 256 entries table:
//   real    0m3.073s user   0m2.600s sys    0m0.439s
// Using a variable to hold the array length (instead of in.length()):
//   real    0m2.972s user   0m2.527s sys    0m0.433s
// Using values from the table instead of isspace() (final)
//   real    0m2.513s user   0m2.059s sys    0m0.439s
//
// The table has one entry per char value (0-256). Invalid base64
// chars take value 256, whitespace 255, Pad ('=') 254. 
// Valid char points contain their base64 value (0-63) 
static const int b64values[] = {
/* 0 */ 256,/* 1 */ 256,/* 2 */ 256,/* 3 */ 256,/* 4 */ 256,
/* 5 */ 256,/* 6 */ 256,/* 7 */ 256,/* 8 */ 256,
/*9 ht */ 255,/* 10 nl */ 255,/* 11 vt */ 255,/* 12 np/ff*/ 255,/* 13 cr */ 255,
/* 14 */ 256,/* 15 */ 256,/* 16 */ 256,/* 17 */ 256,/* 18 */ 256,/* 19 */ 256,
/* 20 */ 256,/* 21 */ 256,/* 22 */ 256,/* 23 */ 256,/* 24 */ 256,/* 25 */ 256,
/* 26 */ 256,/* 27 */ 256,/* 28 */ 256,/* 29 */ 256,/* 30 */ 256,/* 31 */ 256,
/* 32 sp  */ 255,
/* ! */ 256,/* " */ 256,/* # */ 256,/* $ */ 256,/* % */ 256,
/* & */ 256,/* ' */ 256,/* ( */ 256,/* ) */ 256,/* * */ 256,
/* + */ 62,
/* , */ 256,/* - */ 256,/* . */ 256,
/* / */ 63,
/* 0 */ 52,/* 1 */ 53,/* 2 */ 54,/* 3 */ 55,/* 4 */ 56,/* 5 */ 57,/* 6 */ 58,
/* 7 */ 59,/* 8 */ 60,/* 9 */ 61,
/* : */ 256,/* ; */ 256,/* < */ 256,
/* = */ 254,
/* > */ 256,/* ? */ 256,/* @ */ 256,
/* A */ 0,/* B */ 1,/* C */ 2,/* D */ 3,/* E */ 4,/* F */ 5,/* G */ 6,/* H */ 7,
/* I */ 8,/* J */ 9,/* K */ 10,/* L */ 11,/* M */ 12,/* N */ 13,/* O */ 14,
/* P */ 15,/* Q */ 16,/* R */ 17,/* S */ 18,/* T */ 19,/* U */ 20,/* V */ 21,
/* W */ 22,/* X */ 23,/* Y */ 24,/* Z */ 25,
/* [ */ 256,/* \ */ 256,/* ] */ 256,/* ^ */ 256,/* _ */ 256,/* ` */ 256,
/* a */ 26,/* b */ 27,/* c */ 28,/* d */ 29,/* e */ 30,/* f */ 31,/* g */ 32,
/* h */ 33,/* i */ 34,/* j */ 35,/* k */ 36,/* l */ 37,/* m */ 38,/* n */ 39,
/* o */ 40,/* p */ 41,/* q */ 42,/* r */ 43,/* s */ 44,/* t */ 45,/* u */ 46,
/* v */ 47,/* w */ 48,/* x */ 49,/* y */ 50,/* z */ 51,
/* { */ 256,/* | */ 256,/* } */ 256,/* ~ */ 256,
256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,256,
256,256,256,256,256,256,256,256,
};
static const char Base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

bool base64_decode(const string& in, string& out)
{
    int io = 0, state = 0, ch = 0;
    unsigned int ii = 0;
    out.clear();
    size_t ilen = in.length();
    out.reserve(ilen);

    for (ii = 0; ii < ilen; ii++) {
	ch = (unsigned char)in[ii];
	int value = b64values[ch];

	if (value == 255)        /* Skip whitespace anywhere. */
	    continue;
	if (ch == Pad64)
	    break;
	if (value == 256) {
	    /* A non-base64 character. */
	    DPRINT((stderr, "base64_dec: non-base64 char at pos %d\n", ii));
	    return false;
	}

	switch (state) {
	case 0:
	    out += value << 2;
	    state = 1;
	    break;
	case 1:
	    out[io]   |=  value >> 4;
	    out += (value & 0x0f) << 4 ;
	    io++;
	    state = 2;
	    break;
	case 2:
	    out[io]   |=  value >> 2;
	    out += (value & 0x03) << 6;
	    io++;
	    state = 3;
	    break;
	case 3:
	    out[io] |= value;
	    io++;
	    state = 0;
	    break;
	default:
	    fprintf(stderr, "base64_dec: internal!bad state!\n");
	    return false;
	}
    }

    /*
     * We are done decoding Base-64 chars.  Let's see if we ended
     * on a byte boundary, and/or with erroneous trailing characters.
     */

    if (ch == Pad64) {		/* We got a pad char. */
	ch = in[ii++];		/* Skip it, get next. */
	switch (state) {
	case 0:		/* Invalid = in first position */
	case 1:		/* Invalid = in second position */
	    DPRINT((stderr, "base64_dec: pad char in state 0/1\n"));
	    return false;

	case 2:		/* Valid, means one byte of info */
			/* Skip any number of spaces. */
	    for (; ii < in.length(); ch = in[ii++])
		if (!isspace((unsigned char)ch))
		    break;
	    /* Make sure there is another trailing = sign. */
	    if (ch != Pad64) {
		DPRINT((stderr, "base64_dec: missing pad char!\n"));
		// Well, there are bad encoders out there. Let it pass
		// return false;
	    }
	    ch = in[ii++];		/* Skip the = */
	    /* Fall through to "single trailing =" case. */
	    /* FALLTHROUGH */

	case 3:	    /* Valid, means two bytes of info */
	    /*
	     * We know this char is an =.  Is there anything but
	     * whitespace after it?
	     */
	    for (; ii < in.length(); ch = in[ii++])
		if (!isspace((unsigned char)ch)) {
		    DPRINT((stderr, "base64_dec: non-white at eod: 0x%x\n", 
			    (unsigned int)((unsigned char)ch)));
		    // Well, there are bad encoders out there. Let it pass
		    //return false;
		}

	    /*
	     * Now make sure for cases 2 and 3 that the "extra"
	     * bits that slopped past the last full byte were
	     * zeros.  If we don't check them, they become a
	     * subliminal channel.
	     */
	    if (out[io] != 0) {
		DPRINT((stderr, "base64_dec: bad extra bits!\n"));
		// Well, there are bad encoders out there. Let it pass
		out[io] = 0;
		// return false;
	    }
	    // We've appended an extra 0.
	    out.resize(io);
	}
    } else {
	/*
	 * We ended by seeing the end of the string.  Make sure we
	 * have no partial bytes lying around.
	 */
	if (state != 0) {
	    DPRINT((stderr, "base64_dec: bad final state\n"));
	    return false;
	}
    }

    DPRINT((stderr, "base64_dec: ret ok, io %d sz %d len %d value [%s]\n", 
	    io, (int)out.size(), (int)out.length(), out.c_str()));
    return true;
}

#undef Assert
#define Assert(X)

void base64_encode(const string &in, string &out)
{
    unsigned char input[3];
    unsigned char output[4];

    out.clear();

    int srclength = in.length();
    int sidx = 0;
    while (2 < srclength) {
	input[0] = in[sidx++];
	input[1] = in[sidx++];
	input[2] = in[sidx++];
	srclength -= 3;

	output[0] = input[0] >> 2;
	output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
	output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
	output[3] = input[2] & 0x3f;
	Assert(output[0] < 64);
	Assert(output[1] < 64);
	Assert(output[2] < 64);
	Assert(output[3] < 64);

	out += Base64[output[0]];
	out += Base64[output[1]];
	out += Base64[output[2]];
	out += Base64[output[3]];
    }
    
    /* Now we worry about padding. */
    if (0 != srclength) {
	/* Get what's left. */
	input[0] = input[1] = input[2] = '\0';
	for (int i = 0; i < srclength; i++)
	    input[i] = in[sidx++];
	
	output[0] = input[0] >> 2;
	output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
	output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
	Assert(output[0] < 64);
	Assert(output[1] < 64);
	Assert(output[2] < 64);

	out += Base64[output[0]];
	out += Base64[output[1]];
	if (srclength == 1)
	    out += Pad64;
	else
	    out += Base64[output[2]];
	out += Pad64;
    }
    return;
}

#ifdef TEST_BASE64
#include <stdio.h>
#include <stdlib.h>

#include "readfile.h"

const char *thisprog;
static char usage [] = "testfile\n\n"
;
static void
Usage(void)
{
    fprintf(stderr, "%s: usage:\n%s", thisprog, usage);
    exit(1);
}

static int     op_flags;
#define OPT_MOINS 0x1
#define OPT_i	  0x2 
#define OPT_P	  0x4 

int main(int argc, char **argv)
{
    thisprog = argv[0];
    argc--; argv++;

    while (argc > 0 && **argv == '-') {
	(*argv)++;
	if (!(**argv))
	    /* Cas du "adb - core" */
	    Usage();
	while (**argv)
	    switch (*(*argv)++) {
	    case 'i':	op_flags |= OPT_i; break;
	    default: Usage();	break;
	    }
	argc--; argv++;
    }
    
    if (op_flags & OPT_i)  {
	const char *values[] = {"", "1", "12", "123", "1234", 
				"12345", "123456"};
	int nvalues = sizeof(values) / sizeof(char *);
	string in, out, back;
	int err = 0;
	for (int i = 0; i < nvalues; i++) {
	    in = values[i];
	    base64_encode(in, out);
	    base64_decode(out, back);
	    if (in != back) {
		fprintf(stderr, "In [%s] %d != back [%s] %d (out [%s] %d\n", 
			in.c_str(), int(in.length()), 
			back.c_str(), int(back.length()),
			out.c_str(), int(out.length())
			);
		err++;
	    }
	}
	in.erase();
	in += char(0);
	in += char(0);
	in += char(0);
	in += char(0);
	base64_encode(in, out);
	base64_decode(out, back);
	if (in != back) {
	    fprintf(stderr, "In [%s] %d != back [%s] %d (out [%s] %d\n", 
		    in.c_str(), int(in.length()), 
		    back.c_str(), int(back.length()),
		    out.c_str(), int(out.length())
		    );
	    err++;
	}
	exit(!(err == 0));
    } else {
	if (argc > 1)
	    Usage();
	string infile;
	if (argc == 1)
	    infile = *argv++;argc--;
	string idata, reason;
	if (!file_to_string(infile, idata, &reason)) {
	    fprintf(stderr, "Can't read file: %s\n", reason.c_str());
	    exit(1);
	}
	string odata;
	if (!base64_decode(idata, odata)) {
	    fprintf(stderr, "Decoding failed\n");
	    exit(1);
	}
	write(1, odata.c_str(), 
	      odata.size() * sizeof(string::value_type));
	exit(0);
    }
}
#endif
