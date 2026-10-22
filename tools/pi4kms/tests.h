//
// tests.h
//
// Shared by the direct display driver's PC tests (see test.cpp).
//
// This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
// the Raspberry Pi. See LICENSE for licensing details.
//

#ifndef BMMAC_TOOLS_PI4KMS_TESTS_H_
#define BMMAC_TOOLS_PI4KMS_TESTS_H_

extern int g_failed;
void Check(bool ok, const char *name, const char *what);

// test_modes.cpp: EDID, video modes, InfoFrames, PHY settings.
void RunModeTests(void);

#endif
