/* $Id: VBoxDbgDisas.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Debugger GUI - Disassembly View.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef DEBUGGER_INCLUDED_SRC_VBoxDbgDisas_h
#define DEBUGGER_INCLUDED_SRC_VBoxDbgDisas_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/**
 * Feature list:
 *  - Combobox (or some entry field with history similar to the command) for
 *    entering the address. Should support registers and other symbols, and
 *    possibly also grok various operators like the debugger command line.
 *    => Needs to make the DBGC evaluator available somehow.
 *  - Refresh manual/interval (for EIP or other non-fixed address expression).
 *  - Scrollable - down is not an issue, up is a bit more difficult.
 *  - Hide/Unhide PATM patches (jumps/int3/whatever) button in the guest disas.
 *  - Drop down for selecting mixed original guest disas and PATM/REM disas
 *    (Guest Only, Guest+PATM, Guest+REM).
 *
 */
class VBoxDbgDisas
{

};

#endif /* !DEBUGGER_INCLUDED_SRC_VBoxDbgDisas_h */

