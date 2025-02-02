/* $Id: GuestCtrlPrivate.cpp 84857 2020-06-17 07:46:06Z vboxsync $ */
/** @file
 * Internal helpers/structures for guest control functionality.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_GUEST_CONTROL
#include "LoggingNew.h"

#ifndef VBOX_WITH_GUEST_CONTROL
# error "VBOX_WITH_GUEST_CONTROL must defined in this file"
#endif
#include "GuestCtrlImplPrivate.h"
#include "GuestSessionImpl.h"
#include "VMMDev.h"

#include <iprt/asm.h>
#include <iprt/cpp/utils.h> /* For unconst(). */
#include <iprt/ctype.h>
#ifdef DEBUG
# include <iprt/file.h>
#endif
#include <iprt/fs.h>
#include <iprt/rand.h>
#include <iprt/time.h>
#include <VBox/AssertGuest.h>


/**
 * Extracts the timespec from a given stream block key.
 *
 * @return Pointer to handed-in timespec, or NULL if invalid / not found.
 * @param  strmBlk              Stream block to extract timespec from.
 * @param  strKey               Key to get timespec for.
 * @param  pTimeSpec            Where to store the extracted timespec.
 */
/* static */
PRTTIMESPEC GuestFsObjData::TimeSpecFromKey(const GuestProcessStreamBlock &strmBlk, const Utf8Str &strKey, PRTTIMESPEC pTimeSpec)
{
    AssertPtrReturn(pTimeSpec, NULL);

    Utf8Str strTime = strmBlk.GetString(strKey.c_str());
    if (strTime.isEmpty())
        return NULL;

    if (!RTTimeSpecFromString(pTimeSpec, strTime.c_str()))
        return NULL;

    return pTimeSpec;
}

/**
 * Extracts the nanoseconds relative from Unix epoch for a given stream block key.
 *
 * @return Nanoseconds relative from Unix epoch, or 0 if invalid / not found.
 * @param  strmBlk              Stream block to extract nanoseconds from.
 * @param  strKey               Key to get nanoseconds for.
 */
/* static */
int64_t GuestFsObjData::UnixEpochNsFromKey(const GuestProcessStreamBlock &strmBlk, const Utf8Str &strKey)
{
    RTTIMESPEC TimeSpec;
    if (!GuestFsObjData::TimeSpecFromKey(strmBlk, strKey, &TimeSpec))
        return 0;

    return TimeSpec.i64NanosecondsRelativeToUnixEpoch;
}

/**
 * Initializes this object data with a stream block from VBOXSERVICE_TOOL_LS.
 *
 * This is also used by FromStat since the output should be identical given that
 * they use the same output function on the guest side when fLong is true.
 *
 * @return VBox status code.
 * @param  strmBlk              Stream block to use for initialization.
 * @param  fLong                Whether the stream block contains long (detailed) information or not.
 */
