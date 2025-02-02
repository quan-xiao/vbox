/* $Id: Recording.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * Recording code header.
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

#ifndef MAIN_INCLUDED_Recording_h
#define MAIN_INCLUDED_Recording_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/err.h>
#include <VBox/settings.h>

#include "RecordingStream.h"

class Console;

/**
 * Class for managing a recording context.
 */
class RecordingContext
{
public:

    RecordingContext(Console *pConsole, const settings::RecordingSettings &a_Settings);

    virtual ~RecordingContext(void);

public:

    const settings::RecordingSettings &GetConfig(void) const;
    RecordingStream *GetStream(unsigned uScreen) const;
    size_t GetStreamCount(void) const;

    int Create(const settings::RecordingSettings &a_Settings);
    void Destroy(void);

    int Start(void);
    int Stop(void);

    int SendAudioFrame(const void *pvData, size_t cbData, uint64_t uTimestampMs);
    int SendVideoFrame(uint32_t uScreen,
                       uint32_t x, uint32_t y, uint32_t uPixelFormat, uint32_t uBPP,
                       uint32_t uBytesPerLine, uint32_t uSrcWidth, uint32_t uSrcHeight,
                       uint8_t *puSrcData, uint64_t msTimestamp);
public:

    bool IsFeatureEnabled(RecordingFeature_T enmFeature);
    bool IsReady(void);
    bool IsReady(uint32_t uScreen, uint64_t msTimestamp);
    bool IsStarted(void);
    bool IsLimitReached(void);
    bool IsLimitReached(uint32_t uScreen, uint64_t msTimestamp);

    DECLCALLBACK(int) OnLimitReached(uint32_t uScreen, int rc);

protected:

    int createInternal(const settings::RecordingSettings &a_Settings);
    int startInternal(void);
    int stopInternal(void);

    void destroyInternal(void);

    RecordingStream *getStreamInternal(unsigned uScreen) const;

    int lock(void);
    int unlock(void);

    static DECLCALLBACK(int) threadMain(RTTHREAD hThreadSelf, void *pvUser);

    int threadNotify(void);

protected:

    /**
     * Enumeration for a recording context state.
     */
    enum RECORDINGSTS
    {
        /** Context not initialized. */
        RECORDINGSTS_UNINITIALIZED = 0,
        /** Context was created. */
        RECORDINGSTS_CREATED       = 1,
        /** Context was started. */
        RECORDINGSTS_STARTED       = 2,
        /** The usual 32-bit hack. */
        RECORDINGSTS_32BIT_HACK    = 0x7fffffff
    };

    /** Pointer to the console object. */
    Console                     *pConsole;
    /** Used recording configuration. */
    settings::RecordingSettings  Settings;
    /** The current state. */
    RECORDINGSTS                 enmState;
    /** Critical section to serialize access. */
    RTCRITSECT                   CritSect;
    /** Semaphore to signal the encoding worker thread. */
    RTSEMEVENT                   WaitEvent;
    /** Shutdown indicator. */
    bool                         fShutdown;
    /** Worker thread. */
    RTTHREAD                     Thread;
    /** Vector of current recording streams.
     *  Per VM screen (display) one recording stream is being used. */
    RecordingStreams             vecStreams;
    /** Number of streams in vecStreams which currently are enabled for recording. */
    uint16_t                     cStreamsEnabled;
    /** Timestamp (in ms) of when recording has been started. */
    uint64_t                     tsStartMs;
    /** Block map of common blocks which need to get multiplexed
     *  to all recording streams. This common block maps should help
     *  reducing the time spent in EMT and avoid doing the (expensive)
     *  multiplexing work in there.
     *
     *  For now this only affects audio, e.g. all recording streams
     *  need to have the same audio data at a specific point in time. */
    RecordingBlockMap            mapBlocksCommon;
};
#endif /* !MAIN_INCLUDED_Recording_h */

