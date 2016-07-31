/*
 *  ISO 9660 filesystem backend for GRUB (GRand Unified Bootloader)
 *  including Rock Ridge Extensions support
 *
 *  Copyright (C) 1998, 1999  Kousuke Takai  <tak@kmc.kyoto-u.ac.jp>
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
/*
 *  References:
 *	linux/fs/isofs/rock.[ch]
 *	mkisofs-1.11.1/diag/isoinfo.c
 *	mkisofs-1.11.1/iso9660.h
 *		(all are written by Eric Youngdale)
 *
 *  Modifications by:
 *	Leonid Lisovskiy   <lly@pisem.net>	2003
 */

#ifdef FSYS_ISO9660

#include "shared.h"
#include "filesys.h"
#include "iso9660.h"

/* iso9660 super-block data in memory */
struct iso_sb_info {
  unsigned long vol_sector;

};

/* iso fs inode data in memory */
struct iso_inode_info {
  unsigned long file_start;
};

#define ISO_SUPER	\
    ((struct iso_sb_info *)(FSYS_BUF))
#define INODE		\
    ((struct iso_inode_info *)(FSYS_BUF+sizeof(struct iso_sb_info)))
#define PRIMDESC        ((struct iso_primary_descriptor *)(FSYS_BUF + 2048))
//#define DIRREC          ((struct iso_directory_record *)(FSYS_BUF + 4096))
//#define RRCONT_BUF      ((unsigned char *)(FSYS_BUF + 6144))
//#define NAME_BUF        ((unsigned char *)(FSYS_BUF + 8192))
#define UDF_DESC				((struct udf_descriptor *)(FSYS_BUF + 2048))
#define RRCONT_BUF      ((unsigned char *)(FSYS_BUF + 4096))
#define NAME_BUF        ((unsigned char *)(FSYS_BUF + 6144))
#define DIRREC					((struct iso_directory_record *)(FSYS_BUF + 8192))
#define UDF_DIRREC   		((struct udf_descriptor *)(FSYS_BUF + 8192))

int iso_type;		//0/1/2/3=ISO_TYPE_9660/ISO_TYPE_udf/ISO_TYPE_Joliet/ISO_TYPE_RockRidge
unsigned long udf_partition_start;

#if 0
static int
iso9660_devread (unsigned long sector, unsigned long byte_offset, unsigned long byte_len, char *buf)
{
  unsigned short sector_size_lg2 = log2_tmp(buf_geom.sector_size);

  /*
   * We have to use own devread() function since BIOS return wrong geometry
   */
//  if (sector < 0)
//    {
//      errnum = ERR_OUTSIDE_PART;
//      return 0;
//    }
  if (byte_len <= 0)
    return 1;

  sector += (byte_offset >> sector_size_lg2);
  byte_offset &= (buf_geom.sector_size - 1);
  asm volatile ("shl%L0 %1,%0"
		: "=r"(sector)
		: "Ic"((int8_t)(ISO_SECTOR_BITS - sector_size_lg2)),
		"0"(sector));

  if (disk_read_hook && debug)
    printf ("<%d, %d, %d>", sector, byte_offset, byte_len);

  return rawread (current_drive, part_start + sector, byte_offset, byte_len, buf);
}
#endif