int GuestFsObjData::FromLs(const GuestProcessStreamBlock &strmBlk, bool fLong)
{
    LogFlowFunc(("\n"));
#ifdef DEBUG
    strmBlk.DumpToLog();
#endif

    /* Object name. */
    mName = strmBlk.GetString("name");
    ASSERT_GUEST_RETURN(mName.isNotEmpty(), VERR_NOT_FOUND);

    /* Type & attributes. */
    bool fHaveAttribs = false;
    char szAttribs[32];
    memset(szAttribs, '?', sizeof(szAttribs) - 1);
    mType = FsObjType_Unknown;
    const char *psz = strmBlk.GetString("ftype");
    if (psz)
    {
        fHaveAttribs = true;
        szAttribs[0] = *psz;
        switch (*psz)
        {
            case '-':   mType = FsObjType_File; break;
            case 'd':   mType = FsObjType_Directory; break;
            case 'l':   mType = FsObjType_Symlink; break;
            case 'c':   mType = FsObjType_DevChar; break;
            case 'b':   mType = FsObjType_DevBlock; break;
            case 'f':   mType = FsObjType_Fifo; break;
            case 's':   mType = FsObjType_Socket; break;
            case 'w':   mType = FsObjType_WhiteOut; break;
            default:
                AssertMsgFailed(("%s\n", psz));
                szAttribs[0] = '?';
                fHaveAttribs = false;
                break;
        }
    }
    psz = strmBlk.GetString("owner_mask");
    if (   psz
        && (psz[0] == '-' || psz[0] == 'r')
        && (psz[1] == '-' || psz[1] == 'w')
        && (psz[2] == '-' || psz[2] == 'x'))
    {
        szAttribs[1] = psz[0];
        szAttribs[2] = psz[1];
        szAttribs[3] = psz[2];
        fHaveAttribs = true;
    }
    psz = strmBlk.GetString("group_mask");
    if (   psz
        && (psz[0] == '-' || psz[0] == 'r')
        && (psz[1] == '-' || psz[1] == 'w')
        && (psz[2] == '-' || psz[2] == 'x'))
    {
        szAttribs[4] = psz[0];
        szAttribs[5] = psz[1];
        szAttribs[6] = psz[2];
        fHaveAttribs = true;
    }
    psz = strmBlk.GetString("other_mask");
    if (   psz
        && (psz[0] == '-' || psz[0] == 'r')
        && (psz[1] == '-' || psz[1] == 'w')
        && (psz[2] == '-' || psz[2] == 'x'))
    {
        szAttribs[7] = psz[0];
        szAttribs[8] = psz[1];
        szAttribs[9] = psz[2];
        fHaveAttribs = true;
    }
    szAttribs[10] = ' '; /* Reserve three chars for sticky bits. */
    szAttribs[11] = ' ';
    szAttribs[12] = ' ';
    szAttribs[13] = ' '; /* Separator. */
    psz = strmBlk.GetString("dos_mask");
    if (   psz
        && (psz[ 0] == '-' || psz[ 0] == 'R')
        && (psz[ 1] == '-' || psz[ 1] == 'H')
        && (psz[ 2] == '-' || psz[ 2] == 'S')
        && (psz[ 3] == '-' || psz[ 3] == 'D')
        && (psz[ 4] == '-' || psz[ 4] == 'A')
        && (psz[ 5] == '-' || psz[ 5] == 'd')
        && (psz[ 6] == '-' || psz[ 6] == 'N')
        && (psz[ 7] == '-' || psz[ 7] == 'T')
        && (psz[ 8] == '-' || psz[ 8] == 'P')
        && (psz[ 9] == '-' || psz[ 9] == 'J')
        && (psz[10] == '-' || psz[10] == 'C')
        && (psz[11] == '-' || psz[11] == 'O')
        && (psz[12] == '-' || psz[12] == 'I')
        && (psz[13] == '-' || psz[13] == 'E'))
    {
        memcpy(&szAttribs[14], psz, 14);
        fHaveAttribs = true;
    }
    szAttribs[28] = '\0';
    if (fHaveAttribs)
        mFileAttrs = szAttribs;

    /* Object size. */
    int rc = strmBlk.GetInt64Ex("st_size", &mObjectSize);
    ASSERT_GUEST_RC_RETURN(rc, rc);
    strmBlk.GetInt64Ex("alloc", &mAllocatedSize);

    /* INode number and device. */
    psz = strmBlk.GetString("node_id");
    if (!psz)
        psz = strmBlk.GetString("cnode_id"); /* copy & past error fixed in 6.0 RC1 */
    if (psz)
        mNodeID = RTStrToInt64(psz);
    mNodeIDDevice = strmBlk.GetUInt32("inode_dev"); /* (Produced by GAs prior to 6.0 RC1.) */

    if (fLong)
    {
        /* Dates. */
        mAccessTime       = GuestFsObjData::UnixEpochNsFromKey(strmBlk, "st_atime");
        mBirthTime        = GuestFsObjData::UnixEpochNsFromKey(strmBlk, "st_birthtime");
        mChangeTime       = GuestFsObjData::UnixEpochNsFromKey(strmBlk, "st_ctime");
        mModificationTime = GuestFsObjData::UnixEpochNsFromKey(strmBlk, "st_mtime");

        /* Owner & group. */
        mUID = strmBlk.GetInt32("uid");
        psz = strmBlk.GetString("username");
        if (psz)
            mUserName = psz;
        mGID = strmBlk.GetInt32("gid");
        psz = strmBlk.GetString("groupname");
        if (psz)
            mGroupName = psz;

        /* Misc attributes: */
        mNumHardLinks = strmBlk.GetUInt32("hlinks", 1);
        mDeviceNumber = strmBlk.GetUInt32("st_rdev");
        mGenerationID = strmBlk.GetUInt32("st_gen");
        mUserFlags    = strmBlk.GetUInt32("st_flags");

        /** @todo ACL */
    }

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

/**
 * Parses stream block output data which came from the 'stat' (vbox_stat)
 * VBoxService toolbox command. The result will be stored in this object.
 *
 * @returns VBox status code.
 * @param   strmBlk             Stream block output data to parse.
 */
int GuestFsObjData::FromStat(const GuestProcessStreamBlock &strmBlk)
{
    /* Should be identical output. */
    return GuestFsObjData::FromLs(strmBlk, true /*fLong*/);
}

/**
 * Parses stream block output data which came from the 'mktemp' (vbox_mktemp)
 * VBoxService toolbox command. The result will be stored in this object.
 *
 * @returns VBox status code.
 * @param   strmBlk             Stream block output data to parse.
 */
int GuestFsObjData::FromMkTemp(const GuestProcessStreamBlock &strmBlk)
{
    LogFlowFunc(("\n"));

#ifdef DEBUG
    strmBlk.DumpToLog();
#endif
    /* Object name. */
    mName = strmBlk.GetString("name");
    ASSERT_GUEST_RETURN(mName.isNotEmpty(), VERR_NOT_FOUND);

    /* Assign the stream block's rc. */
    int rc = strmBlk.GetRc();

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Returns the IPRT-compatible file mode.
 * Note: Only handling RTFS_TYPE_ flags are implemented for now.
 *
 * @return IPRT file mode.
 */
RTFMODE GuestFsObjData::GetFileMode(void) const
{
    RTFMODE fMode = 0;

    switch (mType)
    {
        case FsObjType_Directory:
            fMode |= RTFS_TYPE_DIRECTORY;
            break;

        case FsObjType_File:
            fMode |= RTFS_TYPE_FILE;
            break;

        case FsObjType_Symlink:
            fMode |= RTFS_TYPE_SYMLINK;
            break;

        default:
            break;
    }

    /** @todo Implement more stuff. */

    return fMode;
}

///////////////////////////////////////////////////////////////////////////////

/** @todo *NOT* thread safe yet! */
/** @todo Add exception handling for STL stuff! */

GuestProcessStreamBlock::GuestProcessStreamBlock(void)
{

}

GuestProcessStreamBlock::~GuestProcessStreamBlock()
{
    Clear();
}

/**
 * Clears (destroys) the currently stored stream pairs.
 */
void GuestProcessStreamBlock::Clear(void)
{
    mPairs.clear();
}

#ifdef DEBUG
/**
 * Dumps the currently stored stream pairs to the (debug) log.
 */
void GuestProcessStreamBlock::DumpToLog(void) const
{
    LogFlowFunc(("Dumping contents of stream block=0x%p (%ld items):\n",
                 this, mPairs.size()));

    for (GuestCtrlStreamPairMapIterConst it = mPairs.begin();
         it != mPairs.end(); ++it)
    {
        LogFlowFunc(("\t%s=%s\n", it->first.c_str(), it->second.mValue.c_str()));
    }
}
#endif

/**
 * Returns a 64-bit signed integer of a specified key.
 *
 * @return VBox status code. VERR_NOT_FOUND if key was not found.
 * @param  pszKey               Name of key to get the value for.
 * @param  piVal                Pointer to value to return.
 */
int GuestProcessStreamBlock::GetInt64Ex(const char *pszKey, int64_t *piVal) const
{
    AssertPtrReturn(pszKey, VERR_INVALID_POINTER);
    AssertPtrReturn(piVal, VERR_INVALID_POINTER);
    const char *pszValue = GetString(pszKey);
    if (pszValue)
    {
        *piVal = RTStrToInt64(pszValue);
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}

/**
 * Returns a 64-bit integer of a specified key.
 *
 * @return  int64_t             Value to return, 0 if not found / on failure.
 * @param   pszKey              Name of key to get the value for.
 */
int64_t GuestProcessStreamBlock::GetInt64(const char *pszKey) const
{
    int64_t iVal;
    if (RT_SUCCESS(GetInt64Ex(pszKey, &iVal)))
        return iVal;
    return 0;
}

/**
 * Returns the current number of stream pairs.
 *
 * @return  uint32_t            Current number of stream pairs.
 */
size_t GuestProcessStreamBlock::GetCount(void) const
{
    return mPairs.size();
}

/**
 * Gets the return code (name = "rc") of this stream block.
 *
 * @return  VBox status code.
 */
int GuestProcessStreamBlock::GetRc(void) const
{
    const char *pszValue = GetString("rc");
    if (pszValue)
    {
        return RTStrToInt16(pszValue);
    }
    return VERR_NOT_FOUND;
}

/**
 * Returns a string value of a specified key.
 *
 * @return  uint32_t            Pointer to string to return, NULL if not found / on failure.
 * @param   pszKey              Name of key to get the value for.
 */
const char *GuestProcessStreamBlock::GetString(const char *pszKey) const
{
    AssertPtrReturn(pszKey, NULL);

    try
    {
        GuestCtrlStreamPairMapIterConst itPairs = mPairs.find(pszKey);
        if (itPairs != mPairs.end())
            return itPairs->second.mValue.c_str();
    }
    catch (const std::exception &ex)
    {
        RT_NOREF(ex);
    }
    return NULL;
}

/**
 * Returns a 32-bit unsigned integer of a specified key.
 *
 * @return  VBox status code. VERR_NOT_FOUND if key was not found.
 * @param   pszKey              Name of key to get the value for.
 * @param   puVal               Pointer to value to return.
 */
int GuestProcessStreamBlock::GetUInt32Ex(const char *pszKey, uint32_t *puVal) const
{
    const char *pszValue = GetString(pszKey);
    if (pszValue)
    {
        *puVal = RTStrToUInt32(pszValue);
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}

/**
 * Returns a 32-bit signed integer of a specified key.
 *
 * @returns 32-bit signed value
 * @param   pszKey              Name of key to get the value for.
 * @param   iDefault            The default to return on error if not found.
 */
int32_t GuestProcessStreamBlock::GetInt32(const char *pszKey, int32_t iDefault) const
{
    const char *pszValue = GetString(pszKey);
    if (pszValue)
    {
        int32_t iRet;
        int rc = RTStrToInt32Full(pszValue, 0, &iRet);
        if (RT_SUCCESS(rc))
            return iRet;
        ASSERT_GUEST_MSG_FAILED(("%s=%s\n", pszKey, pszValue));
    }
    return iDefault;
}

/**
 * Returns a 32-bit unsigned integer of a specified key.
 *
 * @return  uint32_t            Value to return, 0 if not found / on failure.
 * @param   pszKey              Name of key to get the value for.
 * @param   uDefault            The default value to return.
 */
uint32_t GuestProcessStreamBlock::GetUInt32(const char *pszKey, uint32_t uDefault /*= 0*/) const
{
    uint32_t uVal;
    if (RT_SUCCESS(GetUInt32Ex(pszKey, &uVal)))
        return uVal;
    return uDefault;
}

/**
 * Sets a value to a key or deletes a key by setting a NULL value.
 *
 * @return  VBox status code.
 * @param   pszKey              Key name to process.
 * @param   pszValue            Value to set. Set NULL for deleting the key.
 */
int GuestProcessStreamBlock::SetValue(const char *pszKey, const char *pszValue)
{
    AssertPtrReturn(pszKey, VERR_INVALID_POINTER);

    int rc = VINF_SUCCESS;
    try
    {
        Utf8Str Utf8Key(pszKey);

        /* Take a shortcut and prevent crashes on some funny versions
         * of STL if map is empty initially. */
        if (!mPairs.empty())
        {
            GuestCtrlStreamPairMapIter it = mPairs.find(Utf8Key);
            if (it != mPairs.end())
                 mPairs.erase(it);
        }

        if (pszValue)
        {
            GuestProcessStreamValue val(pszValue);
            mPairs[Utf8Key] = val;
        }
    }
    catch (const std::exception &ex)
    {
        RT_NOREF(ex);
    }
    return rc;
}

///////////////////////////////////////////////////////////////////////////////

GuestProcessStream::GuestProcessStream(void)
    : m_cbMax(_32M)
    , m_cbAllocated(0)
    , m_cbUsed(0)
    , m_offBuffer(0)
    , m_pbBuffer(NULL) { }

GuestProcessStream::~GuestProcessStream(void)
{
    Destroy();
}

/**
 * Adds data to the internal parser buffer. Useful if there
 * are multiple rounds of adding data needed.
 *
 * @return  VBox status code. Will return VERR_TOO_MUCH_DATA if the buffer's maximum (limit) has been reached.
 * @param   pbData              Pointer to data to add.
 * @param   cbData              Size (in bytes) of data to add.
 */
int GuestProcessStream::AddData(const BYTE *pbData, size_t cbData)
{
    AssertPtrReturn(pbData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    int rc = VINF_SUCCESS;

    /* Rewind the buffer if it's empty. */
    size_t     cbInBuf   = m_cbUsed - m_offBuffer;
    bool const fAddToSet = cbInBuf == 0;
    if (fAddToSet)
        m_cbUsed = m_offBuffer = 0;

    /* Try and see if we can simply append the data. */
    if (cbData + m_cbUsed <= m_cbAllocated)
    {
        memcpy(&m_pbBuffer[m_cbUsed], pbData, cbData);
        m_cbUsed += cbData;
    }
    else
    {
        /* Move any buffered data to the front. */
        cbInBuf = m_cbUsed - m_offBuffer;
        if (cbInBuf == 0)
            m_cbUsed = m_offBuffer = 0;
        else if (m_offBuffer) /* Do we have something to move? */
        {
            memmove(m_pbBuffer, &m_pbBuffer[m_offBuffer], cbInBuf);
            m_cbUsed = cbInBuf;
            m_offBuffer = 0;
        }

        /* Do we need to grow the buffer? */
        if (cbData + m_cbUsed > m_cbAllocated)
        {
            size_t cbAlloc = m_cbUsed + cbData;
            if (cbAlloc <= m_cbMax)
            {
                cbAlloc = RT_ALIGN_Z(cbAlloc, _64K);
                void *pvNew = RTMemRealloc(m_pbBuffer, cbAlloc);
                if (pvNew)
                {
                    m_pbBuffer = (uint8_t *)pvNew;
                    m_cbAllocated = cbAlloc;
                }
                else
                    rc = VERR_NO_MEMORY;
            }
            else
                rc = VERR_TOO_MUCH_DATA;
        }

        /* Finally, copy the data. */
        if (RT_SUCCESS(rc))
        {
            if (cbData + m_cbUsed <= m_cbAllocated)
            {
                memcpy(&m_pbBuffer[m_cbUsed], pbData, cbData);
                m_cbUsed += cbData;
            }
            else
                rc = VERR_BUFFER_OVERFLOW;
        }
    }

    return rc;
}

/**
 * Destroys the internal data buffer.
 */
void GuestProcessStream::Destroy(void)
{
    if (m_pbBuffer)
    {
        RTMemFree(m_pbBuffer);
        m_pbBuffer = NULL;
    }

    m_cbAllocated = 0;
    m_cbUsed = 0;
    m_offBuffer = 0;
}

#ifdef DEBUG
/**
 * Dumps the raw guest process output to a file on the host.
 * If the file on the host already exists, it will be overwritten.
 *
 * @param   pszFile             Absolute path to host file to dump the output to.
 */
void GuestProcessStream::Dump(const char *pszFile)
{
    LogFlowFunc(("Dumping contents of stream=0x%p (cbAlloc=%u, cbSize=%u, cbOff=%u) to %s\n",
                 m_pbBuffer, m_cbAllocated, m_cbUsed, m_offBuffer, pszFile));

    RTFILE hFile;
    int rc = RTFileOpen(&hFile, pszFile, RTFILE_O_CREATE_REPLACE | RTFILE_O_WRITE | RTFILE_O_DENY_WRITE);
    if (RT_SUCCESS(rc))
    {
        rc = RTFileWrite(hFile, m_pbBuffer, m_cbUsed, NULL /* pcbWritten */);
        RTFileClose(hFile);
    }
}
#endif

/**
 * Tries to parse the next upcoming pair block within the internal
 * buffer.
 *
 * Returns VERR_NO_DATA is no data is in internal buffer or buffer has been
 * completely parsed already.
 *
 * Returns VERR_MORE_DATA if current block was parsed (with zero or more pairs
 * stored in stream block) but still contains incomplete (unterminated)
 * data.
 *
 * Returns VINF_SUCCESS if current block was parsed until the next upcoming
 * block (with zero or more pairs stored in stream block).
 *
 * @return VBox status code.
 * @param streamBlock               Reference to guest stream block to fill.
 */
int GuestProcessStream::ParseBlock(GuestProcessStreamBlock &streamBlock)
{
    if (   !m_pbBuffer
        || !m_cbUsed)
    {
        return VERR_NO_DATA;
    }

    AssertReturn(m_offBuffer <= m_cbUsed, VERR_INVALID_PARAMETER);
    if (m_offBuffer == m_cbUsed)
        return VERR_NO_DATA;

    int rc = VINF_SUCCESS;

    char    *pszOff    = (char*)&m_pbBuffer[m_offBuffer];
    char    *pszStart  = pszOff;
    uint32_t uDistance;
    while (*pszStart)
    {
        size_t pairLen = strlen(pszStart);
        uDistance = (pszStart - pszOff);
        if (m_offBuffer + uDistance + pairLen + 1 >= m_cbUsed)
        {
            rc = VERR_MORE_DATA;
            break;
        }
        else
        {
            char *pszSep = strchr(pszStart, '=');
            char *pszVal = NULL;
            if (pszSep)
                pszVal = pszSep + 1;
            if (!pszSep || !pszVal)
            {
                rc = VERR_MORE_DATA;
                break;
            }

            /* Terminate the separator so that we can
             * use pszStart as our key from now on. */
            *pszSep = '\0';

            rc = streamBlock.SetValue(pszStart, pszVal);
            if (RT_FAILURE(rc))
                return rc;
        }

        /* Next pair. */
        pszStart += pairLen + 1;
    }

    /* If we did not do any movement but we have stuff left
     * in our buffer just skip the current termination so that
     * we can try next time. */
    uDistance = (pszStart - pszOff);
    if (   !uDistance
        && *pszStart == '\0'
        && m_offBuffer < m_cbUsed)
    {
        uDistance++;
    }
    m_offBuffer += uDistance;

    return rc;
}

GuestBase::GuestBase(void)
    : mConsole(NULL)
    , mNextContextID(RTRandU32() % VBOX_GUESTCTRL_MAX_CONTEXTS)
{
}

GuestBase::~GuestBase(void)
{
}

/**
 * Separate initialization function for the base class.
 *
 * @returns VBox status code.
 */
int GuestBase::baseInit(void)
{
    int rc = RTCritSectInit(&mWaitEventCritSect);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Separate uninitialization function for the base class.
 */
void GuestBase::baseUninit(void)
{
    LogFlowThisFuncEnter();

    /* Make sure to cancel any outstanding wait events. */
    int rc2 = cancelWaitEvents();
    AssertRC(rc2);

    rc2 = RTCritSectDelete(&mWaitEventCritSect);
    AssertRC(rc2);

    LogFlowFuncLeaveRC(rc2);
    /* No return value. */
}

/**
 * Cancels all outstanding wait events.
 *
 * @returns VBox status code.
 */
int GuestBase::cancelWaitEvents(void)
{
    LogFlowThisFuncEnter();

    int rc = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc))
    {
        GuestEventGroup::iterator itEventGroups = mWaitEventGroups.begin();
        while (itEventGroups != mWaitEventGroups.end())
        {
            GuestWaitEvents::iterator itEvents = itEventGroups->second.begin();
            while (itEvents != itEventGroups->second.end())
            {
                GuestWaitEvent *pEvent = itEvents->second;
                AssertPtr(pEvent);

                /*
                 * Just cancel the event, but don't remove it from the
                 * wait events map. Don't delete it though, this (hopefully)
                 * is done by the caller using unregisterWaitEvent().
                 */
                int rc2 = pEvent->Cancel();
                AssertRC(rc2);

                ++itEvents;
            }

            ++itEventGroups;
        }

        int rc2 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Handles generic messages not bound to a specific object type.
 *
 * @return VBox status code. VERR_NOT_FOUND if no handler has been found or VERR_NOT_SUPPORTED
 *         if this class does not support the specified callback.
 * @param  pCtxCb               Host callback context.
 * @param  pSvcCb               Service callback data.
 */
int GuestBase::dispatchGeneric(PVBOXGUESTCTRLHOSTCBCTX pCtxCb, PVBOXGUESTCTRLHOSTCALLBACK pSvcCb)
{
    LogFlowFunc(("pCtxCb=%p, pSvcCb=%p\n", pCtxCb, pSvcCb));

    AssertPtrReturn(pCtxCb, VERR_INVALID_POINTER);
    AssertPtrReturn(pSvcCb, VERR_INVALID_POINTER);

    int vrc;

    try
    {
        Log2Func(("uFunc=%RU32, cParms=%RU32\n", pCtxCb->uMessage, pSvcCb->mParms));

        switch (pCtxCb->uMessage)
        {
            case GUEST_MSG_PROGRESS_UPDATE:
                vrc = VINF_SUCCESS;
                break;

            case GUEST_MSG_REPLY:
            {
                if (pSvcCb->mParms >= 4)
                {
                    int idx = 1; /* Current parameter index. */
                    CALLBACKDATA_MSG_REPLY dataCb;
                    /* pSvcCb->mpaParms[0] always contains the context ID. */
                    vrc = HGCMSvcGetU32(&pSvcCb->mpaParms[idx++], &dataCb.uType);
                    AssertRCReturn(vrc, vrc);
                    vrc = HGCMSvcGetU32(&pSvcCb->mpaParms[idx++], &dataCb.rc);
                    AssertRCReturn(vrc, vrc);
                    vrc = HGCMSvcGetPv(&pSvcCb->mpaParms[idx++], &dataCb.pvPayload, &dataCb.cbPayload);
                    AssertRCReturn(vrc, vrc);

                    try
                    {
                        GuestWaitEventPayload evPayload(dataCb.uType, dataCb.pvPayload, dataCb.cbPayload);
                        vrc = signalWaitEventInternal(pCtxCb, dataCb.rc, &evPayload);
                    }
                    catch (int rcEx) /* Thrown by GuestWaitEventPayload constructor. */
                    {
                        vrc = rcEx;
                    }
                }
                else
                    vrc = VERR_INVALID_PARAMETER;
                break;
            }

            default:
                vrc = VERR_NOT_SUPPORTED;
                break;
        }
    }
    catch (std::bad_alloc &)
    {
        vrc = VERR_NO_MEMORY;
    }
    catch (int rc)
    {
        vrc = rc;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Generates a context ID (CID) by incrementing the object's count.
 * A CID consists of a session ID, an object ID and a count.
 *
 * Note: This function does not guarantee that the returned CID is unique;
 *       the caller has to take care of that and eventually retry.
 *
 * @returns VBox status code.
 * @param   uSessionID          Session ID to use for CID generation.
 * @param   uObjectID           Object ID to use for CID generation.
 * @param   puContextID         Where to store the generated CID on success.
 */
int GuestBase::generateContextID(uint32_t uSessionID, uint32_t uObjectID, uint32_t *puContextID)
{
    AssertPtrReturn(puContextID, VERR_INVALID_POINTER);

    if (   uSessionID >= VBOX_GUESTCTRL_MAX_SESSIONS
        || uObjectID  >= VBOX_GUESTCTRL_MAX_OBJECTS)
        return VERR_INVALID_PARAMETER;

    uint32_t uCount = ASMAtomicIncU32(&mNextContextID);
    uCount %= VBOX_GUESTCTRL_MAX_CONTEXTS;

    uint32_t uNewContextID = VBOX_GUESTCTRL_CONTEXTID_MAKE(uSessionID, uObjectID, uCount);

    *puContextID = uNewContextID;

#if 0
    LogFlowThisFunc(("mNextContextID=%RU32, uSessionID=%RU32, uObjectID=%RU32, uCount=%RU32, uNewContextID=%RU32\n",
                     mNextContextID, uSessionID, uObjectID, uCount, uNewContextID));
#endif
    return VINF_SUCCESS;
}

/**
 * Registers (creates) a new wait event based on a given session and object ID.
 *
 * From those IDs an unique context ID (CID) will be built, which only can be
 * around once at a time.
 *
 * @returns VBox status code.
 * @retval  VERR_GSTCTL_MAX_CID_COUNT_REACHED if unable to generate a free context ID (CID, the count part (bits 15:0)).
 * @param   uSessionID              Session ID to register wait event for.
 * @param   uObjectID               Object ID to register wait event for.
 * @param   ppEvent                 Pointer to registered (created) wait event on success.
 *                                  Must be destroyed with unregisterWaitEvent().
 */
int GuestBase::registerWaitEvent(uint32_t uSessionID, uint32_t uObjectID, GuestWaitEvent **ppEvent)
{
    GuestEventTypes eventTypesEmpty;
    return registerWaitEventEx(uSessionID, uObjectID, eventTypesEmpty, ppEvent);
}

/**
 * Creates and registers a new wait event object that waits on a set of events
 * related to a given object within the session.
 *
 * From the session ID and object ID a one-time unique context ID (CID) is built
 * for this wait object.  Normally the CID is then passed to the guest along
 * with a request, and the guest passed the CID back with the reply.  The
 * handler for the reply then emits a signal on the event type associated with
 * the reply, which includes signalling the object returned by this method and
 * the waking up the thread waiting on it.
 *
 * @returns VBox status code.
 * @retval  VERR_GSTCTL_MAX_CID_COUNT_REACHED if unable to generate a free context ID (CID, the count part (bits 15:0)).
 * @param   uSessionID              Session ID to register wait event for.
 * @param   uObjectID               Object ID to register wait event for.
 * @param   lstEvents               List of events to register the wait event for.
 * @param   ppEvent                 Pointer to registered (created) wait event on success.
 *                                  Must be destroyed with unregisterWaitEvent().
 */
int GuestBase::registerWaitEventEx(uint32_t uSessionID, uint32_t uObjectID, const GuestEventTypes &lstEvents,
                                   GuestWaitEvent **ppEvent)
{
    AssertPtrReturn(ppEvent, VERR_INVALID_POINTER);

    uint32_t idContext;
    int rc = generateContextID(uSessionID, uObjectID, &idContext);
    AssertRCReturn(rc, rc);

    GuestWaitEvent *pEvent = new GuestWaitEvent();
    AssertPtrReturn(pEvent, VERR_NO_MEMORY);

    rc = pEvent->Init(idContext, lstEvents);
    AssertRCReturn(rc, rc);

    LogFlowThisFunc(("New event=%p, CID=%RU32\n", pEvent, idContext));

    rc = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc))
    {
        /*
         * Check that we don't have any context ID collisions (should be very unlikely).
         *
         * The ASSUMPTION here is that mWaitEvents has all the same events as
         * mWaitEventGroups, so it suffices to check one of the two.
         */
        if (mWaitEvents.find(idContext) != mWaitEvents.end())
        {
            uint32_t cTries = 0;
            do
            {
                rc = generateContextID(uSessionID, uObjectID, &idContext);
                AssertRCBreak(rc);
                LogFunc(("Found context ID duplicate; trying a different context ID: %#x\n", idContext));
                if (mWaitEvents.find(idContext) != mWaitEvents.end())
                    rc = VERR_GSTCTL_MAX_CID_COUNT_REACHED;
            } while (RT_FAILURE_NP(rc) && cTries++ < 10);
        }
        if (RT_SUCCESS(rc))
        {
            /*
             * Insert event into matching event group. This is for faster per-group lookup of all events later.
             */
            uint32_t cInserts = 0;
            for (GuestEventTypes::const_iterator ItType = lstEvents.begin(); ItType != lstEvents.end(); ++ItType)
            {
                GuestWaitEvents &eventGroup = mWaitEventGroups[*ItType];
                if (eventGroup.find(idContext) == eventGroup.end())
                {
                    try
                    {
                        eventGroup.insert(std::pair<uint32_t, GuestWaitEvent *>(idContext, pEvent));
                        cInserts++;
                    }
                    catch (std::bad_alloc &)
                    {
                        while (ItType != lstEvents.begin())
                        {
                            --ItType;
                            mWaitEventGroups[*ItType].erase(idContext);
                        }
                        rc = VERR_NO_MEMORY;
                        break;
                    }
                }
                else
                    Assert(cInserts > 0); /* else: lstEvents has duplicate entries. */
            }
            if (RT_SUCCESS(rc))
            {
                Assert(cInserts > 0 || lstEvents.size() == 0);
                RT_NOREF(cInserts);

                /*
                 * Register event in the regular event list.
                 */
                try
                {
                    mWaitEvents[idContext] = pEvent;
                }
                catch (std::bad_alloc &)
                {
                    for (GuestEventTypes::const_iterator ItType = lstEvents.begin(); ItType != lstEvents.end(); ++ItType)
                        mWaitEventGroups[*ItType].erase(idContext);
                    rc = VERR_NO_MEMORY;
                }
            }
        }

        RTCritSectLeave(&mWaitEventCritSect);
    }
    if (RT_SUCCESS(rc))
    {
        *ppEvent = pEvent;
        return rc;
    }

    if (pEvent)
        delete pEvent;

    return rc;
}

/**
 * Signals all wait events of a specific type (if found)
 * and notifies external events accordingly.
 *
 * @returns VBox status code.
 * @param   aType               Event type to signal.
 * @param   aEvent              Which external event to notify.
 */
int GuestBase::signalWaitEvent(VBoxEventType_T aType, IEvent *aEvent)
{
    int rc = RTCritSectEnter(&mWaitEventCritSect);
#ifdef DEBUG
    uint32_t cEvents = 0;
#endif
    if (RT_SUCCESS(rc))
    {
        GuestEventGroup::iterator itGroup = mWaitEventGroups.find(aType);
        if (itGroup != mWaitEventGroups.end())
        {
            /* Signal all events in the group, leaving the group empty afterwards. */
            GuestWaitEvents::iterator ItWaitEvt;
            while ((ItWaitEvt = itGroup->second.begin()) != itGroup->second.end())
            {
                LogFlowThisFunc(("Signalling event=%p, type=%ld (CID %#x: Session=%RU32, Object=%RU32, Count=%RU32) ...\n",
                                 ItWaitEvt->second, aType, ItWaitEvt->first, VBOX_GUESTCTRL_CONTEXTID_GET_SESSION(ItWaitEvt->first),
                                 VBOX_GUESTCTRL_CONTEXTID_GET_OBJECT(ItWaitEvt->first), VBOX_GUESTCTRL_CONTEXTID_GET_COUNT(ItWaitEvt->first)));

                int rc2 = ItWaitEvt->second->SignalExternal(aEvent);
                AssertRC(rc2);

                /* Take down the wait event object details before we erase it from this list and invalid ItGrpEvt. */
                const GuestEventTypes &EvtTypes  = ItWaitEvt->second->Types();
                uint32_t               idContext = ItWaitEvt->first;
                itGroup->second.erase(ItWaitEvt);

                for (GuestEventTypes::const_iterator ItType = EvtTypes.begin(); ItType != EvtTypes.end(); ++ItType)
                {
                    GuestEventGroup::iterator EvtTypeGrp = mWaitEventGroups.find(*ItType);
                    if (EvtTypeGrp != mWaitEventGroups.end())
                    {
                        ItWaitEvt = EvtTypeGrp->second.find(idContext);
                        if (ItWaitEvt != EvtTypeGrp->second.end())
                        {
                            LogFlowThisFunc(("Removing event %p (CID %#x) from type %d group\n", ItWaitEvt->second, idContext, *ItType));
                            EvtTypeGrp->second.erase(ItWaitEvt);
                            LogFlowThisFunc(("%zu events left for type %d\n", EvtTypeGrp->second.size(), *ItType));
                            Assert(EvtTypeGrp->second.find(idContext) == EvtTypeGrp->second.end()); /* no duplicates */
                        }
                    }
                }
            }
        }

        int rc2 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

#ifdef DEBUG
    LogFlowThisFunc(("Signalled %RU32 events, rc=%Rrc\n", cEvents, rc));
#endif
    return rc;
}

/**
 * Signals a wait event which is registered to a specific callback (bound to a context ID (CID)).
 *
 * @returns VBox status code.
 * @param   pCbCtx              Pointer to host service callback context.
 * @param   rcGuest             Guest return code (rc) to set additionally, if rc is set to VERR_GSTCTL_GUEST_ERROR.
 * @param   pPayload            Additional wait event payload data set set on return. Optional.
 */
int GuestBase::signalWaitEventInternal(PVBOXGUESTCTRLHOSTCBCTX pCbCtx,
                                       int rcGuest, const GuestWaitEventPayload *pPayload)
{
    if (RT_SUCCESS(rcGuest))
        return signalWaitEventInternalEx(pCbCtx, VINF_SUCCESS,
                                         0 /* Guest rc */, pPayload);

    return signalWaitEventInternalEx(pCbCtx, VERR_GSTCTL_GUEST_ERROR,
                                     rcGuest, pPayload);
}

/**
 * Signals a wait event which is registered to a specific callback (bound to a context ID (CID)).
 * Extended version.
 *
 * @returns VBox status code.
 * @param   pCbCtx              Pointer to host service callback context.
 * @param   rc                  Return code (rc) to set as wait result.
 * @param   rcGuest             Guest return code (rc) to set additionally, if rc is set to VERR_GSTCTL_GUEST_ERROR.
 * @param   pPayload            Additional wait event payload data set set on return. Optional.
 */
int GuestBase::signalWaitEventInternalEx(PVBOXGUESTCTRLHOSTCBCTX pCbCtx,
                                         int rc, int rcGuest,
                                         const GuestWaitEventPayload *pPayload)
{
    AssertPtrReturn(pCbCtx, VERR_INVALID_POINTER);
    /* pPayload is optional. */

    int rc2 = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc2))
    {
        GuestWaitEvents::iterator itEvent = mWaitEvents.find(pCbCtx->uContextID);
        if (itEvent != mWaitEvents.end())
        {
            LogFlowThisFunc(("Signalling event=%p (CID %RU32, rc=%Rrc, rcGuest=%Rrc, pPayload=%p) ...\n",
                             itEvent->second, itEvent->first, rc, rcGuest, pPayload));
            GuestWaitEvent *pEvent = itEvent->second;
            AssertPtr(pEvent);
            rc2 = pEvent->SignalInternal(rc, rcGuest, pPayload);
        }
        else
            rc2 = VERR_NOT_FOUND;

        int rc3 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc2))
            rc2 = rc3;
    }

    return rc2;
}

/**
 * Unregisters (deletes) a wait event.
 *
 * After successful unregistration the event will not be valid anymore.
 *
 * @returns VBox status code.
 * @param   pWaitEvt        Wait event to unregister (delete).
 */
int GuestBase::unregisterWaitEvent(GuestWaitEvent *pWaitEvt)
{
    if (!pWaitEvt) /* Nothing to unregister. */
        return VINF_SUCCESS;

    int rc = RTCritSectEnter(&mWaitEventCritSect);
    if (RT_SUCCESS(rc))
    {
        LogFlowThisFunc(("pWaitEvt=%p\n", pWaitEvt));

/** @todo r=bird: One way of optimizing this would be to use the pointer
 * instead of the context ID as index into the groups, i.e. revert the value
 * pair for the GuestWaitEvents type.
 *
 * An even more efficent way, would be to not use sexy std::xxx containers for
 * the types, but iprt/list.h, as that would just be a RTListNodeRemove call for
 * each type w/o needing to iterate much at all.  I.e. add a struct {
 * RTLISTNODE, GuestWaitEvent *pSelf} array to GuestWaitEvent, and change
 * GuestEventGroup to std::map<VBoxEventType_T, RTListAnchorClass>
 * (RTListAnchorClass == RTLISTANCHOR wrapper with a constructor)).
 *
 * P.S. the try/catch is now longer needed after I changed pWaitEvt->Types() to
 * return a const reference rather than a copy of the type list (and it think it
 * is safe to assume iterators are not hitting the heap).  Copy vs reference is
 * an easy mistake to make in C++.
 *
 * P.P.S. The mWaitEventGroups optimization is probably just a lot of extra work
 * with little payoff.
 */
        try
        {
            /* Remove the event from all event type groups. */
            const GuestEventTypes &lstTypes = pWaitEvt->Types();
            for (GuestEventTypes::const_iterator itType = lstTypes.begin();
                 itType != lstTypes.end(); ++itType)
            {
                /** @todo Slow O(n) lookup. Optimize this. */
                GuestWaitEvents::iterator itCurEvent = mWaitEventGroups[(*itType)].begin();
                while (itCurEvent != mWaitEventGroups[(*itType)].end())
                {
                    if (itCurEvent->second == pWaitEvt)
                    {
                        mWaitEventGroups[(*itType)].erase(itCurEvent);
                        break;
                    }
                    ++itCurEvent;
                }
            }

            /* Remove the event from the general event list as well. */
            GuestWaitEvents::iterator itEvent = mWaitEvents.find(pWaitEvt->ContextID());

            Assert(itEvent != mWaitEvents.end());
            Assert(itEvent->second == pWaitEvt);

            mWaitEvents.erase(itEvent);

            delete pWaitEvt;
            pWaitEvt = NULL;
        }
        catch (const std::exception &ex)
        {
            RT_NOREF(ex);
            AssertFailedStmt(rc = VERR_NOT_FOUND);
        }

        int rc2 = RTCritSectLeave(&mWaitEventCritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    return rc;
}

/**
 * Waits for an already registered guest wait event.
 *
 * @return  VBox status code.
 * @retval  VERR_GSTCTL_GUEST_ERROR may be returned, call GuestResult() to get
 *          the actual result.
 *
 * @param   pWaitEvt                Pointer to event to wait for.
 * @param   msTimeout               Timeout (in ms) for waiting.
 * @param   pType                   Event type of following IEvent. Optional.
 * @param   ppEvent                 Pointer to IEvent which got triggered for this event. Optional.
 */
int GuestBase::waitForEvent(GuestWaitEvent *pWaitEvt, uint32_t msTimeout, VBoxEventType_T *pType, IEvent **ppEvent)
{
    AssertPtrReturn(pWaitEvt, VERR_INVALID_POINTER);
    /* pType is optional. */
    /* ppEvent is optional. */

    int vrc = pWaitEvt->Wait(msTimeout);
    if (RT_SUCCESS(vrc))
    {
        const ComPtr<IEvent> pThisEvent = pWaitEvt->Event();
        if (pThisEvent.isNotNull()) /* Make sure that we actually have an event associated. */
        {
            if (pType)
            {
                HRESULT hr = pThisEvent->COMGETTER(Type)(pType);
                if (FAILED(hr))
                    vrc = VERR_COM_UNEXPECTED;
            }
            if (   RT_SUCCESS(vrc)
                && ppEvent)
                pThisEvent.queryInterfaceTo(ppEvent);

            unconst(pThisEvent).setNull();
        }
    }

    return vrc;
}

#ifndef VBOX_GUESTCTRL_TEST_CASE
/**
 * Convenience function to return a pre-formatted string using an action description and a guest error information.
 *
 * @returns Pre-formatted string with a user-friendly error string.
 * @param   strAction           Action of when the error occurred.
 * @param   guestErrorInfo      Related guest error information to use.
 */
/* static */ Utf8Str GuestBase::getErrorAsString(const Utf8Str& strAction, const GuestErrorInfo& guestErrorInfo)
{
    Assert(strAction.isNotEmpty());
    return Utf8StrFmt("%s: %s", strAction.c_str(), getErrorAsString(guestErrorInfo).c_str());
}

/**
 * Returns a user-friendly error message from a given GuestErrorInfo object.
 *
 * @returns Error message string.
 * @param   guestErrorInfo      Guest error info to return error message for.
 */
/* static */ Utf8Str GuestBase::getErrorAsString(const GuestErrorInfo& guestErrorInfo)
{
    AssertMsg(RT_FAILURE(guestErrorInfo.getRc()), ("Guest rc does not indicate a failure\n"));

    Utf8Str strErr;

#define CASE_TOOL_ERROR(a_eType, a_strTool) \
    case a_eType: \
    { \
        strErr = GuestProcessTool::guestErrorToString(a_strTool, guestErrorInfo); \
        break; \
    }

    switch (guestErrorInfo.getType())
    {
        case GuestErrorInfo::Type_Session:
            strErr = GuestSession::i_guestErrorToString(guestErrorInfo.getRc());
            break;

        case GuestErrorInfo::Type_Process:
            strErr = GuestProcess::i_guestErrorToString(guestErrorInfo.getRc(), guestErrorInfo.getWhat().c_str());
            break;

        case GuestErrorInfo::Type_File:
            strErr = GuestFile::i_guestErrorToString(guestErrorInfo.getRc(), guestErrorInfo.getWhat().c_str());
            break;

        case GuestErrorInfo::Type_Directory:
            strErr = GuestDirectory::i_guestErrorToString(guestErrorInfo.getRc(), guestErrorInfo.getWhat().c_str());
            break;

        CASE_TOOL_ERROR(GuestErrorInfo::Type_ToolCat,    VBOXSERVICE_TOOL_CAT);
        CASE_TOOL_ERROR(GuestErrorInfo::Type_ToolLs,     VBOXSERVICE_TOOL_LS);
        CASE_TOOL_ERROR(GuestErrorInfo::Type_ToolMkDir,  VBOXSERVICE_TOOL_MKDIR);
        CASE_TOOL_ERROR(GuestErrorInfo::Type_ToolMkTemp, VBOXSERVICE_TOOL_MKTEMP);
        CASE_TOOL_ERROR(GuestErrorInfo::Type_ToolRm,     VBOXSERVICE_TOOL_RM);
        CASE_TOOL_ERROR(GuestErrorInfo::Type_ToolStat,   VBOXSERVICE_TOOL_STAT);

        default:
            AssertMsgFailed(("Type not implemented (type=%RU32, rc=%Rrc)\n", guestErrorInfo.getType(), guestErrorInfo.getRc()));
            strErr = Utf8StrFmt("Unknown / Not implemented -- Please file a bug report (type=%RU32, rc=%Rrc)\n",
                                guestErrorInfo.getType(), guestErrorInfo.getRc());
            break;
    }

    return strErr;
}

/**
 * Sets a guest error as error info, needed for API clients.
 *
 * @returns HRESULT COM error.
 * @param   pInterface          Interface to set error for.
 * @param   strAction           What action was involved causing this error.
 * @param   guestErrorInfo      Guest error info to use.
 */
/* static */ HRESULT GuestBase::setErrorExternal(VirtualBoxBase *pInterface,
                                                 const Utf8Str &strAction, const GuestErrorInfo &guestErrorInfo)
{
    AssertPtrReturn(pInterface, E_POINTER);
    return pInterface->setErrorBoth(VBOX_E_IPRT_ERROR,
                                    guestErrorInfo.getRc(),
                                    "%s", Utf8StrFmt("%s: %s", strAction.c_str(), GuestBase::getErrorAsString(guestErrorInfo).c_str()).c_str());
}
#endif /* VBOX_GUESTCTRL_TEST_CASE */

/**
 * Converts RTFMODE to FsObjType_T.
 *
 * @return  Converted FsObjType_T type.
 * @param   fMode               RTFMODE to convert.
 */
/* static */
FsObjType_T GuestBase::fileModeToFsObjType(RTFMODE fMode)
{
    if (RTFS_IS_FILE(fMode))           return FsObjType_File;
    else if (RTFS_IS_DIRECTORY(fMode)) return FsObjType_Directory;
    else if (RTFS_IS_SYMLINK(fMode))   return FsObjType_Symlink;

    return FsObjType_Unknown;
}

GuestObject::GuestObject(void)
    : mSession(NULL),
      mObjectID(0)
{
}

GuestObject::~GuestObject(void)
{
}

/**
 * Binds this guest (control) object to a specific guest (control) session.
 *
 * @returns VBox status code.
 * @param   pConsole            Pointer to console object to use.
 * @param   pSession            Pointer to session to bind this object to.
 * @param   uObjectID           Object ID for this object to use within that specific session.
 *                              Each object ID must be unique per session.
 */
int GuestObject::bindToSession(Console *pConsole, GuestSession *pSession, uint32_t uObjectID)
{
    AssertPtrReturn(pConsole, VERR_INVALID_POINTER);
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    mConsole  = pConsole;
    mSession  = pSession;
    mObjectID = uObjectID;

    return VINF_SUCCESS;
}

/**
 * Registers (creates) a new wait event.
 *
 * @returns VBox status code.
 * @param   lstEvents           List of events which the new wait event gets triggered at.
 * @param   ppEvent             Returns the new wait event on success.
 */
int GuestObject::registerWaitEvent(const GuestEventTypes &lstEvents,
                                   GuestWaitEvent **ppEvent)
{
    AssertPtr(mSession);
    return GuestBase::registerWaitEventEx(mSession->i_getId(), mObjectID, lstEvents, ppEvent);
}

/**
 * Sends a HGCM message to the guest (via the guest control host service).
 *
 * @returns VBox status code.
 * @param   uMessage            Message ID of message to send.
 * @param   cParms              Number of HGCM message parameters to send.
 * @param   paParms             Array of HGCM message parameters to send.
 */
int GuestObject::sendMessage(uint32_t uMessage, uint32_t cParms, PVBOXHGCMSVCPARM paParms)
{
#ifndef VBOX_GUESTCTRL_TEST_CASE
    ComObjPtr<Console> pConsole = mConsole;
    Assert(!pConsole.isNull());

    int vrc = VERR_HGCM_SERVICE_NOT_FOUND;

    /* Forward the information to the VMM device. */
    VMMDev *pVMMDev = pConsole->i_getVMMDev();
    if (pVMMDev)
    {
        /* HACK ALERT! We extend the first parameter to 64-bit and use the
                       two topmost bits for call destination information. */
        Assert(paParms[0].type == VBOX_HGCM_SVC_PARM_32BIT);
        paParms[0].type = VBOX_HGCM_SVC_PARM_64BIT;
        paParms[0].u.uint64 = (uint64_t)paParms[0].u.uint32 | VBOX_GUESTCTRL_DST_SESSION;

        /* Make the call. */
        LogFlowThisFunc(("uMessage=%RU32, cParms=%RU32\n", uMessage, cParms));
        vrc = pVMMDev->hgcmHostCall(HGCMSERVICE_NAME, uMessage, cParms, paParms);
        if (RT_FAILURE(vrc))
        {
            /** @todo What to do here? */
        }
    }
#else
    LogFlowThisFuncEnter();

    /* Not needed within testcases. */
    RT_NOREF(uMessage, cParms, paParms);
    int vrc = VINF_SUCCESS;
#endif
    return vrc;
}

GuestWaitEventBase::GuestWaitEventBase(void)
    : mfAborted(false),
      mCID(0),
      mEventSem(NIL_RTSEMEVENT),
      mRc(VINF_SUCCESS),
      mGuestRc(VINF_SUCCESS)
{
}

GuestWaitEventBase::~GuestWaitEventBase(void)
{
    if (mEventSem != NIL_RTSEMEVENT)
    {
        RTSemEventDestroy(mEventSem);
        mEventSem = NIL_RTSEMEVENT;
    }
}

/**
 * Initializes a wait event with a specific context ID (CID).
 *
 * @returns VBox status code.
 * @param   uCID                Context ID (CID) to initialize wait event with.
 */
int GuestWaitEventBase::Init(uint32_t uCID)
{
    mCID = uCID;

    return RTSemEventCreate(&mEventSem);
}

/**
 * Signals a wait event.
 *
 * @returns VBox status code.
 * @param   rc                  Return code (rc) to set as wait result.
 * @param   rcGuest             Guest return code (rc) to set additionally, if rc is set to VERR_GSTCTL_GUEST_ERROR.
 * @param   pPayload            Additional wait event payload data set set on return. Optional.
 */
int GuestWaitEventBase::SignalInternal(int rc, int rcGuest,
                                       const GuestWaitEventPayload *pPayload)
{
    if (mfAborted)
        return VERR_CANCELLED;

#ifdef VBOX_STRICT
    if (rc == VERR_GSTCTL_GUEST_ERROR)
        AssertMsg(RT_FAILURE(rcGuest), ("Guest error indicated but no actual guest error set (%Rrc)\n", rcGuest));
    else
        AssertMsg(RT_SUCCESS(rcGuest), ("No guest error indicated but actual guest error set (%Rrc)\n", rcGuest));
#endif

    int rc2;
    if (pPayload)
        rc2 = mPayload.CopyFromDeep(*pPayload);
    else
        rc2 = VINF_SUCCESS;
    if (RT_SUCCESS(rc2))
    {
        mRc = rc;
        mGuestRc = rcGuest;

        rc2 = RTSemEventSignal(mEventSem);
    }

    return rc2;
}

/**
 * Waits for the event to get triggered. Will return success if the
 * wait was successufl (e.g. was being triggered), otherwise an error will be returned.
 *
 * @returns VBox status code.
 * @retval  VERR_GSTCTL_GUEST_ERROR may be returned, call GuestResult() to get
 *          the actual result.
 *
 * @param   msTimeout           Timeout (in ms) to wait.
 *                              Specifiy 0 to wait indefinitely.
 */
int GuestWaitEventBase::Wait(RTMSINTERVAL msTimeout)
{
    int rc = VINF_SUCCESS;

    if (mfAborted)
        rc = VERR_CANCELLED;

    if (RT_SUCCESS(rc))
    {
        AssertReturn(mEventSem != NIL_RTSEMEVENT, VERR_CANCELLED);

        rc = RTSemEventWait(mEventSem, msTimeout ? msTimeout : RT_INDEFINITE_WAIT);
        if (   RT_SUCCESS(rc)
            && mfAborted)
        {
            rc = VERR_CANCELLED;
        }

        if (RT_SUCCESS(rc))
        {
            /* If waiting succeeded, return the overall
             * result code. */
            rc = mRc;
        }
    }

    return rc;
}

GuestWaitEvent::GuestWaitEvent(void)
{
}

GuestWaitEvent::~GuestWaitEvent(void)
{

}

/**
 * Cancels the event.
 */
int GuestWaitEvent::Cancel(void)
{
    if (mfAborted) /* Already aborted? */
        return VINF_SUCCESS;

    mfAborted = true;

#ifdef DEBUG_andy
    LogFlowThisFunc(("Cancelling %p ...\n"));
#endif
    return RTSemEventSignal(mEventSem);
}

/**
 * Initializes a wait event with a given context ID (CID).
 *
 * @returns VBox status code.
 * @param   uCID                Context ID to initialize wait event with.
 */
int GuestWaitEvent::Init(uint32_t uCID)
{
    return GuestWaitEventBase::Init(uCID);
}

/**
 * Initializes a wait event with a given context ID (CID) and a list of event types to wait for.
 *
 * @returns VBox status code.
 * @param   uCID                Context ID to initialize wait event with.
 * @param   lstEvents           List of event types to wait for this wait event to get signalled.
 */
int GuestWaitEvent::Init(uint32_t uCID, const GuestEventTypes &lstEvents)
{
    int rc = GuestWaitEventBase::Init(uCID);
    if (RT_SUCCESS(rc))
    {
        mEventTypes = lstEvents;
    }

    return rc;
}

/**
 * Signals the event.
 *
 * @return  VBox status code.
 * @param   pEvent              Public IEvent to associate.
 *                              Optional.
 */
int GuestWaitEvent::SignalExternal(IEvent *pEvent)
{
    if (pEvent)
        mEvent = pEvent;

    return RTSemEventSignal(mEventSem);
}

