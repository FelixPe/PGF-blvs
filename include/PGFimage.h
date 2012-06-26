/*
 * The Progressive Graphics File; http://www.libpgf.org
 * 
 * $Date: 2007-02-03 13:04:21 +0100 (Sa, 03 Feb 2007) $
 * $Revision: 280 $
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
/// @file PGFimage.h
/// @brief PGF image class
/// @author C. Stamm

#ifndef PGF_PGFIMAGE_H
#define PGF_PGFIMAGE_H

#include "PGFstream.h"

//////////////////////////////////////////////////////////////////////
// types
enum ProgressMode { PM_Relative, PM_Absolute };

//////////////////////////////////////////////////////////////////////
// prototypes
class CDecoder;
class CEncoder;
class CWaveletTransform;

//////////////////////////////////////////////////////////////////////
/// PGF image class is the main class. You always need a PGF object
/// for encoding or decoding image data.
/// Decoding:
///		pgf.Open(...)
///		pgf.Read(...)
///		pgf.GetBitmap(...)
/// Encoding:
///		pgf.SetHeader(...)
///		pgf.ImportBitmap(...)
///		pgf.Write(...)
/// @author C. Stamm, R. Spuler
/// @brief PGF main class
class CPGFImage {
public:
	
	//////////////////////////////////////////////////////////////////////
	/// Standard constructor: It is used to create a PGF instance for opening and reading.
	CPGFImage();

	//////////////////////////////////////////////////////////////////////
	/// Destructor: Destroy internal data structures.
	virtual ~CPGFImage();

	//////////////////////////////////////////////////////////////////////
	/// Close PGF image after opening and reading.
	/// Destructor calls this method during destruction.
	virtual void Close();

	//////////////////////////////////////////////////////////////////////
	/// Destroy internal data structures.
	/// Destructor calls this method during destruction.
	virtual void Destroy();

	//////////////////////////////////////////////////////////////////////
	/// Open a PGF image at current stream position: read pre-header, header, and ckeck image type.
	/// Precondition: The stream has been opened for reading.
	/// It might throw an IOException.
	/// @param stream A PGF stream
	void Open(CPGFStream* stream) THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Returns true if the PGF has been opened and not closed.
	bool IsOpen() const	{ return m_decoder != NULL; }

	//////////////////////////////////////////////////////////////////////
	/// Read and decode some levels of a PGF image at current stream position.
	/// A PGF image is structered in levels, numbered between 0 and Levels() - 1.
	/// Each level can be seen as a single image, containing the same content
	/// as all other levels, but in a different size (width, height).
	/// The image size at level i is double the size (width, height) of the image at level i+1.
	/// The image at level 0 contains the original size.
	/// Precondition: The PGF image has been opened with a call of Open(...).
	/// It might throw an IOException.
	/// @param level [0, nLevels) The image level of the resulting image in the internal image buffer.
	/// @param cb A pointer to a callback procedure. The procedure is called after reading a single level. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	void Read(int level = 0, CallbackPtr cb = NULL, void *data = NULL) THROW_;

#ifdef __PGFROISUPPORT__
	//////////////////////////////////////////////////////////////////////
	/// Read a rectangular region of interest of a PGF image at current stream position.
	/// The origin of the coordinate axis is the top-left corner of the image.
	/// All coordinates are measured in pixels.
	/// It might throw an IOException.
	/// @param rect [inout] Rectangular region of interest (ROI). The rect might be cropped.
	/// @param level [0, nLevels) The image level of the resulting image in the internal image buffer.
	/// @param cb A pointer to a callback procedure. The procedure is called after reading a single level. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	void Read(PGFRect& rect, int level = 0, CallbackPtr cb = NULL, void *data = NULL) THROW_;
#endif

	//////////////////////////////////////////////////////////////////////
	/// Read and decode smallest level of a PGF image at current stream position.
	/// For details, please refert to Read(...)
	/// Precondition: The PGF image has been opened with a call of Open(...).
	/// It might throw an IOException.
	void ReadPreview() THROW_										{ Read(Levels() - 1); }

	//////////////////////////////////////////////////////////////////////
	/// After you've written a PGF image, you can call this method followed by GetBitmap/GetYUV
	/// to get a quick reconstruction (coded -> decoded image).
	/// It might throw an IOException.
	/// @param level The image level of the resulting image in the internal image buffer.
	void Reconstruct(int level = 0) THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Get image data in interleaved format: (ordering of RGB data is BGR[A])
	/// Upsampling, YUV to RGB transform and interleaving are done here to reduce the number 
	/// of passes over the data.
	/// The absolute value of pitch is the number of bytes of an image row of the given image buffer.
	/// If pitch is negative, then the image buffer must point to the last row of a bottom-up image (first byte on last row).
	/// if pitch is positive, then the image buffer must point to the first row of a top-down image (first byte).
	/// The sequence of output channels in the output image buffer does not need to be the same as provided by PGF. In case of different sequences you have to
	/// provide a channelMap of size of expected channels (depending on image mode). For example, PGF provides a channel sequence BGR in RGB color mode.
	/// If your provided image buffer expects a channel sequence ARGB, then the channelMap looks like { 3, 2, 1, 0 }.
	/// It might throw an IOException.
	/// @param pitch The number of bytes of a row of the image buffer.
	/// @param buff An image buffer.
	/// @param bpp The number of bits per pixel used in image buffer.
	/// @param channelMap A integer array containing the mapping of PGF channel ordering to expected channel ordering.
	/// @param cb A pointer to a callback procedure. The procedure is called after each copied buffer row. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	void GetBitmap(int pitch, UINT8* buff, BYTE bpp, int channelMap[] = NULL, CallbackPtr cb = NULL, void *data = NULL) const THROW_; // throws IOException

	//////////////////////////////////////////////////////////////////////
	/// Get YUV image data in interleaved format: (ordering is YUV[A])
	/// The absolute value of pitch is the number of bytes of an image row of the given image buffer.
	/// If pitch is negative, then the image buffer must point to the last row of a bottom-up image (first byte on last row).
	/// if pitch is positive, then the image buffer must point to the first row of a top-down image (first byte).
	/// The sequence of output channels in the output image buffer does not need to be the same as provided by PGF. In case of different sequences you have to
	/// provide a channelMap of size of expected channels (depending on image mode). For example, PGF provides a channel sequence BGR in RGB color mode.
	/// If your provided image buffer expects a channel sequence VUY, then the channelMap looks like { 2, 1, 0 }.
	/// It might throw an IOException.
	/// @param pitch The number of bytes of a row of the image buffer.
	/// @param buff An image buffer.
	/// @param bpp The number of bits per pixel used in image buffer.
	/// @param channelMap A integer array containing the mapping of PGF channel ordering to expected channel ordering.
	/// @param cb A pointer to a callback procedure. The procedure is called after each copied buffer row. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	void GetYUV(int pitch, DataT* buff, BYTE bpp, int channelMap[] = NULL, CallbackPtr cb = NULL, void *data = NULL) const THROW_; // throws IOException

	//////////////////////////////////////////////////////////////////////
	/// Import an image from a specified image buffer.
	/// This method is usually called before Write(...) and after SetHeader(...).
	/// The absolute value of pitch is the number of bytes of an image row.
	/// If pitch is negative, then buff points to the last row of a bottom-up image (first byte on last row).
	/// If pitch is positive, then buff points to the first row of a top-down image (first byte).
	/// The sequence of input channels in the input image buffer does not need to be the same as expected from PGF. In case of different sequences you have to
	/// provide a channelMap of size of expected channels (depending on image mode). For example, PGF expects in RGB color mode a channel sequence BGR.
	/// If your provided image buffer contains a channel sequence ARGB, then the channelMap looks like { 3, 2, 1, 0 }.
	/// It might throw an IOException.
	/// @param pitch The number of bytes of a row of the image buffer.
	/// @param buff An image buffer.
	/// @param bpp The number of bits per pixel used in image buffer.
	/// @param channelMap A integer array containing the mapping of input channel ordering to expected channel ordering.
	/// @param cb A pointer to a callback procedure. The procedure is called after each imported buffer row. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	void ImportBitmap(int pitch, UINT8 *buff, BYTE bpp, int channelMap[] = NULL, CallbackPtr cb = NULL, void *data = NULL) THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Import a YUV image from a specified image buffer.
	/// The absolute value of pitch is the number of bytes of an image row.
	/// If pitch is negative, then buff points to the last row of a bottom-up image (first byte on last row).
	/// If pitch is positive, then buff points to the first row of a top-down image (first byte).
	/// The sequence of input channels in the input image buffer does not need to be the same as expected from PGF. In case of different sequences you have to
	/// provide a channelMap of size of expected channels (depending on image mode). For example, PGF expects in RGB color mode a channel sequence BGR.
	/// If your provided image buffer contains a channel sequence VUY, then the channelMap looks like { 2, 1, 0 }.
	/// It might throw an IOException.
	/// @param pitch The number of bytes of a row of the image buffer.
	/// @param buff An image buffer.
	/// @param bpp The number of bits per pixel used in image buffer.
	/// @param channelMap A integer array containing the mapping of input channel ordering to expected channel ordering.
	/// @param cb A pointer to a callback procedure. The procedure is called after each imported buffer row. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	void ImportYUV(int pitch, DataT *buff, BYTE bpp, int channelMap[] = NULL, CallbackPtr cb = NULL, void *data = NULL) THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Encode and write a entire PGF image (header and image) at current stream position.
	/// A PGF image is structered in levels, numbered between 0 and Levels() - 1.
	/// Each level can be seen as a single image, containing the same content
	/// as all other levels, but in a different size (width, height).
	/// The image size at level i is double the size (width, height) of the image at level i+1.
	/// The image at level 0 contains the original size.
	/// Precondition: the PGF image contains a valid header (see also SetHeader(...)). 
	/// It might throw an IOException.
	/// @param stream A PGF stream
	/// @param nWrittenBytes [in-out] The number of bytes written into stream are added to the input value.
	/// @param cb A pointer to a callback procedure. The procedure is called after writing a single level. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	void Write(CPGFStream* stream, UINT32* nWrittenBytes = NULL, CallbackPtr cb = NULL, void *data = NULL) THROW_;

	//////////////////////////////////////////////////////////////////
	/// Create wavelet transform channels and encoder. Write header at current stream position.
	/// Call this method before your first call of Write(int level) or WriteImage(), but after SetHeader().
	/// This method is called inside of Write(stream, ...).
	/// It might throw an IOException.
	/// @param stream A PGF stream
	/// @return The number of bytes written into stream.
	UINT32 WriteHeader(CPGFStream* stream) THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Encode and write the one and only image at current stream position.
	/// Call this method after WriteHeader(). In case you want to write uncached metadata, 
	/// then do that after WriteHeader() and before WriteImage(). 
	/// This method is called inside of Write(stream, ...).
	/// It might throw an IOException.
	/// @param stream A PGF stream
	/// @param cb A pointer to a callback procedure. The procedure is called after writing a single level. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	/// @return The number of bytes written into stream.
	UINT32 WriteImage(CPGFStream* stream, CallbackPtr cb = NULL, void *data = NULL) THROW_;

#ifdef __PGFROISUPPORT__
	//////////////////////////////////////////////////////////////////
	/// Encode and write down to given level at current stream position.
	/// A PGF image is structered in levels, numbered between 0 and Levels() - 1.
	/// Each level can be seen as a single image, containing the same content
	/// as all other levels, but in a different size (width, height).
	/// The image size at level i is double the size (width, height) of the image at level i+1.
	/// The image at level 0 contains the original size.
	/// Preconditions: the PGF image contains a valid header (see also SetHeader(...)) and 
	/// WriteHeader() has been called before. Levels() > 0.
	/// The ROI encoding scheme must be used (see also SetHeader(...)).
	/// It might throw an IOException.
	/// @param level [0, nLevels) The image level of the resulting image in the internal image buffer.
	/// @param cb A pointer to a callback procedure. The procedure is called after writing a single level. If cb returns true, then it stops proceeding.
	/// @param data Data Pointer to C++ class container to host callback procedure.
	/// @return The number of bytes written into stream.
	UINT32 Write(int level, CallbackPtr cb = NULL, void *data = NULL) THROW_;
#endif

	/////////////////////////////////////////////////////////////////////
	/// Configures the encoder.
	/// @param useOMP Use parallel threading with Open MP during encoding. Default value: true. Influences the encoding only if the codec has been compiled with OpenMP support.
	/// @param favorSpeedOverSize Favors encoding speed over compression ratio. Default value: false
	void ConfigureEncoder(bool useOMP = true, bool favorSpeedOverSize = false) { m_useOMPinEncoder = useOMP; m_favorSpeedOverSize = favorSpeedOverSize; }

	/////////////////////////////////////////////////////////////////////
	/// Configures the decoder.
	/// @param useOMP Use parallel threading with Open MP during decoding. Default value: true. Influences the decoding only if the codec has been compiled with OpenMP support.
	/// @param skipUserData The file might contain user data (metadata). User data ist usually read during Open and stored in memory. Set this flag to false when storing in memory is not needed.
	void ConfigureDecoder(bool useOMP = true, bool skipUserData = false) { m_useOMPinDecoder = useOMP; m_skipUserData = skipUserData; }

	////////////////////////////////////////////////////////////////////
	/// Reset stream position to start of PGF pre-header
	void ResetStreamPos() THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Set internal PGF image buffer channel.
	/// @param channel A YUV data channel
	/// @param c A channel index
	void SetChannel(DataT* channel, int c = 0)						{ ASSERT(c >= 0 && c < MaxChannels); m_channel[c] = channel; }

	//////////////////////////////////////////////////////////////////////
	/// Set PGF header and user data.
	/// Precondition: The PGF image has been closed with Close(...) or never opened with Open(...).
	/// It might throw an IOException.
	/// @param header A valid and already filled in PGF header structure
	/// @param flags A combination of additional version flags. In case you use level-wise encoding then set flag = PGFROI.
	/// @param userData A user-defined memory block containing any kind of cached metadata.
	/// @param userDataLength The size of user-defined memory block in bytes
	void SetHeader(const PGFHeader& header, BYTE flags = 0, UINT8* userData = 0, UINT32 userDataLength = 0) THROW_; // throws IOException

	//////////////////////////////////////////////////////////////////////
	/// Set maximum intensity value for image modes with more than eight bits per channel.
	/// Call this method after SetHeader, but before ImportBitmap.
	/// @param maxValue The maximum intensity value.
	void SetMaxValue(UINT32 maxValue);

	//////////////////////////////////////////////////////////////////////
	/// Set progress mode used in Read and Write.
	/// Default mode is PM_Relative.
	/// This method must be called before Open() or SetHeader().
	/// PM_Relative: 100% = level difference between current level and target level of Read/Write
	/// PM_Absolute: 100% = number of levels
	void SetProgressMode(ProgressMode pm)							{ m_progressMode = pm; }

	//////////////////////////////////////////////////////////////////////
	/// Set refresh callback procedure and its parameter.
	/// The refresh callback is called during Read(...) after each level read.
	/// @param callback A refresh callback procedure
	/// @param arg A parameter of the refresh callback procedure
	void SetRefreshCallback(RefreshCB callback, void* arg)			{ m_cb = callback; m_cbArg = arg; }

	//////////////////////////////////////////////////////////////////////
	/// Sets the red, green, blue (RGB) color values for a range of entries in the palette (clut).
	/// It might throw an IOException.
	/// @param iFirstColor The color table index of the first entry to set.
	/// @param nColors The number of color table entries to set.
	/// @param prgbColors A pointer to the array of RGBQUAD structures to set the color table entries.
	void SetColorTable(UINT32 iFirstColor, UINT32 nColors, const RGBQUAD* prgbColors) THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Return an internal YUV image channel.
	/// @param c A channel index
	/// @return An internal YUV image channel
	DataT* GetChannel(int c = 0)									{ ASSERT(c >= 0 && c < MaxChannels); return m_channel[c]; }

	//////////////////////////////////////////////////////////////////////
	/// Retrieves red, green, blue (RGB) color values from a range of entries in the palette of the DIB section.
	/// It might throw an IOException.
	/// @param iFirstColor The color table index of the first entry to retrieve.
	/// @param nColors The number of color table entries to retrieve.
	/// @param prgbColors A pointer to the array of RGBQUAD structures to retrieve the color table entries.
	void GetColorTable(UINT32 iFirstColor, UINT32 nColors, RGBQUAD* prgbColors) const THROW_;

	//////////////////////////////////////////////////////////////////////
	// Returns address of internal color table
	/// @return Address of color table
	const RGBQUAD* GetColorTable() const							{ return m_postHeader.clut; }

	//////////////////////////////////////////////////////////////////////
	/// Return the PGF header structure.
	/// @return A PGF header structure
	const PGFHeader* GetHeader() const								{ return &m_header; }

	//////////////////////////////////////////////////////////////////////
	/// Get maximum intensity value for image modes with more than eight bits per channel.
	/// Don't call this method before the PGF header has been read.
	/// @return The maximum intensity value.
	UINT32 GetMaxValue() const										{ return (1 << m_header.usedBitsPerChannel) - 1; }

	//////////////////////////////////////////////////////////////////////
	/// Return the stream position of the user data or 0.
	/// Precondition: The PGF image has been opened with a call of Open(...).
	UINT64 GetUserDataPos() const									{ return m_userDataPos; }									

	//////////////////////////////////////////////////////////////////////
	/// Return user data and size of user data.
	/// Precondition: The PGF image has been opened with a call of Open(...).
	/// @param size [out] Size of user data in bytes.
	/// @return A pointer to user data or NULL if there is no user data.
	const UINT8* GetUserData(UINT32& size) const;

	//////////////////////////////////////////////////////////////////////
	/// Return the length of all encoded headers in bytes.
	/// Precondition: The PGF image has been opened with a call of Open(...).
	/// @return The length of all encoded headers in bytes
	UINT32 GetEncodedHeaderLength() const;

	//////////////////////////////////////////////////////////////////////
	/// Return the length of an encoded PGF level in bytes.
	/// Precondition: The PGF image has been opened with a call of Open(...).
	/// @param level The image level
	/// @return The length of a PGF level in bytes
	UINT32 GetEncodedLevelLength(int level) const					{ ASSERT(level >= 0 && level < m_header.nLevels); return m_levelLength[m_header.nLevels - level - 1]; }

	//////////////////////////////////////////////////////////////////////
	/// Reads the encoded PGF headers and copies it to a target buffer.
	/// Precondition: The PGF image has been opened with a call of Open(...).
	/// It might throw an IOException.
	/// @param target The target buffer
	/// @param targetLen The length of the target buffer in bytes
	/// @return The number of bytes copied to the target buffer
	UINT32 ReadEncodedHeader(UINT8* target, UINT32 targetLen) const THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Reads the data of an encoded PGF level and copies it to a target buffer 
	/// without decoding.
	/// Precondition: The PGF image has been opened with a call of Open(...).
	/// It might throw an IOException.
	/// @param level The image level
	/// @param target The target buffer
	/// @param targetLen The length of the target buffer in bytes
	/// @return The number of bytes copied to the target buffer
	UINT32 ReadEncodedData(int level, UINT8* target, UINT32 targetLen) const THROW_;

	//////////////////////////////////////////////////////////////////////
	/// Return current image width of given channel in pixels.
	/// The returned width depends on the levels read so far and on ROI.
	/// @param c A channel index
	/// @return Channel width in pixels
	UINT32 ChannelWidth(int c = 0) const							{ ASSERT(c >= 0 && c < MaxChannels); return m_width[c]; }

	//////////////////////////////////////////////////////////////////////
	/// Return current image height of given channel in pixels.
	/// The returned height depends on the levels read so far and on ROI.
	/// @param c A channel index
	/// @return Channel height in pixels
	UINT32 ChannelHeight(int c = 0) const							{ ASSERT(c >= 0 && c < MaxChannels); return m_height[c]; }

	//////////////////////////////////////////////////////////////////////
	/// Return bits per channel of the image's encoder.
	/// @return Bits per channel
	BYTE ChannelDepth() const										{ return CurrentChannelDepth(m_preHeader.version); }

	//////////////////////////////////////////////////////////////////////
	/// Return image width of channel 0 at given level in pixels.
	/// The returned width is independent of any Read-operations and ROI.
	/// @param level A level
	/// @return Image level width in pixels
	UINT32 Width(int level = 0) const								{ ASSERT(level >= 0); return LevelWidth(m_header.width, level); }

	//////////////////////////////////////////////////////////////////////
	/// Return image height of channel 0 at given level in pixels.
	/// The returned height is independent of any Read-operations and ROI.
	/// @param level A level
	/// @return Image level height in pixels
	UINT32 Height(int level = 0) const								{ ASSERT(level >= 0); return LevelHeight(m_header.height, level); }

	//////////////////////////////////////////////////////////////////////
	/// Return current image level. 
	/// Since Read(...) can be used to read each image level separately, it is
	/// helpful to know the current level. The current level immediately after Open(...) is Levels().
	/// @return Current image level
	BYTE Level() const												{ return (BYTE)m_currentLevel; }

	//////////////////////////////////////////////////////////////////////
	/// Return the number of image levels. 
	/// @return Number of image levels
	BYTE Levels() const												{ return m_header.nLevels; }

	//////////////////////////////////////////////////////////////////////
	/// Return the PGF quality. The quality is inbetween 0 and MaxQuality.
	/// PGF quality 0 means lossless quality.
	/// @return PGF quality
	BYTE Quality() const											{ return m_header.quality; }

	//////////////////////////////////////////////////////////////////////
	/// Return the number of image channels.
	/// An image of type RGB contains 3 image channels (B, G, R).
	/// @return Number of image channels
	BYTE Channels() const											{ return m_header.channels; }
	
	//////////////////////////////////////////////////////////////////////
	/// Return the image mode.
	/// An image mode is a predefined constant value (see also PGFtypes.h) compatible with Adobe Photoshop.
	/// It represents an image type and format.
	/// @return Image mode
	BYTE Mode() const												{ return m_header.mode; }

	//////////////////////////////////////////////////////////////////////
	/// Return the number of bits per pixel.
	/// Valid values can be 1, 8, 12, 16, 24, 32, 48, 64.
	/// @return Number of bits per pixel.
	BYTE BPP() const												{ return m_header.bpp; }

	//////////////////////////////////////////////////////////////////////
	/// Return true if the pgf image supports Region Of Interest (ROI).
	/// @return true if the pgf image supports ROI.
	bool ROIisSupported() const										{ return (m_preHeader.version & PGFROI) == PGFROI; }

	//////////////////////////////////////////////////////////////////////
	/// Returns number of used bits per input/output image channel.
	/// Precondition: header must be initialized.
	/// @return number of used bits per input/output image channel.
	BYTE UsedBitsPerChannel() const;

	//////////////////////////////////////////////////////////////////////
	/// Returns images' PGF version
	/// @return PGF codec version of the image
	BYTE Version() const											{ return CurrentVersion(m_preHeader.version); }

	//class methods

	//////////////////////////////////////////////////////////////////////
	/// Check for valid import image mode.
	/// @param mode Image mode
	/// @return True if an image of given mode can be imported with ImportBitmap(...)
	static bool ImportIsSupported(BYTE mode);

	//////////////////////////////////////////////////////////////////////
	/// Compute and return image width at given level.
	/// @param width Original image width (at level 0)
	/// @param level An image level
	/// @return Image level width in pixels
	static UINT32 LevelWidth(UINT32 width, int level)				{ ASSERT(level >= 0); UINT32 w = (width >> level); return ((w << level) == width) ? w : w + 1; }

	//////////////////////////////////////////////////////////////////////
	/// Compute and return image height at given level.
	/// @param height Original image height (at level 0)
	/// @param level An image level
	/// @return Image level height in pixels
	static UINT32 LevelHeight(UINT32 height, int level)				{ ASSERT(level >= 0); UINT32 h = (height >> level); return ((h << level) == height) ? h : h + 1; }

	//////////////////////////////////////////////////////////////////////
	/// Compute and return codec version.
	/// @return current PGF codec version
	static BYTE CurrentVersion(BYTE version = PGFVersion);

	//////////////////////////////////////////////////////////////////////
	/// Compute and return codec version.
	/// @return current PGF codec version
	static BYTE CurrentChannelDepth(BYTE version = PGFVersion)		{ return (version & PGF32) ? 32 : 16; }

protected:
	CWaveletTransform* m_wtChannel[MaxChannels];	///< wavelet transformed color channels
	DataT* m_channel[MaxChannels];					///< untransformed channels in YUV format
	CDecoder* m_decoder;			///< PGF decoder
	CEncoder* m_encoder;			///< PGF encoder
	UINT32* m_levelLength;			///< length of each level in bytes; first level starts immediately after this array
	UINT32 m_width[MaxChannels];	///< width of each channel at current level
	UINT32 m_height[MaxChannels];	///< height of each channel at current level
	PGFPreHeader m_preHeader;		///< PGF pre-header
	PGFHeader m_header;				///< PGF file header
	PGFPostHeader m_postHeader;		///< PGF post-header
	UINT64 m_userDataPos;			///< stream position of user data
	int m_currentLevel;				///< transform level of current image
	BYTE m_quant;					///< quantization parameter
	bool m_downsample;				///< chrominance channels are downsampled
	bool m_favorSpeedOverSize;		///< favor encoding speed over compression ratio
	bool m_useOMPinEncoder;			///< use Open MP in encoder
	bool m_useOMPinDecoder;			///< use Open MP in decoder
	bool m_skipUserData;			///< skip user data (metadata) during open
#ifdef __PGFROISUPPORT__
	bool m_streamReinitialized;		///< stream has been reinitialized
	PGFRect m_roi;					///< region of interest
#endif

private:	
	RefreshCB m_cb;					///< pointer to refresh callback procedure
	void *m_cbArg;					///< refresh callback argument
	double m_percent;				///< progress [0..1]
	ProgressMode m_progressMode;	///< progress mode used in Read and Write; PM_Relative is default mode

	void ComputeLevels();
	void CompleteHeader();
	void RgbToYuv(int pitch, UINT8* rgbBuff, BYTE bpp, int channelMap[], CallbackPtr cb, void *data) THROW_;
	void Downsample(int nChannel);
	UINT32 UpdatePostHeaderSize() THROW_;
	void WriteLevel() THROW_;

#ifdef __PGFROISUPPORT__
	void SetROI(PGFRect rect);
#endif

	UINT8 Clamp4(DataT v) const {
		if (v & 0xFFFFFFF0) return (v < 0) ? (UINT8)0: (UINT8)15; else return (UINT8)v;
	}	
	UINT16 Clamp6(DataT v) const {
		if (v & 0xFFFFFFC0) return (v < 0) ? (UINT16)0: (UINT16)63; else return (UINT16)v;
	}	
	UINT8 Clamp8(DataT v) const {
		// needs only one test in the normal case
		if (v & 0xFFFFFF00) return (v < 0) ? (UINT8)0 : (UINT8)255; else return (UINT8)v;
	}
	UINT16 Clamp16(DataT v) const {
		if (v & 0xFFFF0000) return (v < 0) ? (UINT16)0: (UINT16)65535; else return (UINT16)v;
	}	
	UINT32 Clamp31(DataT v) const {
		return (v < 0) ? 0 : (UINT32)v;
	}	
};

#endif //PGF_PGFIMAGE_H