int
iso9660_mount (void)
{
	unsigned long sector, size, extent;
	struct iso_directory_record *idr;
	idr = &PRIMDESC->root_directory_record;

  /*
   *  Because there is no defined slice type ID for ISO-9660 filesystem,
   *  this test will pass only either (1) if entire disk is used, or
   *  (2) if current partition is BSD style sub-partition whose ID is
   *  ISO-9660.
   */
//  if ((current_partition != 0xFFFFFF)
//      && !IS_PC_SLICE_TYPE_BSD_WITH_FS(current_slice, FS_ISO9660))
//    return 0;

  /*
   *  Currently, only FIRST session of MultiSession disks are supported !!!
   */
//  for (sector = 16 ; sector < 32 ; sector++)
//   {
//      emu_iso_sector_size_2048 = 1;
//      if (! devread(sector, 0, sizeof(*PRIMDESC), (unsigned long long)(unsigned int)(char *)PRIMDESC, 0xedde0d90)) 
//	break;
//      /* check ISO_VD_PRIMARY and ISO_STANDARD_ID */
//      if (PRIMDESC->type.l == ISO_VD_PRIMARY
//	  && ! memcmp ((char *)(PRIMDESC->id), ISO_STANDARD_ID, sizeof(PRIMDESC->id)))
//	{
//	  ISO_SUPER->vol_sector = sector;
//	  INODE->file_start = 0;
//	  fsmax = PRIMDESC->volume_space_size.l;
//	  return 1;
//	}
//    }
//
//  return 0;
//}
    iso_type = ISO_TYPE_9660;	//0
		udf_partition_start = 0;
	//Test UDF system
	for (sector = 16 ; sector < 32 ; sector++)
 	{
 		emu_iso_sector_size_2048 = 1;
  	if (! devread(sector, 0, 0x100, (unsigned long long)(unsigned int)(char *)PRIMDESC, 0xedde0d90))
			return 0;
  	//Check UDF_STANDARD_ID
	if ( (iso_types & (1<<ISO_TYPE_udf)) && ! memcmp ((char *)(PRIMDESC->id), UDF_STANDARD_ID, 5))	//UDF_STANDARD_ID="BEA01"
	{
		iso_type = ISO_TYPE_udf;
	  	INODE->file_start = 0;
			break;
		}
	}   
	if (iso_type == ISO_TYPE_udf)
	{
		sector = 0x100;
		//The reading anchor Volume Descriptor Pointer
        emu_iso_sector_size_2048 = 1;
		devread(sector, 0, 0x800, (unsigned long long)(unsigned int)(char *)UDF_DESC, 0xedde0d90);
		if (UDF_DESC->Tag != UDF_Anchor)
			return 0;
		sector = UDF_DESC->AnchorVolume_MainVolume_ExtentLocation;
		for (;;)	//Reading partition descriptor, file set descriptor, file entry descriptor
		{
			emu_iso_sector_size_2048 = 1;
			devread(sector, 0, 0x800, (unsigned long long)(unsigned int)(char *)UDF_DESC, 0xedde0d90);
			if (UDF_DESC->Tag == UDF_FileEntry)  //Be found in the root directory of the file entry is no longer cycle
				break;
			switch (UDF_DESC->Tag)
			{
				case UDF_Partition:	//Partition descriptor
					udf_partition_start = UDF_DESC->Partition_PartitionStartingLocation;	//Partition start (the relative logical sector base address)
					fsmax = UDF_DESC->Partition_PartitionLength;				//Volume space size
					sector = udf_partition_start;
					break;
				case UDF_FileSet: //File Set Descriptor
					sector = UDF_DESC->FileSet_RootDirectoryLocation + udf_partition_start;//The root directory of the file entry
					break;
				default:
					sector++;
					break;	
			}
		}
		return 1;
	}
	else
	{
		for (sector = 17 ; sector < 32 ; sector++)
	  {
	  	emu_iso_sector_size_2048 = 1;
			devread(sector, 0, 0x800, (unsigned long long)(unsigned int)(char *)PRIMDESC, 0xedde0d90);
		if ((iso_types & (1<<ISO_TYPE_Joliet)) && (PRIMDESC->type.l == ISO_VD_ENHANCED)
	  		&& (! memcmp ((char *)(PRIMDESC->id), ISO_STANDARD_ID, 5)) && (*(unsigned short *)((char *)PRIMDESC  + 0x58) == 0x2F25))	//ISO_STANDARD_ID="CD001"
			{
				ISO_SUPER->vol_sector = sector;
	  		INODE->file_start = 0;
	  		fsmax = PRIMDESC->volume_space_size.l;
				iso_type = ISO_TYPE_Joliet;	//iso9600_Joliet
				extent = idr->extent.l;
				return 1;
			}
	  	if ((PRIMDESC->type.l == ISO_VD_END)
	  		&& ! memcmp ((char *)(PRIMDESC->id), ISO_STANDARD_ID, 5))	//ISO_VD_END=255=end
			break;
		}
		emu_iso_sector_size_2048 = 1;
		devread(16, 0, 0x800, (unsigned long long)(unsigned int)(char *)PRIMDESC, 0xedde0d90);
    size = idr->size.l;
		extent = idr->extent.l;
    emu_iso_sector_size_2048 = 1;
    devread (extent, 0, size, (unsigned long long)(unsigned int)(char *)DIRREC, 0xedde0d90);
    idr = (struct iso_directory_record *)DIRREC;
    idr = (struct iso_directory_record *)((char *)idr + idr->length.l);
    idr = (struct iso_directory_record *)((char *)idr + idr->length.l);
    if ((iso_types & (1<<ISO_TYPE_RockRidge)) && (idr->length.l - idr->name_len.l - sizeof(struct iso_directory_record) + sizeof(idr->name)) > 1)
			iso_type = ISO_TYPE_RockRidge; //iso9600_RockRidge
 		if ((PRIMDESC->type.l == ISO_VD_PRIMARY)
	  	&& ! memcmp ((char *)(PRIMDESC->id), ISO_STANDARD_ID, 5))
		{
        ISO_SUPER->vol_sector = 16;
	  	INODE->file_start = 0;
	  	fsmax = PRIMDESC->volume_space_size.l;
			return 1;
		}
		return 0;
	}
}

