/* $Id: RecordingUtils.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * Recording utility header.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_RecordingUtils_h
#define MAIN_INCLUDED_RecordingUtils_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/**
 * Iterator class for running through a BGRA32 image buffer and converting
 * it to RGB.
 */
class ColorConvBGRA32Iter
{
private:
    enum { PIX_SIZE = 4 };
public:
    ColorConvBGRA32Iter(unsigned aWidth, unsigned aHeight, uint8_t *aBuf)
    {
        mPos = 0;
        mSize = aWidth * aHeight * PIX_SIZE;
        mBuf = aBuf;
    }

    /**
     * Convert the next pixel to RGB.
     *
     * @returns true on success, false if we have reached the end of the buffer
     * @param   aRed            where to store the red value.
     * @param   aGreen          where to store the green value.
     * @param   aBlue           where to store the blue value.
     */
    bool getRGB(unsigned *aRed, unsigned *aGreen, unsigned *aBlue)
    {
        bool rc = false;
        if (mPos + PIX_SIZE <= mSize)
        {
            *aRed   = mBuf[mPos + 2];
            *aGreen = mBuf[mPos + 1];
            *aBlue  = mBuf[mPos    ];
            mPos += PIX_SIZE;
            rc = true;
        }
        return rc;
    }

    /**
     * Skip forward by a certain number of pixels.
     *
     * @param aPixels           How many pixels to skip.
     */
    void skip(unsigned aPixels)
    {
        mPos += PIX_SIZE * aPixels;
    }
private:
    /** Size of the picture buffer. */
    unsigned mSize;
    /** Current position in the picture buffer. */
    unsigned mPos;
    /** Address of the picture buffer. */
    uint8_t *mBuf;
};

/**
 * Iterator class for running through an BGR24 image buffer and converting
 * it to RGB.
 */
class ColorConvBGR24Iter
{
private:
    enum { PIX_SIZE = 3 };
public:
    ColorConvBGR24Iter(unsigned aWidth, unsigned aHeight, uint8_t *aBuf)
    {
        mPos = 0;
        mSize = aWidth * aHeight * PIX_SIZE;
        mBuf = aBuf;
    }

    /**
     * Convert the next pixel to RGB.
     *
     * @returns true on success, false if we have reached the end of the buffer.
     * @param   aRed            where to store the red value.
     * @param   aGreen          where to store the green value.
     * @param   aBlue           where to store the blue value.
     */
    bool getRGB(unsigned *aRed, unsigned *aGreen, unsigned *aBlue)
    {
        bool rc = false;
        if (mPos + PIX_SIZE <= mSize)
        {
            *aRed   = mBuf[mPos + 2];
            *aGreen = mBuf[mPos + 1];
            *aBlue  = mBuf[mPos    ];
            mPos += PIX_SIZE;
            rc = true;
        }
        return rc;
    }

    /**
     * Skip forward by a certain number of pixels.
     *
     * @param aPixels           How many pixels to skip.
     */
    void skip(unsigned aPixels)
    {
        mPos += PIX_SIZE * aPixels;
    }
private:
    /** Size of the picture buffer. */
    unsigned mSize;
    /** Current position in the picture buffer. */
    unsigned mPos;
    /** Address of the picture buffer. */
    uint8_t *mBuf;
};

/**
 * Iterator class for running through an BGR565 image buffer and converting
 * it to RGB.
 */
class ColorConvBGR565Iter
{
private:
    enum { PIX_SIZE = 2 };
public:
    ColorConvBGR565Iter(unsigned aWidth, unsigned aHeight, uint8_t *aBuf)
    {
        mPos = 0;
        mSize = aWidth * aHeight * PIX_SIZE;
        mBuf = aBuf;
    }

    /**
     * Convert the next pixel to RGB.
     *
     * @returns true on success, false if we have reached the end of the buffer.
     * @param   aRed            Where to store the red value.
     * @param   aGreen          where to store the green value.
     * @param   aBlue           where to store the blue value.
     */
    bool getRGB(unsigned *aRed, unsigned *aGreen, unsigned *aBlue)
    {
        bool rc = false;
        if (mPos + PIX_SIZE <= mSize)
        {
            unsigned uFull =  (((unsigned) mBuf[mPos + 1]) << 8)
                             | ((unsigned) mBuf[mPos]);
            *aRed   = (uFull >> 8) & ~7;
            *aGreen = (uFull >> 3) & ~3 & 0xff;
            *aBlue  = (uFull << 3) & ~7 & 0xff;
            mPos += PIX_SIZE;
            rc = true;
        }
        return rc;
    }

    /**
     * Skip forward by a certain number of pixels.
     *
     * @param aPixels           How many pixels to skip.
     */
    void skip(unsigned aPixels)
    {
        mPos += PIX_SIZE * aPixels;
    }
private:
    /** Size of the picture buffer. */
    unsigned mSize;
    /** Current position in the picture buffer. */
    unsigned mPos;
    /** Address of the picture buffer. */
    uint8_t *mBuf;
};

int RecordingUtilsRGBToYUV(uint32_t uPixelFormat,
                           uint8_t *paDst, uint32_t uDstWidth, uint32_t uDstHeight,
                           uint8_t *paSrc, uint32_t uSrcWidth, uint32_t uSrcHeight);

#endif /* !MAIN_INCLUDED_RecordingUtils_h */

