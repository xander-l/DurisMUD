/*
 * Copyright (c) 1995-2007, Michael Glosenger
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of Michael Glosenger may not be used to endorse or promote 
 *       products derived from this software without specific prior written 
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MICHAEL GLOSENGER ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
 * EVENT SHALL MICHAEL GLOSENGER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _OBJCRAFT_H_
#define _OBJCRAFT_H_

// craftsmanship values

#define OBJCRAFT_LOWEST          0
#define OBJCRAFT_TERRIBLE        0 // terribly made
#define OBJCRAFT_EXTREMELY_POOR  1 // extremely poorly made
#define OBJCRAFT_VERY_POOR       2 // very poorly made
#define OBJCRAFT_FAIRLY_POOR     3 // fairly poorly made
#define OBJCRAFT_WELL_BELOW      4 // well below average
#define OBJCRAFT_BELOW_AVG       5 // below average
#define OBJCRAFT_SLIGHTLY_BELOW  6 // slightly below average
#define OBJCRAFT_AVERAGE         7 // average
#define OBJCRAFT_SLIGHTLY_ABOVE  8 // slightly above average
#define OBJCRAFT_ABOVE_AVG       9 // above average
#define OBJCRAFT_WELL_ABOVE     10 // well above average
#define OBJCRAFT_EXCELLENT      11 // excellently made
#define OBJCRAFT_GOOD_ARTISAN   12 // clearly made by a skilled artisan
#define OBJCRAFT_VERY_ARTISAN   13 // clearly made by a very skilled artisan
#define OBJCRAFT_GREAT_ARTISAN  14 // clearly made by a master at the craft
#define OBJCRAFT_SPOOGEALICIOUS 15 // made by a true master - i.e. a master
                                   // dwarf artisian making chainmail, or
                                   // just some really spiffy guy making
                                   // great shit
#define OBJCRAFT_HIGHEST        15
#define NUMB_OBJCRAFT_VALS      16

#endif