int
iso9660_dir (char *dirname)
{
  struct iso_directory_record *idr;
  RR_ptr_t rr_ptr;
  struct rock_ridge *ce_ptr;
  unsigned long pathlen;
  unsigned long size;
  unsigned long extent;
  unsigned char file_type;
  unsigned long rr_len;
  unsigned char rr_flag;
  char tmp_name[256];
  char *ch;
	unsigned int name_len;
	unsigned char *utf8 = (unsigned char *) RRCONT_BUF;
	char *name;
	int j, k;
	struct udf_descriptor *idr_udf_105;
	struct udf_FileIdentifier *idr_udf_101;
	char tmp_name1[256];
	int name_offset;

  idr = &PRIMDESC->root_directory_record;
  idr_udf_105 = (struct udf_descriptor *)UDF_DESC;
  INODE->file_start = 0;

  do
    {
      while (*dirname == '/')	/* skip leading slashes */
	dirname++;
      /* pathlen = strcspn(dirname, "/\n\t "); */
//      for (pathlen = 0 ;
//	   dirname[pathlen]
//	     && !isspace(dirname[pathlen]) && dirname[pathlen] != '/' ;
//	   pathlen++)
//	;
      for (ch = tmp_name;*dirname;++dirname)
	{
		if (*dirname == '\\')
				{
					*ch++ = *dirname++;
					*ch++ = *dirname++;
				}	
				if (/*isspace(*dirname) || */*dirname == '/')
					break;
		if (ch - tmp_name >= 255 || !(*ch = *dirname))
			break;
		++ch;
	}
	*ch = 0;
	pathlen = ch - tmp_name;
	
	if (iso_type == ISO_TYPE_udf)
	{	
//		unsigned long *tmp = (unsigned long *)(&UDF_DESC->FileEntry_BaseAddress + UDF_DESC->FileEntry_LengthofExtendedAttributes);
		unsigned long *tmp = (unsigned long *)(&idr_udf_105->FileEntry_BaseAddress + idr_udf_105->FileEntry_LengthofExtendedAttributes);
		size = *tmp;
		extent = *(tmp + 1) + udf_partition_start;
	}
	else
	{	
      size = idr->size.l;
      extent = idr->extent.l;
  }
	name_offset=0;

      while (size > 0)
	{
			emu_iso_sector_size_2048 = 1;
			if (! devread (extent, 0, ISO_SECTOR_SIZE*2, (unsigned long long)(unsigned int)(char *)DIRREC, 0xedde0d90))	
			{
	      errnum = ERR_FSYS_CORRUPT;
	      return 0;
	    }
			extent++;
			idr = (struct iso_directory_record *)DIRREC;
			idr_udf_101 = (struct udf_FileIdentifier *)((char *)DIRREC+name_offset);

		for (; ((iso_type == ISO_TYPE_udf)?(idr_udf_101->Tag != 0):(idr->length.l > 0)); )
	  {
	      if (iso_type == ISO_TYPE_udf)
			{		
				name_len = idr_udf_101->NameLength;		
				if (name_len == 0) 
					goto ssss;
				name_len--;
				name = (char *)(&idr_udf_101->NameBaseAddress + idr_udf_101->LengthofImplementationUse);
	  		if (name[0] == 8)
	  		{
	  			name++;  			
	  			grub_memcpy(utf8, name, name_len);	  			
	  		}	
	  		else if (name[0] == 16)
	  		{
					name++;
					big_to_little (name, name_len);
	  			name_len = unicode_to_utf8 ((unsigned short *)name, utf8, (unsigned long)(name_len/2));		
				}
				name = (char *)utf8;
				file_type = (idr_udf_101->FileCharacteristics & 2) ? ISO_DIRECTORY : ISO_REGULAR;
			}		
			else
			{	      
//	      const char *name = (const char *)(idr->name);
//	      unsigned int name_len = idr->name_len.l;
				name_len = idr->name_len.l;
	 			name = (char *)(idr->name);

	      file_type = (idr->flags.l & 2) ? ISO_DIRECTORY : ISO_REGULAR;
	    }
	    if (iso_type != ISO_TYPE_udf)
			{
	      if (name_len == 1)
		{
		  if ((name[0] == 0) ||	/* self */
		      (name[0] == 1)) 	/* parent */
//		    continue;
				goto ssss;
		}
		if (iso_type == ISO_TYPE_Joliet)
		{
			big_to_little (name, name_len);
			name_len = unicode_to_utf8 ((unsigned short *)name, utf8, (unsigned long)(name_len/2));
			name = (char *)utf8;
			goto dddd;
		}

	      if (name_len > 2 && CHECK2(name + name_len - 2, ';', '1'))
		{
		  name_len -= 2;	/* truncate trailing file version */
		  if (name_len > 1 && name[name_len - 1] == '.')
		    name_len--;		/* truncate trailing dot */
		}
		if (iso_type == ISO_TYPE_9660)	//0
			goto dddd;	
	      /*
	       *  Parse Rock-Ridge extension
	       */
	      rr_len = (idr->length.l - idr->name_len.l
			- sizeof(struct iso_directory_record)
			+ sizeof(idr->name));
	      rr_ptr.ptr = (char *)(((unsigned char *)idr + idr->name_len.l
			    + sizeof(struct iso_directory_record)
			    - sizeof(idr->name)));
	      if (rr_ptr.i & 1)
		rr_ptr.i++, rr_len--;
	      ce_ptr = NULL;
	      rr_flag = RR_FLAG_NM | RR_FLAG_PX /*| RR_FLAG_SL*/;

	      while (rr_len >= 4)
		{
		  if (rr_ptr.rr->version != 1)
		    {
		      if (((unsigned long)debug) >= 0x7FFFFFFF)
			printf(
			       "Non-supported version (%d) RockRidge chunk "
			       "`%c%c'\n", rr_ptr.rr->version,
			       (unsigned long)(unsigned char)rr_ptr.rr->signature,
			       (unsigned long)(unsigned char)(rr_ptr.rr->signature >> 8));
		      rr_flag = 0;
		    }
		  else
		    {
		      switch (rr_ptr.rr->signature)
			{
			case RRMAGIC('R', 'R'):
			  if ( rr_ptr.rr->len >= (4+sizeof(struct RR)))
			    rr_flag &= rr_ptr.rr->u.rr.flags.l;
			  break;
			case RRMAGIC('N', 'M'):
			  name = (char *)(rr_ptr.rr->u.nm.name);
			  name_len = rr_ptr.rr->len - (4+sizeof(struct NM));
			  rr_flag &= ~RR_FLAG_NM;
			  break;
			case RRMAGIC('P', 'X'):
			  if (rr_ptr.rr->len >= (4+sizeof(struct PX)))
			    {
			      file_type = ((rr_ptr.rr->u.px.mode.l & POSIX_S_IFMT)
					   == POSIX_S_IFREG
					   ? ISO_REGULAR
					   : ((rr_ptr.rr->u.px.mode.l & POSIX_S_IFMT)
					      == POSIX_S_IFDIR
					      ? ISO_DIRECTORY : ISO_OTHER));
			      rr_flag &= ~RR_FLAG_PX;
			    }
			  break;
			case RRMAGIC('C', 'E'):
			  if (rr_ptr.rr->len >= (4+sizeof(struct CE)))
			    ce_ptr = rr_ptr.rr;
			  break;
#if 0		// RockRidge symlinks are not supported yet
			case RRMAGIC('S', 'L'):
			  {
			    int slen;
			    unsigned char rootflag, prevflag;
			    char *rpnt = NAME_BUF+1024;
			    struct SL_component *slp;

			    slen = rr_ptr.rr->len - (4+1);
			    slp = &rr_ptr.rr->u.sl.link;
			    while (slen > 1)
			      {
				rootflag = 0;
				switch (slp->flags.l)
				  {
				  case 0:
				    memcpy(rpnt, slp->text, slp->len);
				    rpnt += slp->len;
				    break;
				  case 4:
				    *rpnt++ = '.';
				    /* fallthru */
				  case 2:
				    *rpnt++ = '.';
				    break;
				  case 8:
				    rootflag = 1;
				    *rpnt++ = '/';
				    break;
				  default:
				    printf("Symlink component flag not implemented (%d)\n",
					   slp->flags.l);
				    slen = 0;
				    break;
				  }
				slen -= slp->len + 2;
				prevflag = slp->flags.l;
				slp = (struct SL_component *) ((char *) slp + slp->len + 2);

				if (slen < 2)
				  {
				    /*
				     * If there is another SL record, and this component
				     * record isn't continued, then add a slash.
				     */
				    if ((!rootflag) && (rr_ptr.rr->u.sl.flags.l & 1) && !(prevflag & 1))
				      *rpnt++='/';
				    break;
				  }

				/*
				 * If this component record isn't continued, then append a '/'.
				 */
				if (!rootflag && !(prevflag & 1))
				  *rpnt++ = '/';
			      }
			    *rpnt++ = '\0';
			    grub_putstr(NAME_BUF+1024);// debug print!
			  }
			  rr_flag &= ~RR_FLAG_SL;
			  break;
#endif
			default:
			  break;
			}
		    }
		  if (!rr_flag)
		    /*
		     * There is no more extension we expects...
		     */
		    break;

		  rr_len -= rr_ptr.rr->len;
		  rr_ptr.ptr += rr_ptr.rr->len;
		  if (rr_len < 4 && ce_ptr != NULL)
		    {
		      /* preserve name before loading new extent. */
		      if( RRCONT_BUF <= (unsigned char *)name
			  && (unsigned char *)name < RRCONT_BUF + ISO_SECTOR_SIZE )
			{
			  memcpy(NAME_BUF, name, name_len);
			  name = (char *)NAME_BUF;
			}
		      rr_ptr.ptr = (char *)(RRCONT_BUF + ce_ptr->u.ce.offset.l);
		      rr_len = ce_ptr->u.ce.size.l;
		      emu_iso_sector_size_2048 = 1;
		      if (! devread(ce_ptr->u.ce.extent.l, 0, ISO_SECTOR_SIZE, (unsigned long long)(unsigned int)(char *)(RRCONT_BUF), 0xedde0d90))
			{
			  errnum = 0;	/* this is not fatal. */
			  break;
			}
		      ce_ptr = NULL;
		    }
		} /* rr_len >= 4 */
	}	//if (iso_type !== 1)
dddd:	
	      filemax = MAXINT;
			for (j = 0, k = 0;j < name_len;)
			{
				if (name[j] == '\\')
				{
					tmp_name1[k++] = name[j++];
					tmp_name1[k++] = name[j++];
					continue;
				}	
				if (name[j] == ' ')
				{
					/* quote the SPACE with a backslash */
					tmp_name1[k++] = '\\';
					tmp_name1[k++] = name[j++];
				}
				else
					tmp_name1[k++] = name[j++];
			}
			tmp_name1[k] = 0;
			name_len = k;
			if (substring(tmp_name, tmp_name1, 1) != 1)
		{
		  if (*dirname == '/' || !print_possibilities)
		    {
		      /*
		       *  DIRNAME is directory component of pathname,
		       *  or we are to open a file.
		       */
		      if (pathlen == name_len)
			{
						if (iso_type == ISO_TYPE_udf)
						{
							size = idr_udf_101->FileEntryLength;																		//File entry length in bytes
							extent = idr_udf_101->FileEntryLocation + udf_partition_start;					//File entrance logical sector						
							emu_iso_sector_size_2048 = 1;
//							devread (extent, 0, size, (unsigned long long)(unsigned int)(char *)UDF_DESC, 0xedde0d90);
							devread (extent, 0, size, (unsigned long long)(unsigned int)(char *)UDF_DIRREC, 0xedde0d90);
							idr_udf_105 = (struct udf_descriptor *)UDF_DIRREC;
						}	
			  if (*dirname == '/')
			    {
			      if (file_type != ISO_DIRECTORY)
				{
				  errnum = ERR_BAD_FILETYPE;
				  return 0;
				}
			      goto next_dir_level;
			    }
			  if (file_type != ISO_REGULAR)
			    {
			      errnum = ERR_BAD_FILETYPE;
			      return 0;
			    }
			    if (iso_type == ISO_TYPE_udf)
			 		{
//						unsigned long *tmp = (unsigned long *)(&UDF_DESC->FileEntry_BaseAddress + UDF_DESC->FileEntry_LengthofExtendedAttributes);
						unsigned long *tmp = (unsigned long *)(&idr_udf_105->FileEntry_BaseAddress + idr_udf_105->FileEntry_LengthofExtendedAttributes);
			  		INODE->file_start = *(tmp + 1) + udf_partition_start;
			  		filepos = 0;
			  		filemax = *tmp;	  			
			  		return 1;		    				
			    	}		
			    	else
			   		{		
			  INODE->file_start = idr->extent.l;
			  filepos = 0;
			  filemax = idr->size.l;
			  return 1;
			  		}
			}
		    }
		  else	/* Completion */
		    {
//		      int j, k;
//		      char ch1;
//		      char *tmp_name1 = (char *)(NAME_BUF);

		      if (print_possibilities > 0)
			print_possibilities = -print_possibilities;
		      //memcpy(NAME_BUF, name, name_len);
		      //NAME_BUF[name_len] = '\0';

		      /* copy name to tmp_name1, and quote spaces with '\\' */
//		      for (j = 0, k = 0; j < name_len; j++)
//		      {
//			if (! (ch1 = name[j]))
//				break;
//			if (ch1 == ' ')
//				tmp_name1[k++] = '\\';
//			tmp_name1[k++] = ch1;
//		      }
//		      tmp_name1[k] = 0;

		      print_a_completion (tmp_name1, 0);
		    }
		}
//	    } /* for */
ssss:			
		if (iso_type == ISO_TYPE_udf)
		{
			name = (char *)(&idr_udf_101->NameBaseAddress + idr_udf_101->LengthofImplementationUse + idr_udf_101->NameLength);
			//int j;
			for (j = 0; j < 4; j++)
			{
				if ((name[0] == 1) && (name[1] == 1))
					break;
				else
					name++;
			}
			if ((int)(name - (char*)UDF_DIRREC) > ISO_SECTOR_SIZE)
			{
				name_offset = (int)(name - (char*)UDF_DIRREC - ISO_SECTOR_SIZE);
				break;
			}
			else
				idr_udf_101 = (struct udf_FileIdentifier *)name;
		}
		else
			idr = (struct iso_directory_record *)((char *)idr + idr->length.l);
	} /* for */
				
	  if (size < ISO_SECTOR_SIZE)
		break;
	  size -= ISO_SECTOR_SIZE;
	} /* size>0 */

      if (*dirname == '/' || print_possibilities >= 0)
	{
	  errnum = ERR_FILE_NOT_FOUND;
	  return 0;
	}

    next_dir_level:
    ;
//      dirname += pathlen;

    } while (*dirname == '/');

  return 1;
}

