////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

This package contains a tiny version of the WavPack 4.40 decoder that might
be used in a "resource limited" CPU environment or form the basis for a
hardware decoding implementation. It is packaged with a demo command-line
program that accepts a WavPack audio file on stdin and outputs a RIFF wav
file to stdout. The program is standard C, and a win32 executable is
included which was compiled under MS Visual C++ 6.0 using this command:

cl /O1 /DWIN32 wvfilter.c wputils.c unpack.c float.c metadata.c words.c bits.c

WavPack data is read with a stream reading callback. No direct seeking is
provided for, but it is possible to start decoding anywhere in a WavPack
stream. In this case, WavPack will be able to provide the sample-accurate
position when it synchs with the data and begins decoding. The WIN32 macro
is used for Windows to force the stdin and stdout streams to be binary mode.

Compared to the previous version, this library has been optimized somewhat
for improved performance in exchange for slightly larger code size. The
library also now includes hand-optimized assembly language versions of the
decorrelation functions for both the ColdFire (w/EMAC) and ARM processors.

For demonstration purposes this uses a single static copy of the
WavpackContext structure, so obviously it cannot be used for more than one
file at a time. Also, this decoder will not handle "correction" files, plays
only the first two channels of multi-channel files, and is limited in
resolution in some large integer or floating point files (but always
provides at least 24 bits of resolution). It also will not accept WavPack
files from before version 4.0.

The previous version of this library would handle float files by returning
32-bit floating-point data (even though no floating point math was used).
Because this library would normally be used for simply playing WavPack
files where lossless performance (beyond 24-bits) is not relevant, I have
changed this behavior. Now, these files will generate clipped 24-bit data.
The MODE_FLOAT flag will still be returned by WavpackGetMode(), but the
BitsPerSample and BytesPerSample queries will be 24 and 3, respectfully.
What this means is that an application that can handle 24-bit data will
now be able to handle floating point data (assuming that the MODE_FLOAT
flag is ignored).

To make this code viable on the greatest number of hardware platforms, the
following are true:

   speed is about 5x realtime on an AMD K6 300 MHz
      ("high" mode 16/44 stereo; normal mode is about twice that fast)

   no floating-point math required; just 32b * 32b = 32b int multiply

   large data areas are static and less than 4K total
   executable code and tables are less than 40K
   no malloc / free usage

To maintain compatibility on various platforms, the following conventions
are used:

   a "char" must be exactly 8-bits
   a "short" must be exactly 16-bits
   an "int" must be at least 16-bits, but may be larger
   the "long" type is not used to avoid problems with 64-bit compilers

Questions or comments should be directed to david@wavpack.com
