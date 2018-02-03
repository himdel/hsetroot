//////////////////////////////////////////////////////////////////////
/*

  Copyright (C) 2016 Martin Hradil
                2018 Pete Beardmore

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

  http://www.gnu.org/licenses/gpl-2.0.txt

*/
//////////////////////////////////////////////////////////////////////

#ifndef __OUTPUTS_H__
#define __OUTPUTS_H__

typedef struct {
  int idx;
  char* name;
  int w, h;
  int x, y;
} OutputInfo;

typedef struct {
  int noutputs;
  OutputInfo* infos;
} Outputs;

void outputs_set(Outputs*);
void outputs_print(OutputInfo);
OutputInfo *outputs_by_name(Outputs*, char*);
void outputs_free(Outputs*);

#endif	// __OUTPUTS_H__
