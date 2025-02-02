/* $Id: pxtcp.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * NAT Network - TCP proxy, internal interface declarations.
 */

/*
 * Copyright (C) 2013-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_NAT_pxtcp_h
#define VBOX_INCLUDED_SRC_NAT_pxtcp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "lwip/err.h"
#include "lwip/ip_addr.h"

struct pbuf;
struct tcp_pcb;
struct pxtcp;
struct fwspec;

err_t pxtcp_pcb_accept_outbound(struct tcp_pcb *, struct pbuf *, int, ipX_addr_t *, u16_t);

struct pxtcp *pxtcp_create_forwarded(SOCKET);
void pxtcp_cancel_forwarded(struct pxtcp *);

void pxtcp_pcb_connect(struct pxtcp *, const struct fwspec *);

int pxtcp_pmgr_add(struct pxtcp *);
void pxtcp_pmgr_del(struct pxtcp *);

#endif /* !VBOX_INCLUDED_SRC_NAT_pxtcp_h */