unsigned long long
iso9660_read (unsigned long long buf, unsigned long long len, unsigned long write)
{
  unsigned long sector, blkoffset, size, ret;

  if (INODE->file_start == 0)
    return 0;

  ret = 0;
  blkoffset = filepos & (ISO_SECTOR_SIZE - 1);
  sector = filepos >> ISO_SECTOR_BITS;
  
  while (len > 0)
  {
      size = ISO_SECTOR_SIZE - blkoffset;
      
      if (size > len)
      	  size = len;

      emu_iso_sector_size_2048 = 1;

      disk_read_func = disk_read_hook;

//      blkoffset = devread (INODE->file_start + sector, blkoffset, size, buf, write);
			blkoffset = devread (INODE->file_start + sector, blkoffset, size, buf, write);

      disk_read_func = NULL;

      if (! blkoffset)
	  return 0;

      len -= size;	/* len always >= 0 */
      if (buf)
	buf += size;
      ret += size;
      filepos += size;
      sector++;
      blkoffset = 0;
  }

  return ret;
}

int
big_to_little (char *filename, unsigned int n)	//unicode16  Tai Mei turn a small tail
{
	unsigned int i;
	unsigned char a;
	for (i = 0; i < n;)	
	{
		a = filename[i];
		filename[i] = filename[i + 1];
		filename[i + 1] = a;
		i += 2;
	}
	return i;
}


#endif /* FSYS_ISO9660 */
