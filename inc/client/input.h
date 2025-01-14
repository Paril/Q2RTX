/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef INPUT_H
#define INPUT_H

//
// input.h -- external (non-keyboard) input devices
//

bool IN_GetMouseMotion(int *dx, int *dy);
void IN_WarpMouse(int x, int y);
void IN_ShutdownMouse(void);
bool IN_InitMouse(void);
void IN_GrabMouse(bool grab);

void IN_Frame(void);
void IN_Activate(void);

#endif // INPUT_H
