-- $Id: tmdb-r25-vcsrevisions-2.pgsql 84884 2020-06-20 21:53:12Z vboxsync $
--- @file
-- VBox Test Manager Database - Creates a new index on VcsRevisions
--

--
-- Copyright (C) 2013-2020 Oracle Corporation
--
-- This file is part of VirtualBox Open Source Edition (OSE), as
-- available from http://www.virtualbox.org. This file is free software;
-- you can redistribute it and/or modify it under the terms of the GNU
-- General Public License (GPL) as published by the Free Software
-- Foundation, in version 2 as it comes in the "COPYING" file of the
-- VirtualBox OSE distribution. VirtualBox OSE is distributed in the
-- hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
--
-- The contents of this file may alternatively be used under the terms
-- of the Common Development and Distribution License Version 1.0
-- (CDDL) only, as it comes in the "COPYING.CDDL" file of the
-- VirtualBox OSE distribution, in which case the provisions of the
-- CDDL are applicable instead of those of the GPL.
--
-- You may elect to license modified versions of this file under the
-- terms and conditions of either the GPL or the CDDL or both.
--

--
-- Die on error from now on.
--
\set ON_ERROR_STOP 1
\set AUTOCOMMIT 1


CREATE INDEX VcsRevisionsByDate ON VcsRevisions (tsCreated DESC);

