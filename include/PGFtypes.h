/*
 * The Progressive Graphics File; http://www.libpgf.org
 * 
 * $Date: 2007-06-11 10:56:17 +0200 (Mo, 11 Jun 2007) $
 * $Revision: 299 $
 * 
 * This file Copyright (C) 2006 xeraina GmbH, Switzerland
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

//////////////////////////////////////////////////////////////////////
/// @file PGFtypes.h
/// @brief PGF definitions
/// @author C. Stamm

#ifndef PGF_PGFTYPES_H
#define PGF_PGFTYPES_H

#include "PGFplatform.h"

//-------------------------------------------------------------------------------
//	Constraints
//-------------------------------------------------------------------------------
// BufferSize <= UINT16_MAX

//-------------------------------------------------------------------------------
//	Codec versions
//
// Version 2:	modified data structure PGFHeader (backward compatibility assured)
// Version 4:	DataT: INT32 instead of INT16, allows 31 bit per pixel and channel (backward compatibility assured)
// Version 5:	ROI, new block-reordering scheme (backward compatibility assured)
// Version 6:	modified data structure PGFPreHeader: hSize (header size) is now a UINT32 instead of a UINT16 (backward compatibility assured)
//
//-------------------------------------------------------------------------------
#define PGFCodecVersion		"6.12.24"			///< Major number
												///< Minor number: Year (2) Week (2)
#define PGFCodecVersionID   0x061224            ///< Codec version ID to use for API check in client implementation

//-------------------------------------------------------------------------------
//	Image constants
//-------------------------------------------------------------------------------
#define Magic				"PGF"				///< PGF identification
#define MaxLevel			30					///< maximum number of transform levels
#define NSubbands			4					///< number of subbands per level
#define MaxChannels			8					///< maximum number of (color) channels
#define DownsampleThreshold 3					///< if quality is larger than this threshold than downsampling is used
#define ColorTableLen		256					///< size of color lookup table (clut)
// version flags
#define Version2			2					///< data structure PGFHeader of major version 2
#define PGF32				4					///< 32 bit values are used -> allows at maximum 31 bits, otherwise 16 bit values are used -> allows at maximum 15 bits
#define PGFROI				8					///< supports Regions Of Interest
#define Version5			16					///< new coding scheme since major version 5
#define Version6			32					///< new HeaderSize: 32 bits instead of 16 bits 
// version numbers
#ifdef __PGF32SUPPORT__
#define PGFVersion			(Version2 | PGF32 | Version5 | Version6)	///< current standard version
#else
#define PGFVersion			(Version2 |         Version5 | Version6)	///< current standard version
#endif

//-------------------------------------------------------------------------------
//	Coder constants
//-------------------------------------------------------------------------------
#define BufferSize			16384				///< must be a multiple of WordWidth
#define RLblockSizeLen		15					///< block size length (< 16): ld(BufferSize) < RLblockSizeLen <= 2*ld(BufferSize)
#define LinBlockSize		8					///< side length of a coefficient block in a HH or LL subband
#define InterBlockSize		4					///< side length of a coefficient block in a HL or LH subband
#ifdef __PGF32SUPPORT__
	#define MaxBitPlanes	31					///< maximum number of bit planes of m_value: 32 minus sign bit
#else
	#define MaxBitPlanes	15					///< maximum number of bit planes of m_value: 16 minus sign bit
#endif
#define MaxBitPlanesLog		5					///< number of bits to code the maximum number of bit planes (in 32 or 16 bit mode)
#define MaxQuality			MaxBitPlanes		///< maximum quality

//-------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------
enum Orientation { LL=0, HL=1, LH=2, HH=3 };

/// general PGF file structure
/// PGFPreHeaderV6 PGFHeader PGFPostHeader LevelLengths Level_n-1 Level_n-2 ... Level_0
/// PGFPostHeader ::= [ColorTable] [UserData]
/// LevelLengths  ::= UINT32[nLevels]

#pragma pack(1)
/////////////////////////////////////////////////////////////////////
/// PGF magic and version (part of PGF pre-header)
/// @author C. Stamm
/// @brief PGF identification and version
struct PGFMagicVersion {
	char magic[3];				///< PGF identification = "PGF"
	UINT8 version;				///< PGF version
	// total: 4 Bytes
};

/////////////////////////////////////////////////////////////////////
/// PGF pre-header defined header length and PGF identification and version
/// @author C. Stamm
/// @brief PGF pre-header
struct PGFPreHeader : PGFMagicVersion {
	UINT32 hSize;				///< total size of PGFHeader, [ColorTable], and [UserData] in bytes
	// total: 8 Bytes
};

/////////////////////////////////////////////////////////////////////
/// PGF header contains image information
/// @author C. Stamm
/// @brief PGF header
struct PGFHeader {
	PGFHeader() : width(0), height(0), nLevels(0), quality(0), bpp(0), channels(0), mode(ImageModeUnknown), usedBitsPerChannel(0), reserved1(0), reserved2(0) {}
	UINT32 width;				///< image width in pixels
	UINT32 height;				///< image height in pixels
	UINT8 nLevels;				///< number of DWT levels
	UINT8 quality;				///< quantization parameter: 0=lossless, 4=standard, 6=poor quality
	UINT8 bpp;					///< bits per pixel
	UINT8 channels;				///< number of channels
	UINT8 mode;					///< image mode according to Adobe's image modes
	UINT8 usedBitsPerChannel;	///< number of used bits per channel in 16- and 32-bit per channel modes
	UINT8 reserved1, reserved2;	///< not used
	// total: 16 Bytes
};

/////////////////////////////////////////////////////////////////////
/// PGF post-header is optional. It contains color table and user data
/// @author C. Stamm
/// @brief Optional PGF post-header
struct PGFPostHeader {
	RGBQUAD clut[ColorTableLen];///< color table for indexed color images
	UINT8 *userData;			///< user data of size userDataLen
	UINT32 userDataLen;			///< user data size in bytes
};

/////////////////////////////////////////////////////////////////////
/// ROI block header is used with ROI coding scheme. It contains block size and tile end flag
/// @author C. Stamm
/// @brief Block header used with ROI coding scheme 
union ROIBlockHeader {
	/// Constructor
	/// @param v Buffer size
	ROIBlockHeader(UINT16 v) { val = v; }
	/// Constructor
	/// @param size Buffer size
	/// @param end 0/1 Flag; 1: last part of a tile
	ROIBlockHeader(UINT32 size, bool end)	{ ASSERT(size < (1 << RLblockSizeLen)); rbh.bufferSize = size; rbh.tileEnd = end; }
	
	UINT16 val; ///< unstructured union value
	/// @brief Named ROI block header (part of the union)
	struct RBH {
#ifdef PGF_USE_BIG_ENDIAN
		UINT16 tileEnd   :				1;	///< 1: last part of a tile
		UINT16 bufferSize: RLblockSizeLen;	///< number of uncoded UINT32 values in a block
#else
		UINT16 bufferSize: RLblockSizeLen;	///< number of uncoded UINT32 values in a block
		UINT16 tileEnd   :				1;	///< 1: last part of a tile
#endif // PGF_USE_BIG_ENDIAN
	} rbh;	///< ROI block header
	// total: 2 Bytes
};

#pragma pack()

/////////////////////////////////////////////////////////////////////
/// PGF I/O exception 
/// @author C. Stamm
/// @brief PGF exception
struct IOException {
	/// Standard constructor
	IOException() : error(NoError) {}
	/// Constructor
	/// @param err Run-time error
	IOException(OSError err) : error(err) {}

	OSError error;				///< operating system error code
};

/////////////////////////////////////////////////////////////////////
/// Rectangle
/// @author C. Stamm
/// @brief Rectangle
struct PGFRect {
	/// Standard constructor
	PGFRect() : left(0), top(0), right(0), bottom(0) {}
	/// Constructor
	/// @param x Left offset
	/// @param y Top offset
	/// @param width Rectangle width
	/// @param height Rectangle height
	PGFRect(UINT32 x, UINT32 y, UINT32 width, UINT32 height) : left(x), top(y), right(x + width), bottom(y + height) {}

	/// @return Rectangle width
	UINT32 Width() const					{ return right - left; }
	/// @return Rectangle height
	UINT32 Height() const					{ return bottom - top; }
	
	/// Test if point (x,y) is inside this rectangle
	/// @param x Point coordinate x
	/// @param y Point coordinate y
	/// @return True if point (x,y) is inside this rectangle
	bool IsInside(UINT32 x, UINT32 y) const { return (x >= left && x < right && y >= top && y < bottom); }

	UINT32 left, top, right, bottom;
};

#ifdef __PGF32SUPPORT__
typedef INT32 DataT;
#else
typedef INT16 DataT;
#endif

typedef void (*RefreshCB)(void *p);

//-------------------------------------------------------------------------------
// Image constants
//-------------------------------------------------------------------------------
#define MagicVersionSize	sizeof(PGFMagicVersion)
#define PreHeaderSize		sizeof(PGFPreHeader)
#define HeaderSize			sizeof(PGFHeader)
#define ColorTableSize		ColorTableLen*sizeof(RGBQUAD)
#define DataTSize			sizeof(DataT)

#endif //PGF_PGFTYPES_H
