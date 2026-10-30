//
// test_layout.cpp
//
// Tests where the Mac's picture goes on the display (src/av/layout.h) on
// the PC: the Macs' screens on 16:9, 16:10 and 5:4 displays, sharp and
// filtered, at full and smaller Screen Sizes. On every one the picture
// fits the display, is centred, and keeps its shape; on a 5:4 monitor it
// has bars above and below, not at the sides (issue #3).
//
//   tools/pi4scaler/run_tests.sh
//
// This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
// the Raspberry Pi. See LICENSE for licensing details.
//

#include "../../src/av/layout.h"

#include <math.h>
#include <stdio.h>

static int g_failed = 0;

static void Check(bool ok, const char *name, const char *what) {
  printf("%s  %s: %s\n", ok ? "PASS" : "FAIL", name, what);
  if (!ok) {
    g_failed++;
  }
}

// The layout must be X,Y WxH (and REPEAT lines a Mac line).
static void Expect(const char *name, unsigned dw, unsigned dh, unsigned vw,
                   unsigned vh, double aspect, double scale, bool sharp,
                   unsigned x, unsigned y, unsigned w, unsigned h,
                   unsigned repeat) {
  PictureLayout l = LayOutPicture(dw, dh, vw, vh, aspect, scale, sharp);
  char what[160];
  snprintf(what, sizeof what, "%ux%u at %u,%u x%u (wanted %ux%u at %u,%u x%u)",
           l.w, l.h, l.x, l.y, l.repeat, w, h, x, y, repeat);
  Check(l.x == x && l.y == y && l.w == w && l.h == h && l.repeat == repeat,
        name, what);
}

// Whatever the sizes: on the display, centred, and its shape kept (to
// within a pixel's rounding).
static void Sane(unsigned dw, unsigned dh, unsigned vw, unsigned vh,
                 double scale, bool sharp) {
  double aspect = (double)vw / vh;
  PictureLayout l = LayOutPicture(dw, dh, vw, vh, aspect, scale, sharp);
  bool fits = l.x + l.w <= dw && l.y + l.h <= dh;
  bool centred = l.x == (dw - l.w) / 2 && l.y == (dh - l.h) / 2;
  double shape = (double)l.w / l.h;
  bool kept = fabs(shape - aspect) <= aspect / l.h + 1.0 / l.h + 1e-9;
  bool whole = !sharp || l.repeat * vh == l.h || l.h == dh || l.repeat == 1;
  if (!(fits && centred && kept && whole)) {
    char name[96];
    snprintf(name, sizeof name, "%ux%u on %ux%u, %s, scale %.2f", vw, vh, dw,
             dh, sharp ? "sharp" : "filtered", scale);
    char what[160];
    snprintf(what, sizeof what, "%ux%u at %u,%u x%u: %s%s%s%s", l.w, l.h,
             l.x, l.y, l.repeat, fits ? "" : "off the display ",
             centred ? "" : "not centred ", kept ? "" : "squashed ",
             whole ? "" : "uneven lines");
    Check(false, name, what);
  }
}

int main(void) {
  // A 5:4 monitor (issue #3).
  Expect("640x480 on 1280x1024, sharp", 1280, 1024, 640, 480, 640.0 / 480,
         1.0, true, 0, 32, 1280, 960, 2);
  Expect("640x480 on 1280x1024, filtered", 1280, 1024, 640, 480, 640.0 / 480,
         1.0, false, 0, 32, 1280, 960, 1);
  Expect("1280x1024 on 1280x1024, sharp: fills it", 1280, 1024, 1280, 1024,
         1.25, 1.0, true, 0, 0, 1280, 1024, 1);
  Expect("Mac Plus 512x342 on 1280x1024, sharp", 1280, 1024, 512, 342,
         512.0 / 342, 1.0, true, 128, 170, 1024, 684, 2);
  Expect("Mac Plus 512x342 on 1280x1024, filtered", 1280, 1024, 512, 342,
         512.0 / 342, 1.0, false, 0, 84, 1280, 855, 1);
  Expect("800x600 on 1280x1024, filtered", 1280, 1024, 800, 600, 800.0 / 600,
         1.0, false, 0, 32, 1280, 960, 1);

  // 16:9 displays: as before, the height filled, bars at the sides.
  Expect("640x480 on 1280x720, sharp", 1280, 720, 640, 480, 640.0 / 480, 1.0,
         true, 320, 120, 640, 480, 1);
  Expect("640x480 on 1920x1080, sharp", 1920, 1080, 640, 480, 640.0 / 480,
         1.0, true, 320, 60, 1280, 960, 2);
  Expect("640x480 on 1920x1080, filtered", 1920, 1080, 640, 480, 640.0 / 480,
         1.0, false, 240, 0, 1440, 1080, 1);
  Expect("800x600 on 1920x1080, CRT (filtered)", 1920, 1080, 800, 600,
         800.0 / 600, 1.0, false, 240, 0, 1440, 1080, 1);
  Expect("1024x768 on 1280x720, sharp: shrunk to fit", 1280, 720, 1024, 768,
         1024.0 / 768, 1.0, true, 160, 0, 960, 720, 1);
  Expect("1600x1200 on 1920x1080, sharp: shrunk to fit", 1920, 1080, 1600,
         1200, 1600.0 / 1200, 1.0, true, 240, 0, 1440, 1080, 1);
  Expect("640x480 on 1920x1080 at 90%, filtered", 1920, 1080, 640, 480,
         640.0 / 480, 0.9, false, 312, 54, 1296, 972, 1);

  // A 16:10 display (issue #2's 1920x1200).
  Expect("800x600 on 1920x1200, sharp", 1920, 1200, 800, 600, 800.0 / 600,
         1.0, true, 160, 0, 1600, 1200, 2);
  Expect("1600x1200 on 1920x1200, sharp: 1 to 1", 1920, 1200, 1600, 1200,
         1600.0 / 1200, 1.0, true, 160, 0, 1600, 1200, 1);

  // Nothing at all: no picture rather than a division by zero.
  Expect("no view", 1280, 720, 0, 0, 1.0, 1.0, true, 0, 0, 1, 1, 1);

  // Every Mac screen on every display, both ways, at a few sizes.
  static const unsigned displays[][2] = {{1280, 720}, {1920, 1080},
                                         {1920, 1200}, {1280, 1024},
                                         {1024, 768},  {3840, 2160},
                                         {1680, 1050}, {1366, 768}};
  static const unsigned screens[][2] = {{512, 342},  {640, 480},  {800, 600},
                                        {832, 624},  {1024, 768}, {1152, 870},
                                        {1280, 1024}, {1600, 1200}};
  static const double scales[] = {1.0, 0.95, 0.9, 0.8, 0.5};
  int before = g_failed;
  for (auto &d : displays) {
    for (auto &s : screens) {
      for (double scale : scales) {
        Sane(d[0], d[1], s[0], s[1], scale, true);
        Sane(d[0], d[1], s[0], s[1], scale, false);
      }
    }
  }
  if (g_failed == before) {
    Check(true, "every screen on every display",
          "on the display, centred, its shape kept");
  }

  printf("\n%s\n", g_failed ? "Layout tests FAILED." : "All layout tests passed.");
  return g_failed ? 1 : 0;
}
