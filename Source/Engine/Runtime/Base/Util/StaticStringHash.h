/*
 * This source file is part of the CrunchyBytes Game Development Kit.
 *
 * For the latest information, see http://n00body.squarespace.com/
 *
 * Copyright (c) 2011 CrunchyBytes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#pragma once

/** Generates integer IDs at compile time by hashing strings.

    Based on:
    http://www.humus.name/index.php?page=News&ID=296

    @remarks This class exploits two language details to get optimized out of
            the compiled code. Firstly, the use of array references lets the 
            compiler know the string's existence and size. Secondly, the lack
            of the @c explicit keyword allows a string literal to be converted 
            directly to a StringId.
            
    @author Joshua Ols <crunchy.bytes.blog@gmail.com>
    @date 2011-05-28
*/

inline U32 GetDynamicStringHash( const char* str )
{
	U32 hash = *str++;
	while( *str ) {
		hash = (hash * 65599u) + *str++;
	}
	return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[2])
{
    return str[0];
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[3])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[4])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[5])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[6])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[7])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[8])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[9])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[10])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[11])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[12])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[13])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[14])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[15])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[16])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[17])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[18])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[19])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[20])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[21])
{
    U32 hash = str[0];
    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
    return hash;
}


//#define HELPER_MACRO_HASH_COMBINE_1

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[22])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];

    return hash;
}


mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[23])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];

    return hash;
}



mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[24])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];

    return hash;
}


mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[25])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[26])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[27])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];

    return hash;
}


mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[28])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];

    return hash;
}


mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[29])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[30])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[31])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[32])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[33])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[34])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[35])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[36])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[37])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[38])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[39])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[40])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[41])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[42])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[43])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[44])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[45])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[46])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[47])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];
	hash = (hash * 65599u) + str[45];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[48])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];
	hash = (hash * 65599u) + str[45];
	hash = (hash * 65599u) + str[46];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[49])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];
	hash = (hash * 65599u) + str[45];
	hash = (hash * 65599u) + str[46];
	hash = (hash * 65599u) + str[47];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[50])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];
	hash = (hash * 65599u) + str[45];
	hash = (hash * 65599u) + str[46];
	hash = (hash * 65599u) + str[47];
	hash = (hash * 65599u) + str[48];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[51])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];
	hash = (hash * 65599u) + str[45];
	hash = (hash * 65599u) + str[46];
	hash = (hash * 65599u) + str[47];
	hash = (hash * 65599u) + str[48];
	hash = (hash * 65599u) + str[49];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[52])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];
	hash = (hash * 65599u) + str[45];
	hash = (hash * 65599u) + str[46];
	hash = (hash * 65599u) + str[47];
	hash = (hash * 65599u) + str[48];
	hash = (hash * 65599u) + str[49];
	hash = (hash * 65599u) + str[50];

    return hash;
}

mxFORCEINLINE
U32 GetStaticStringHash(
    const char (&str)[53])
{
    U32 hash = str[0];

    hash = (hash * 65599u) + str[1];
    hash = (hash * 65599u) + str[2];
    hash = (hash * 65599u) + str[3];
    hash = (hash * 65599u) + str[4];
    hash = (hash * 65599u) + str[5];
    hash = (hash * 65599u) + str[6];
    hash = (hash * 65599u) + str[7];
    hash = (hash * 65599u) + str[8];
    hash = (hash * 65599u) + str[9];
    hash = (hash * 65599u) + str[10];
    hash = (hash * 65599u) + str[11];
    hash = (hash * 65599u) + str[12];
    hash = (hash * 65599u) + str[13];
    hash = (hash * 65599u) + str[14];
    hash = (hash * 65599u) + str[15];
    hash = (hash * 65599u) + str[16];
    hash = (hash * 65599u) + str[17];
    hash = (hash * 65599u) + str[18];
    hash = (hash * 65599u) + str[19];
	hash = (hash * 65599u) + str[20];
	hash = (hash * 65599u) + str[21];
	hash = (hash * 65599u) + str[22];
	hash = (hash * 65599u) + str[23];
	hash = (hash * 65599u) + str[24];
	hash = (hash * 65599u) + str[25];
	hash = (hash * 65599u) + str[26];
	hash = (hash * 65599u) + str[27];
	hash = (hash * 65599u) + str[28];
	hash = (hash * 65599u) + str[29];
	hash = (hash * 65599u) + str[30];
	hash = (hash * 65599u) + str[31];
	hash = (hash * 65599u) + str[32];
	hash = (hash * 65599u) + str[33];
	hash = (hash * 65599u) + str[34];
	hash = (hash * 65599u) + str[35];
	hash = (hash * 65599u) + str[36];
	hash = (hash * 65599u) + str[37];
	hash = (hash * 65599u) + str[38];
	hash = (hash * 65599u) + str[39];
	hash = (hash * 65599u) + str[40];
	hash = (hash * 65599u) + str[41];
	hash = (hash * 65599u) + str[42];
	hash = (hash * 65599u) + str[43];
	hash = (hash * 65599u) + str[44];
	hash = (hash * 65599u) + str[45];
	hash = (hash * 65599u) + str[46];
	hash = (hash * 65599u) + str[47];
	hash = (hash * 65599u) + str[48];
	hash = (hash * 65599u) + str[49];
	hash = (hash * 65599u) + str[50];
	hash = (hash * 65599u) + str[51];

    return hash;
}
