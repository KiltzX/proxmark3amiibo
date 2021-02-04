//-----------------------------------------------------------------------------
// Iceman - August 2019
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// Routines to support a mangeled ISO 14443 type A for Thinfilm tags by Kovio
//-----------------------------------------------------------------------------

#ifndef __THINFILM_H
#define __THINFILM_H

#include "common.h"

void ReadThinFilm(void);
void SimulateThinFilm(uint8_t *data, size_t len);

#endif /* __ISO14443A_H */
