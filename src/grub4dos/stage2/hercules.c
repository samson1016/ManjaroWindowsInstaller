/* hercules.c - hercules console interface */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001,2002  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef SUPPORT_HERCULES

#include <shared.h>
#include <hercules.h>
#include <term.h>

/* Write a byte to a port.  */
static inline void
outb (unsigned short port, unsigned char value)
{
  asm volatile ("outb	%b0, %w1" : : "a" (value), "Nd" (port));
}

static void
herc_set_cursor (void)
{
  unsigned offset = fonty * HERCULES_WIDTH + fontx;
  
  outb (HERCULES_INDEX_REG, 0x0f);
  outb (0x80, 0);
  outb (HERCULES_DATA_REG, offset & 0xFF);
  outb (0x80, 0);
  
  outb (HERCULES_INDEX_REG, 0x0e);
  outb (0x80, 0);
  outb (HERCULES_DATA_REG, offset >> 8);
  outb (0x80, 0);
}

unsigned int
hercules_putchar (unsigned int c, unsigned int max_width)
{
  switch (c)
    {
    case '\b':
      if (fontx > 0)
	fontx--;
      break;
      
    case '\n':
      fonty++;
      break;
      
    case '\r':
      fontx = 0;
      break;

    case '\a':
      break;

    default:
      {
	volatile unsigned short *video
	  = (unsigned short *) HERCULES_VIDEO_ADDR;
	
	video[fonty * HERCULES_WIDTH + fontx] = (current_color << 8) | c;
	fontx++;
	if (fontx >= HERCULES_WIDTH)
	  {
	    fontx = 0;
	    fonty++;
	  }
      }
      break;
    }

  if (fonty >= HERCULES_HEIGHT)
    {
      volatile unsigned long *video = (unsigned long *) HERCULES_VIDEO_ADDR;
      int i;
      
      fonty = HERCULES_HEIGHT - 1;
      grub_memmove ((char *) HERCULES_VIDEO_ADDR,
		    (char *) HERCULES_VIDEO_ADDR + HERCULES_WIDTH * 2,
		    HERCULES_WIDTH * (HERCULES_HEIGHT - 1) * 2);
      for (i = HERCULES_WIDTH * (HERCULES_HEIGHT - 1) / 2;
	   i < HERCULES_WIDTH * HERCULES_HEIGHT / 2;
	   i++)
	video[i] = 0x07200720;
    }
  return 1;
}

void
hercules_cls (void)
{
  int i;
  volatile unsigned long *video = (unsigned long *) HERCULES_VIDEO_ADDR;
  
  for (i = 0; i < HERCULES_WIDTH * HERCULES_HEIGHT / 2; i++)
    video[i] = 0x07200720;

  fontx = fonty = 0;
  herc_set_cursor ();
}

int
hercules_getxy (void)
{
  return (fonty << 8) | fontx;
}

void
hercules_gotoxy (int x, int y)
{
  fontx = x;
  fonty = y;
  herc_set_cursor ();
}

void
hercules_setcursor (unsigned long on)
{
  outb (HERCULES_INDEX_REG, 0x0a);
  outb (0x80, 0);
  outb (HERCULES_DATA_REG, on ? 0 : (1 << 5));
  outb (0x80, 0);

  return;
}

#endif /* SUPPORT_HERCULES */
