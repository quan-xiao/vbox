/* $Id: DBGFInline.h 86728 2020-10-28 10:18:28Z vboxsync $ */
/** @file
 * DBGF - Internal header file containing the inlined functions.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VMM_INCLUDED_SRC_include_DBGFInline_h
#define VMM_INCLUDED_SRC_include_DBGFInline_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/**
 * Initializes the given L2 table entry with the given values.
 *
 * @returns nothing.
 * @param   pL2Entry            The L2 entry to intialize.
 * @param   hBp                 The breakpoint handle.
 * @param   GCPtr               The GC pointer used as the key (only the upper 6 bytes are used).
 * @param   idxL2Left           The left L2 table index.
 * @param   idxL2Right          The right L2 table index.
 * @param   iDepth              The depth of the node in the tree.
 */
DECLINLINE(void) dbgfBpL2TblEntryInit(PDBGFBPL2ENTRY pL2Entry, DBGFBP hBp, RTGCPTR GCPtr,
                                      uint32_t idxL2Left, uint32_t idxL2Right, uint8_t iDepth)
{
    uint64_t u64GCPtrKeyAndBpHnd1 =   ((uint64_t)hBp & DBGF_BP_L2_ENTRY_BP_1ST_MASK) << DBGF_BP_L2_ENTRY_BP_1ST_SHIFT
                                    | DBGF_BP_INT3_L2_KEY_EXTRACT_FROM_ADDR(GCPtr);
    uint64_t u64LeftRightIdxDepthBpHnd2 =   (((uint64_t)hBp & DBGF_BP_L2_ENTRY_BP_2ND_MASK) >> 16) << DBGF_BP_L2_ENTRY_BP_2ND_SHIFT
                                          | ((uint64_t)iDepth << DBGF_BP_L2_ENTRY_DEPTH_SHIFT)
                                          | ((uint64_t)idxL2Right << DBGF_BP_L2_ENTRY_RIGHT_IDX_SHIFT)
                                          | ((uint64_t)idxL2Left << DBGF_BP_L2_ENTRY_LEFT_IDX_SHIFT);

    ASMAtomicWriteU64(&pL2Entry->u64GCPtrKeyAndBpHnd1, u64GCPtrKeyAndBpHnd1);
    ASMAtomicWriteU64(&pL2Entry->u64LeftRightIdxDepthBpHnd2, u64LeftRightIdxDepthBpHnd2);
}


/**
 * Updates the given L2 table entry with the new pointers.
 *
 * @returns nothing.
 * @param   pL2Entry            The L2 entry to update.
 * @param   idxL2Left           The new left L2 table index.
 * @param   idxL2Right          The new right L2 table index.
 * @param   iDepth              The new depth of the tree.
 */
DECLINLINE(void) dbgfBpL2TblEntryUpdate(PDBGFBPL2ENTRY pL2Entry, uint32_t idxL2Left, uint32_t idxL2Right,
                                        uint8_t iDepth)
{
    uint64_t u64LeftRightIdxDepthBpHnd2 = ASMAtomicReadU64(&pL2Entry->u64LeftRightIdxDepthBpHnd2) & DBGF_BP_L2_ENTRY_BP_2ND_L2_ENTRY_MASK;
    u64LeftRightIdxDepthBpHnd2 |=   ((uint64_t)iDepth << DBGF_BP_L2_ENTRY_DEPTH_SHIFT)
                                  | ((uint64_t)idxL2Right << DBGF_BP_L2_ENTRY_RIGHT_IDX_SHIFT)
                                  | ((uint64_t)idxL2Left << DBGF_BP_L2_ENTRY_LEFT_IDX_SHIFT);

    ASMAtomicWriteU64(&pL2Entry->u64LeftRightIdxDepthBpHnd2, u64LeftRightIdxDepthBpHnd2);
}


/**
 * Updates the given L2 table entry with the left pointer.
 *
 * @returns nothing.
 * @param   pL2Entry            The L2 entry to update.
 * @param   idxL2Left           The new left L2 table index.
 * @param   iDepth              The new depth of the tree.
 */
DECLINLINE(void) dbgfBpL2TblEntryUpdateLeft(PDBGFBPL2ENTRY pL2Entry, uint32_t idxL2Left, uint8_t iDepth)
{
    uint64_t u64LeftRightIdxDepthBpHnd2 = ASMAtomicReadU64(&pL2Entry->u64LeftRightIdxDepthBpHnd2) & (  DBGF_BP_L2_ENTRY_BP_2ND_L2_ENTRY_MASK
                                                                                                     | DBGF_BP_L2_ENTRY_RIGHT_IDX_MASK);

    u64LeftRightIdxDepthBpHnd2 |=   ((uint64_t)iDepth << DBGF_BP_L2_ENTRY_DEPTH_SHIFT)
                                  | ((uint64_t)idxL2Left << DBGF_BP_L2_ENTRY_LEFT_IDX_SHIFT);

    ASMAtomicWriteU64(&pL2Entry->u64LeftRightIdxDepthBpHnd2, u64LeftRightIdxDepthBpHnd2);
}


/**
 * Updates the given L2 table entry with the right pointer.
 *
 * @returns nothing.
 * @param   pL2Entry            The L2 entry to update.
 * @param   idxL2Right          The new right L2 table index.
 * @param   iDepth              The new depth of the tree.
 */
DECLINLINE(void) dbgfBpL2TblEntryUpdateRight(PDBGFBPL2ENTRY pL2Entry, uint32_t idxL2Right, uint8_t iDepth)
{
    uint64_t u64LeftRightIdxDepthBpHnd2 = ASMAtomicReadU64(&pL2Entry->u64LeftRightIdxDepthBpHnd2) & (  DBGF_BP_L2_ENTRY_BP_2ND_L2_ENTRY_MASK
                                                                                                     | DBGF_BP_L2_ENTRY_LEFT_IDX_MASK);

    u64LeftRightIdxDepthBpHnd2 |=   ((uint64_t)iDepth << DBGF_BP_L2_ENTRY_DEPTH_SHIFT)
                                  | ((uint64_t)idxL2Right << DBGF_BP_L2_ENTRY_RIGHT_IDX_SHIFT);

    ASMAtomicWriteU64(&pL2Entry->u64LeftRightIdxDepthBpHnd2, u64LeftRightIdxDepthBpHnd2);
}

#endif /* !VMM_INCLUDED_SRC_include_DBGFInline_h */
