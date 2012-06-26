/*
 * The Progressive Graphics File; http://www.libpgf.org
 * 
 * $Date: 2006-05-18 16:03:32 +0200 (Do, 18 Mai 2006) $
 * $Revision: 194 $
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
/// @file WaveletTransform.cpp
/// @brief PGF wavelet transform class implementation
/// @author C. Stamm

#include "WaveletTransform.h"

#define c1 1	// best value 1
#define c2 2	// best value 2

//////////////////////////////////////////////////////////////////////////
// Constructor: Constructs a wavelet transform pyramid of given size and levels.
// @param width The width of the original image (at level 0) in pixels
// @param height The height of the original image (at level 0) in pixels
// @param levels The number of levels (>= 0)
// @param data Input data of subband LL at level 0
CWaveletTransform::CWaveletTransform(UINT32 width, UINT32 height, int levels, DataT* data) 
: m_nLevels(levels + 1)
, m_subband(0) 
{
	ASSERT(m_nLevels > 0 && m_nLevels <= MaxLevel + 1);
	InitSubbands(width, height, data);
#ifdef __PGFROISUPPORT__
	m_ROIindices.SetLevels(levels + 1);
#endif
}

/////////////////////////////////////////////////////////////////////
// Initialize size subbands on all levels
void CWaveletTransform::InitSubbands(UINT32 width, UINT32 height, DataT* data) {
	if (m_subband) Destroy();

	// create subbands
	m_subband = new CSubband[m_nLevels][NSubbands];

	// init subbands
	UINT32 loWidth = width;
	UINT32 hiWidth = width;
	UINT32 loHeight = height;
	UINT32 hiHeight = height;

	for (int level = 0; level < m_nLevels; level++) {
		m_subband[level][LL].Initialize(loWidth, loHeight, level, LL);	// LL
		m_subband[level][HL].Initialize(hiWidth, loHeight, level, HL);	//    HL
		m_subband[level][LH].Initialize(loWidth, hiHeight, level, LH);	// LH
		m_subband[level][HH].Initialize(hiWidth, hiHeight, level, HH);	//    HH
		hiWidth = loWidth >> 1;			hiHeight = loHeight >> 1;
		loWidth = (loWidth + 1) >> 1;	loHeight = (loHeight + 1) >> 1;
	}
	if (data) {
		m_subband[0][LL].SetBuffer(data);
	}
}

//////////////////////////////////////////////////////////////////////////
// Compute fast forward wavelet transform of LL subband at given level and
// stores result on all 4 subbands of level + 1.
// Wavelet transform used in writing a PGF file
// Forward Transform of srcBand and split and store it into subbands on destLevel
// high pass filter at even positions: 1/4(-2, 4, -2)
// low pass filter at odd positions: 1/8(-1, 2, 6, 2, -1)
// @param level A wavelet transform pyramid level (>= 0 && < Levels())
// @param quant A quantization value (linear scalar quantization)
// @return error in case of a memory allocation problem
OSError CWaveletTransform::ForwardTransform(int level, int quant) {
	ASSERT(level >= 0 && level < m_nLevels - 1);
	const int destLevel = level + 1;
	ASSERT(m_subband[destLevel]);
	CSubband* srcBand = &m_subband[level][LL]; ASSERT(srcBand);
	const UINT32 width = srcBand->GetWidth();
	const UINT32 height = srcBand->GetHeight();
	DataT* src = srcBand->GetBuffer(); ASSERT(src);
	DataT *row0, *row1, *row2, *row3;

	// Allocate memory for next transform level
	for (int i=0; i < NSubbands; i++) {
		if (!m_subband[destLevel][i].AllocMemory()) return InsufficientMemory;
	}

 	if (height >= FilterHeight) {
		// transform LL subband
		// top border handling
		row0 = src; row1 = row0 + width; row2 = row1 + width;
		ForwardRow(row0, width);
		ForwardRow(row1, width);
		ForwardRow(row2, width);
		for (UINT32 k=0; k < width; k++) {
			row1[k] -= ((row0[k] + row2[k] + c1) >> 1);
			row0[k] += ((row1[k] + c1) >> 1);
		}
		LinearToMallat(destLevel, row0, row1, width);
		row0 = row1; row1 = row2; row2 += width; row3 = row2 + width;

		// middle part
		for (UINT32 i=3; i < height-1; i += 2) {
			ForwardRow(row2, width);
			ForwardRow(row3, width);
			for (UINT32 k=0; k < width; k++) {
				row2[k] -= ((row1[k] + row3[k] + c1) >> 1);
				row1[k] += ((row0[k] + row2[k] + c2) >> 2);
			}
			LinearToMallat(destLevel, row1, row2, width);
			row0 = row2; row1 = row3; row2 = row3 + width; row3 = row2 + width;
		}

		// bottom border handling
		if (height & 1) {
			for (UINT32 k=0; k < width; k++) {
				row1[k] += ((row0[k] + c1) >> 1);
			}
			LinearToMallat(destLevel, row1, NULL, width);
			row0 = row1; row1 += width;
		} else {
			ForwardRow(row2, width);
			for (UINT32 k=0; k < width; k++) {
				row2[k] -= row1[k];
				row1[k] += ((row0[k] + row2[k] + c2) >> 2);
			}
			LinearToMallat(destLevel, row1, row2, width);
			row0 = row1; row1 = row2; row2 += width;
		}
	} else {
		// if height is too small
		row0 = src; row1 = row0 + width;
		// first part
		for (UINT32 k=0; k < height; k += 2) {
			ForwardRow(row0, width);
			ForwardRow(row1, width);
			LinearToMallat(destLevel, row0, row1, width);
			row0 += width << 1; row1 += width << 1;
		}
		// bottom
		if (height & 1) {
			LinearToMallat(destLevel, row0, NULL, width);
		}
	}

	if (quant > 0) {
		// subband quantization (without LL)
		for (int i=1; i < NSubbands; i++) {
			m_subband[destLevel][i].Quantize(quant);
		}
		// LL subband quantization
		if (destLevel == m_nLevels - 1) {
			m_subband[destLevel][LL].Quantize(quant);
		}
	}

	// free source band
	srcBand->FreeMemory();
	return NoError;
}

//////////////////////////////////////////////////////////////
// Forward transform one row
// high pass filter at even positions: 1/4(-2, 4, -2)
// low pass filter at odd positions: 1/8(-1, 2, 6, 2, -1)
void CWaveletTransform::ForwardRow(DataT* src, UINT32 width) {
	if (width >= FilterWidth) {
		UINT32 i = 3;

		// left border handling
		src[1] -= ((src[0] + src[2] + c1) >> 1);
		src[0] += ((src[1] + c1) >> 1);
		
		// middle part
		for (; i < width-1; i += 2) {
			src[i] -= ((src[i-1] + src[i+1] + c1) >> 1);
			src[i-1] += ((src[i-2] + src[i] + c2) >> 2);
		}

		// right border handling
		if (width & 1) {
			src[i-1] += ((src[i-2] + c1) >> 1);
		} else {
			src[i] -= src[i-1];
			src[i-1] += ((src[i-2] + src[i] + c2) >> 2);
		}
	}
}

/////////////////////////////////////////////////////////////////
// Copy transformed rows loRow and hiRow to subbands LL,HL,LH,HH
void CWaveletTransform::LinearToMallat(int destLevel, DataT* loRow, DataT* hiRow, UINT32 width) {
	const UINT32 wquot = width >> 1;
	const bool wrem = width & 1;
	CSubband &ll = m_subband[destLevel][LL], &hl = m_subband[destLevel][HL];
	CSubband &lh = m_subband[destLevel][LH], &hh = m_subband[destLevel][HH];

	if (hiRow) {
		for (UINT32 i=0; i < wquot; i++) {
			ll.WriteBuffer(*loRow++);	// first access, than increment
			hl.WriteBuffer(*loRow++);
			lh.WriteBuffer(*hiRow++);	// first access, than increment
			hh.WriteBuffer(*hiRow++);
		}
		if (wrem) {
			ll.WriteBuffer(*loRow);
			lh.WriteBuffer(*hiRow);
		}
	} else {
		for (UINT32 i=0; i < wquot; i++) {
			ll.WriteBuffer(*loRow++);	// first access, than increment
			hl.WriteBuffer(*loRow++);
		}
		if (wrem) ll.WriteBuffer(*loRow);
	}
}

//////////////////////////////////////////////////////////////////////////
// Compute fast inverse wavelet transform of all 4 subbands of given level and
// stores result in LL subband of level - 1.
// Inverse wavelet transform used in reading a PGF file
// Inverse Transform srcLevel and combine to destBand
// inverse high pass filter for even positions: 1/4(-1, 4, -1)
// inverse low pass filter for odd positions: 1/8(-1, 4, 6, 4, -1)
// @param srcLevel A wavelet transform pyramid level (> 0 && <= Levels())
// @param w [out] A pointer to the returned width of subband LL (in pixels)
// @param h [out] A pointer to the returned height of subband LL (in pixels)
// @param data [out] A pointer to the returned array of image data
// @return error in case of a memory allocation problem
OSError CWaveletTransform::InverseTransform(int srcLevel, UINT32* w, UINT32* h, DataT** data) {
	ASSERT(srcLevel > 0 && srcLevel < m_nLevels);
	const int destLevel = srcLevel - 1;
	ASSERT(m_subband[destLevel]);
	CSubband* destBand = &m_subband[destLevel][LL];
	UINT32 width, height;

	// allocate memory for the results of the inverse transform 
	if (!destBand->AllocMemory()) return InsufficientMemory;
	DataT *dest = destBand->GetBuffer(), *origin = dest, *row0, *row1, *row2, *row3;

#ifdef __PGFROISUPPORT__
	PGFRect destROI = destBand->GetROI();	// is valid only after AllocMemory
	width = destROI.Width();
	height = destROI.Height();
	const UINT32 destWidth = width; // destination buffer width
	const UINT32 destHeight = height; // destination buffer height

	// update destination ROI
	if (destROI.top & 1) {
		destROI.top++;
		origin += destWidth;
		height--;
	}
	if (destROI.left & 1) {
		destROI.left++;
		origin++;
		width--;
	}

	// init source buffer position
	for (int i=0; i < NSubbands; i++) {
		UINT32 left = (destROI.left >> 1) - m_subband[srcLevel][i].GetROI().left;
		UINT32 top = (destROI.top >> 1) - m_subband[srcLevel][i].GetROI().top;
		m_subband[srcLevel][i].InitBuffPos(left, top);
	}
#else
	width = destBand->GetWidth();
	height = destBand->GetHeight();
	PGFRect destROI(0, 0, width, height);
	const UINT32 destWidth = width; // destination buffer width
	const UINT32 destHeight = height; // destination buffer height

	// init source buffer position
	for (int i=0; i < NSubbands; i++) {
		m_subband[srcLevel][i].InitBuffPos();
	}
#endif

	if (destHeight >= FilterHeight) {
		// top border handling
		row0 = origin; row1 = row0 + destWidth;
		MallatToLinear(srcLevel, row0, row1, width);
		for (UINT32 k=0; k < width; k++) {
			row0[k] -= ((row1[k] + c1) >> 1);
		}

		// middle part
		row2 = row1 + destWidth; row3 = row2 + destWidth;
		for (UINT32 i=destROI.top + 2; i < destROI.bottom - 1; i += 2) {
			MallatToLinear(srcLevel, row2, row3, width);
			for (UINT32 k=0; k < width; k++) {
				row2[k] -= ((row1[k] + row3[k] + c2) >> 2);
				row1[k] += ((row0[k] + row2[k] + c1) >> 1);
			}
			InverseRow(row0, width);
			InverseRow(row1, width);
			row0 = row2; row1 = row3; row2 = row1 + destWidth; row3 = row2 + destWidth;
		}

		// bottom border handling
		if (height & 1) {
			MallatToLinear(srcLevel, row2, NULL, width);
			for (UINT32 k=0; k < width; k++) {
				row2[k] -= ((row1[k] + c1) >> 1);
				row1[k] += ((row0[k] + row2[k] + c1) >> 1);
			}
			InverseRow(row0, width);
			InverseRow(row1, width);
			InverseRow(row2, width);
			row0 = row1; row1 = row2; row2 += destWidth;
		} else {
			for (UINT32 k=0; k < width; k++) {
				row1[k] += row0[k];
			}
			InverseRow(row0, width);
			InverseRow(row1, width);
			row0 = row1; row1 += destWidth;
		}
	} else {
		// height is too small
		row0 = origin; row1 = row0 + destWidth;
		// first part
		for (UINT32 k=0; k < height; k += 2) {
			MallatToLinear(srcLevel, row0, row1, width);
			InverseRow(row0, width);
			InverseRow(row1, width);
			row0 += destWidth << 1; row1 += destWidth << 1;
		}
		// bottom
		if (height & 1) {
			MallatToLinear(srcLevel, row0, NULL, width);
			InverseRow(row0, width);
		} 
	}

	// free memory of the current srcLevel
	for (int i=0; i < NSubbands; i++) {
		m_subband[srcLevel][i].FreeMemory();
	}

	// return info
	*w = destWidth;
	*h = height;
	*data = dest;
	return NoError;
}

//////////////////////////////////////////////////////////////////////
// Inverse Wavelet Transform of one row
// inverse high pass filter for even positions: 1/4(-1, 4, -1)
// inverse low pass filter for odd positions: 1/8(-1, 4, 6, 4, -1)
void CWaveletTransform::InverseRow(DataT* dest, UINT32 width) {
	if (width >= FilterWidth) {
		UINT32 i = 2;

		// left border handling
		dest[0] -= ((dest[1] + c1) >> 1);

		// middle part
		for (; i < width - 1; i += 2) {
			dest[i] -= ((dest[i-1] + dest[i+1] + c2) >> 2);
			dest[i-1] += ((dest[i-2] + dest[i] + c1) >> 1);
		}

		// right border handling
		if (width & 1) {
			dest[i] -= ((dest[i-1] + c1) >> 1);
			dest[i-1] += ((dest[i-2] + dest[i] + c1) >> 1);
		} else {
			dest[i-1] += dest[i-2];
		}
	}
}

///////////////////////////////////////////////////////////////////
// Copy transformed coefficients from subbands LL,HL,LH,HH to interleaved format
void CWaveletTransform::MallatToLinear(int srcLevel, DataT* loRow, DataT* hiRow, UINT32 width) {
	const UINT32 wquot = width >> 1;
	const bool wrem = width & 1;
	CSubband &ll = m_subband[srcLevel][LL], &hl = m_subband[srcLevel][HL];
	CSubband &lh = m_subband[srcLevel][LH], &hh = m_subband[srcLevel][HH];

	if (hiRow) {
	#ifdef __PGFROISUPPORT__
		const bool storePos = wquot < ll.BufferWidth();
		UINT32 llPos = 0, hlPos = 0, lhPos = 0, hhPos = 0;

		if (storePos) {
			// save current src buffer positions
			llPos = ll.GetBuffPos(); 
			hlPos = hl.GetBuffPos(); 
			lhPos = lh.GetBuffPos(); 
			hhPos = hh.GetBuffPos(); 
		}
	#endif

		for (UINT32 i=0; i < wquot; i++) {
			*loRow++ = ll.ReadBuffer();// first access, than increment
			*loRow++ = hl.ReadBuffer();// first access, than increment
			*hiRow++ = lh.ReadBuffer();// first access, than increment
			*hiRow++ = hh.ReadBuffer();// first access, than increment
		}

		if (wrem) {
			*loRow++ = ll.ReadBuffer();// first access, than increment
			*hiRow++ = lh.ReadBuffer();// first access, than increment
		}

	#ifdef __PGFROISUPPORT__
		if (storePos) {
			// increment src buffer positions
			ll.IncBuffRow(llPos); 
			hl.IncBuffRow(hlPos); 
			lh.IncBuffRow(lhPos); 
			hh.IncBuffRow(hhPos); 
		}
	#endif

	} else {
	#ifdef __PGFROISUPPORT__
		const bool storePos = wquot < ll.BufferWidth();
		UINT32 llPos = 0, hlPos = 0;

		if (storePos) {
			// save current src buffer positions
			llPos = ll.GetBuffPos(); 
			hlPos = hl.GetBuffPos(); 
		}
	#endif

		for (UINT32 i=0; i < wquot; i++) {
			*loRow++ = ll.ReadBuffer();// first access, than increment
			*loRow++ = hl.ReadBuffer();// first access, than increment
		}
		if (wrem) *loRow++ = ll.ReadBuffer();

	#ifdef __PGFROISUPPORT__
		if (storePos) {
			// increment src buffer positions
			ll.IncBuffRow(llPos); 
			hl.IncBuffRow(hlPos); 
		}
	#endif
	}
}

#ifdef __PGFROISUPPORT__
//////////////////////////////////////////////////////////////////////
/// Compute and store ROIs for each level
/// @param rect rectangular region of interest (ROI)
void CWaveletTransform::SetROI(const PGFRect& rect) {
	// create tile indices
	m_ROIindices.CreateIndices();

	// compute tile indices
	m_ROIindices.ComputeIndices(m_subband[0][LL].GetWidth(), m_subband[0][LL].GetHeight(), rect);

	// compute ROIs
	UINT32 w, h;
	PGFRect r;

	for (int i=0; i < m_nLevels; i++) {
		const PGFRect& indices = m_ROIindices.GetIndices(i);

		for (int o=0; o < NSubbands; o++) {
			CSubband& subband = m_subband[i][o];

			subband.SetNTiles(m_ROIindices.GetNofTiles(i)); // must be called before TilePosition()
			subband.TilePosition(indices.left, indices.top, r.left, r.top, w, h);
			subband.TilePosition(indices.right - 1, indices.bottom - 1, r.right, r.bottom, w, h);
			r.right += w;
			r.bottom += h;
			subband.SetROI(r);
		}
	}
}

/////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////
void CRoiIndices::CreateIndices() {
	if (!m_indices) {
		// create tile indices 
		m_indices = new PGFRect[m_nLevels];
	}
}

//////////////////////////////////////////////////////////////////////
/// Computes a tile index either in x- or y-direction for a given image position.
/// @param width PGF image width
/// @param height PGF image height
/// @param pos A valid image position: (0 <= pos < width) or (0 <= pos < height)
/// @param horizontal If true, then pos must be a x-value, otherwise a y-value
/// @param isMin If true, then pos is left/top, else pos right/bottom
void CRoiIndices::ComputeTileIndex(UINT32 width, UINT32 height, UINT32 pos, bool horizontal, bool isMin) {
	ASSERT(m_indices);

	UINT32 m;
	UINT32 tileIndex = 0;
	UINT32 tileMin = 0, tileMax = (horizontal) ? width : height;
	ASSERT(pos <= tileMax);

	// compute tile index with binary search
	for (int i=m_nLevels - 1; i >= 0; i--) {
		// store values
		if (horizontal) {
			if (isMin) {
				m_indices[i].left = tileIndex;
			} else {
				m_indices[i].right = tileIndex + 1;
			}
		} else {
			if (isMin) {
				m_indices[i].top = tileIndex;
			} else {
				m_indices[i].bottom = tileIndex + 1;
			}
		}

		// compute values
		tileIndex <<= 1;
		m = (tileMin + tileMax)/2;
		if (pos >= m) {
			tileMin = m;
			tileIndex++;
		} else {
			tileMax = m;
		}
	}
}

/////////////////////////////////////////////////////////////////////
/// Compute tile indices for given rectangle (ROI)
/// @param width PGF image width
/// @param height PGF image height
/// @param rect ROI
void CRoiIndices::ComputeIndices(UINT32 width, UINT32 height, const PGFRect& rect) {
	ComputeTileIndex(width, height, rect.left, true, true);
	ComputeTileIndex(width, height, rect.top, false, true);
	ComputeTileIndex(width, height, rect.right, true, false);
	ComputeTileIndex(width, height, rect.bottom, false, false);
}

#endif // __PGFROISUPPORT__
