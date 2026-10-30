//
// layout.h
//
// Where the Mac's picture goes on the display, for every Pi: the
// framebuffer and the direct display driver on the Pi 4 and 5
// (src/av/video.cpp) and the firmware's layer on the others
// (src/kernel.cpp). Kept apart from them so the PC can test it
// (tools/pi4scaler/test_layout.cpp).
//
// The picture fills the display's height (times SCALE, the menu's Screen
// Size), its width following ASPECT, its shape on the screen. Sharp
// pictures are a whole number of display lines for each of the Mac's
// (REPEAT), so the lines are all alike. A picture wider than the display
// (a 4:3 or 3:2 Mac on a 5:4 monitor) is made smaller, never squashed:
// sharp, with fewer repeats; otherwise, as wide as the display and as tall
// as its shape makes it.
//
// This file is part of BMMacintosh, a bare metal 68k Macintosh emulator for
// the Raspberry Pi. See LICENSE for licensing details.
//

#ifndef BMMAC_AV_LAYOUT_H
#define BMMAC_AV_LAYOUT_H

struct PictureLayout {
  unsigned x, y, w, h;  // on the display, centred
  unsigned repeat;      // display lines a Mac line (sharp; 1 otherwise)
};

inline PictureLayout LayOutPicture(unsigned display_w, unsigned display_h,
                                   unsigned view_w, unsigned view_h,
                                   double aspect, double scale, bool sharp) {
  PictureLayout l = {0, 0, 1, 1, 1};
  if (display_w == 0 || display_h == 0 || view_w == 0 || view_h == 0 ||
      aspect <= 0.0) {
    return l;
  }
  if (scale <= 0.1 || scale > 1.0) {
    scale = 1.0;
  }
  unsigned h;
  if (sharp) {
    unsigned repeat = (unsigned)(display_h * scale) / view_h;
    while (repeat > 1 && repeat * view_h * aspect > display_w) {
      repeat--;
    }
    if (repeat == 0) {
      repeat = 1;
    }
    l.repeat = repeat;
    h = repeat * view_h;
  } else {
    h = (unsigned)(display_h * scale);
  }
  if (h > display_h) {
    h = display_h;  // a sharp picture taller than the display: shrunk
  }
  if (h == 0) {
    h = 1;
  }
  unsigned w = (unsigned)(h * aspect + 0.5);
  if (w > display_w) {
    w = display_w;
    h = (unsigned)(w / aspect + 0.5);
    if (h == 0) {
      h = 1;
    }
  }
  l.w = w;
  l.h = h;
  l.x = (display_w - w) / 2;
  l.y = (display_h - h) / 2;
  return l;
}

#endif
