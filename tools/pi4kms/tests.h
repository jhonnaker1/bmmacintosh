//
// tests.h
//
// Shared by the direct display driver's PC tests (see test.cpp).
//
// This file is part of BMAtariST, a bare metal Atari ST emulator for
// the Raspberry Pi. See LICENSE for licensing details.
//

#ifndef BMATARIST_TOOLS_PI4KMS_TESTS_H_
#define BMATARIST_TOOLS_PI4KMS_TESTS_H_

extern int g_failed;
void Check(bool ok, const char *name, const char *what);

// test_modes.cpp: EDID, video modes, InfoFrames, PHY settings.
void RunModeTests(void);

#endif
