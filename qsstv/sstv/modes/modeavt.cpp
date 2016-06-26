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
#include "modeavt.h"
#include "configparams.h"
// one number is 1 startbit + 16 databits.
#define WORDTIME (5.3108/32.)
#define BITTIME (WORDTIME/17)

modeAVT::modeAVT(esstvMode m,unsigned int len, bool tx,bool narrowMode):modeBase(m,len,tx,narrowMode)
{
  avtTrailerDetect=true;
  trailerState=D1900;
  code=0;
  duration=0;
}


modeAVT:: ~modeAVT()
{
}


void modeAVT::setupParams(double clock)
{
  visibleLineLength=(getLineLength(mode,clock))/3.;
}


modeBase::eModeBase modeAVT::process(quint16 *demod,unsigned int syncPos,bool goToSync)
{
  unsigned int i=0;
  unsigned char a,b;
  if(goToSync)
    {
      if(syncPos >=length)
        {
          addToLog(QString("modebase:process: syncPos: %1 > length %2").arg(syncPos).arg(length),LOGMODES);
          return MBENDOFIMAGE;
        }
      else
        {
            for(i=0;i<syncPos;i++)  debugStatePtr[i]=debugState;
        }
    }
  if(avtTrailerDetect)
  {
      for(;i<length;i++)
        {
          duration++;
          sample=demod[i];
          debugStatePtr[i]=debugState;
          avgSample+=0.9*(sample-avgSample);
          switch(trailerState)
            {
                case D1900:
                  if(fabs(avgSample-1900.) <50.)
                    {
                      debugState=st1900B;
                      count++;
                    }
                  else
                    {
                      if (count >0) count --;
                      else duration=0;
                    }
                  if(count>50)
                    {
                      count=10;
                      trailerState=D1900END;
                    }
                break;
                case D1900END:
                  if(fabs(avgSample-1900.) <50.)
                    {
                      if(count<10) count++;
                    }
                  else
                    {
                      debugState=st1900E;
                      if (count >0) count --;
                    }
                  if (count==0)
                    {
                      duration-=5;
                      if((duration<(unsigned int)(0.011*rxClock/SUBSAMPLINGFACTOR)) && (duration>(unsigned int)(0.009*rxClock/SUBSAMPLINGFACTOR)))
                        {
                          bitCounter=0;
                          count=10;
                          trailerState=DELAYHALF;
                        }
                       else
                        {
                         duration=0;
                         trailerState=D1900;
                        }
                    }
                break;
                case DELAYHALF:
                  debugState=stHALF;
                  count++;
                  code=0;
                  if(count>=(unsigned int)(round((BITTIME/2)*rxClock/SUBSAMPLINGFACTOR))) trailerState=BITS;
                break;
                case BITS:
                  debugState=stBITS+bitCounter;
                  code=code<<1;
                  if(avgSample>1900.) code|=0x0001;
                  bitCounter++;
                  if (bitCounter==16) trailerState=CALCDELAY;
                  else
                    {
                      count=0;
                      trailerState=DELAYFULL;
                    }
                break;
                case DELAYFULL:
                  debugState=stFULL;
                  count++;
                  if(count>=(unsigned int)(round(BITTIME*rxClock/SUBSAMPLINGFACTOR))) trailerState=BITS;
                break;
                case CALCDELAY:
                  //check if
                  a=code>>8;
                  b=(code&0xFF)^0xFF;
                  addToLog(QString("avtcode =%1 mode=%1,pos=%1").arg(QString::number(code,16)).arg((code&0xE000)>>13).arg((code&0x1F00)>>8),LOGMODES);
                  count=0;
                  duration=0;
                  if(a!=b)
                    {
                      trailerState=D1900;
                      break;
                    }
                   a&=0x1F;
                   delay=(unsigned int)(((31-a)*WORDTIME+BITTIME/2)*rxClock/SUBSAMPLINGFACTOR);
                   trailerState=WAITSTART;
                break;
                case WAITSTART:
                  debugState=stWAIT;
                  delay--;
                  if(delay==0)
                    {
                      avtTrailerDetect=false;
                      debugState=stColorLine0;
                    return modeBase::process(demod,i,true);
                    }
                break;
            }
        }

  }


  else
    {
      return modeBase::process(demod);
    }
  return MBRUNNING;
}



modeBase::embState modeAVT::rxSetupLine()
{	
  start=lineTimeTableRX[lineCounter];
//	addToLog(QString("modeAVT: subLine %1").arg(subLine),LOGMODES);
	
  switch(subLine)
		{
			case 0:
        calcPixelPositionTable(REDLINE,false);
        pixelArrayPtr=redArrayPtr;
				return MBPIXELS;
      case 1:
        calcPixelPositionTable(GREENLINE,false);
        pixelArrayPtr=greenArrayPtr;
				return MBPIXELS;
      case 2:
        calcPixelPositionTable(BLUELINE,false);
        pixelArrayPtr=blueArrayPtr;
				return MBPIXELS;
			break;
			default:
				return MBENDOFLINE;
		}
}

void modeAVT::calcPixelPositionTable(unsigned int colorLine,bool tx)
{
	unsigned int i;
	int ofx=0;
	if(tx) ofx=1;
	debugState=stColorLine0+colorLine;
	
	switch (colorLine)
		{
      case REDLINE:

			break;
      case GREENLINE:
        start+=(visibleLineLength);
			break;
      case BLUELINE:
        start+=(2.*visibleLineLength);
			break;
		}
  for(i=0;i<activeSSTVParam->numberOfPixels;i++)
		{
      pixelPositionTable[i]=(unsigned int)round(start+(((float)(i+ofx)*visibleLineLength)/activeSSTVParam->numberOfPixels));
		}
}


modeBase::embState modeAVT::txSetupLine()
{
  switch(subLine)
		{
			case 0:
				calcPixelPositionTable(GREENLINE,true);
				pixelArrayPtr=greenArrayPtr;
				return MBPIXELS;
			case 1:
				txFreq=1500.;
				txDur=(unsigned int)rint(blank);
				return MBTXGAP;
			case 2:
				calcPixelPositionTable(BLUELINE,true);
				pixelArrayPtr=blueArrayPtr;
				return MBPIXELS;
			case 3:
				txFreq=1500.;
				txDur=(unsigned int)rint(blank);
				return MBTXGAP;
			case 4:
				calcPixelPositionTable(REDLINE,true);
				pixelArrayPtr=redArrayPtr;
				return MBPIXELS;
			case 5:
				txFreq=1500;
				txDur=(unsigned int)rint(fp);
				return MBTXGAP;
			case 6:
        txFreq=syncFreq;
				txDur=(unsigned int)rint(syncDuration);
				return MBTXGAP;
			default:
				return MBENDOFLINE;
		}
}

