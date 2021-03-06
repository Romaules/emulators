/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <FrameBuffer.hxx>
#include <emuframework/EmuApp.hh>
#include <stella/emucore/TIA.hxx>

extern uint16 tiaColorMap[256], tiaPhosphorColorMap[256][256];

void FrameBuffer::showMessage(const string& message, int position, bool force, uInt32 color)
{
	popup.printf(3, false, "%s", message.c_str());
}

void FrameBuffer::enablePhosphor(bool enable, int blend)
{
	myUsePhosphor = enable;
	myPhosphorBlend = blend;
}

uint8 FrameBuffer::getPhosphor(uInt8 c1, uInt8 c2) const
{
  if(c2 > c1)
    std::swap(c1, c2);

  return ((c1 - c2) * myPhosphorBlend)/100 + c2;
}

void FrameBuffer::setPalette(const uInt32* palette)
{
	logMsg("setTIAPalette");
	iterateTimes(256, i)
	{
		uint8 r = (palette[i] >> 16) & 0xff;
		uint8 g = (palette[i] >> 8) & 0xff;
		uint8 b = palette[i] & 0xff;

		// TODO: RGB 565
		tiaColorMap[i] = IG::PIXEL_DESC_RGB565.build(r >> 3, g >> 2, b >> 3, 0);
	}

	iterateTimes(256, i)
	{
		iterateTimes(256, j)
		{
			uint8 ri = (palette[i] >> 16) & 0xff;
			uint8 gi = (palette[i] >> 8) & 0xff;
			uint8 bi = palette[i] & 0xff;
			uint8 rj = (palette[j] >> 16) & 0xff;
			uint8 gj = (palette[j] >> 8) & 0xff;
			uint8 bj = palette[j] & 0xff;

			uint8 r = getPhosphor(ri, rj);
			uint8 g = getPhosphor(gi, gj);
			uint8 b = getPhosphor(bi, bj);

			// TODO: RGB 565
			tiaPhosphorColorMap[i][j] = IG::PIXEL_DESC_RGB565.build(r >> 3, g >> 2, b >> 3, 0);
		}
	}
}

void FrameBuffer::render(uInt16 *pixBuff, TIA &tia)
{
	assert(tia.height() <= 320);
	uint h = tia.height();
	if(myUsePhosphor)
	{
		uint8* currentFrame = tia.currentFrameBuffer();
		uint8* prevFrame = tia.previousFrameBuffer();
		iterateTimes(160 * h, i)
		{
			pixBuff[i] = tiaPhosphorColorMap[currentFrame[i]][prevFrame[i]];
		}
	}
	else
	{
		uint8* currentFrame = tia.currentFrameBuffer();
		iterateTimes(160 * h, i)
		{
			pixBuff[i] = tiaColorMap[currentFrame[i]];
		}
	}
}
