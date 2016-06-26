/***************************************************************************
 *   Copyright (C) 2000-2008 by Johan Maes                                 *
 *   on4qz@telenet.be                                                      *
 *   http://users.telenet.be/on4qz                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef MODEGBR_H
#define MODEGBR_H

#include "modebase.h"

/**
	@author Johan Maes <on4qz@telenet.be>
*/
class modeGBR : public modeBase
{
public:
  modeGBR(esstvMode m, unsigned int len, bool tx, bool narrowMode);
	~modeGBR();
protected:
  embState rxSetupLine();
	void calcPixelPositionTable(unsigned int colorLine,bool tx);
	void setupParams(double clock);
  embState txSetupLine();
};


#endif
