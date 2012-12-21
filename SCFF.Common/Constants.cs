﻿// Copyright 2012 Alalf <alalf.iQLc_at_gmail.com>
//
// This file is part of SCFF-DirectShow-Filter.
//
// SCFF DSF is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// SCFF DSF is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SCFF DSF.  If not, see <http://www.gnu.org/licenses/>.

namespace SCFF.Common {

using System.Collections.Generic;

public static class Constants {
  public const double MainWindowLeft = 32.0;
  public const double MainWindowTop = 32.0;
  public const double MainWindowWidth = 730.0;
  public const double MainWindowHeight = 545.0;
  public const double CompactMainWindowWidth = 280.0;
  public const double CompactMainWindowHeight = 280.0;

  // ItemsSourceを使いたくないのでディクショナリを二つ用意しておく
  public static Dictionary<Profile.SWScaleFlags, string> ResizeMethods =
      new Dictionary<Profile.SWScaleFlags, string>() {
    {Profile.SWScaleFlags.FastBilinear, "FastBilinear (fast bilinear)"},
    {Profile.SWScaleFlags.Bilinear, "Bilinear (bilinear)"},
    {Profile.SWScaleFlags.Bicubic, "Bicubic (bicubic)"},
    {Profile.SWScaleFlags.X, "X (experimental)"},
    {Profile.SWScaleFlags.Point, "Point (nearest neighbor)"},
    {Profile.SWScaleFlags.Area, "Area (averaging area)"},
    {Profile.SWScaleFlags.Bicublin, "Bicublin (luma bicubic, chroma bilinear)"},
    {Profile.SWScaleFlags.Gauss, "Gauss (gaussian)"},
    {Profile.SWScaleFlags.Sinc, "Sinc (sinc)"},
    {Profile.SWScaleFlags.Lanczos, "Lanczos (lanczos)"},
    {Profile.SWScaleFlags.Spline, "Spline (natural bicubic spline)"}
  };

  public static Dictionary<Profile.SWScaleFlags, int> ResizeMethodIndexes =
      new Dictionary<Profile.SWScaleFlags, int>() {
    {Profile.SWScaleFlags.FastBilinear, 0},
    {Profile.SWScaleFlags.Bilinear, 1},
    {Profile.SWScaleFlags.Bicubic, 2},
    {Profile.SWScaleFlags.X, 3},
    {Profile.SWScaleFlags.Point, 4},
    {Profile.SWScaleFlags.Area, 5},
    {Profile.SWScaleFlags.Bicublin, 6},
    {Profile.SWScaleFlags.Gauss, 7},
    {Profile.SWScaleFlags.Sinc, 8},
    {Profile.SWScaleFlags.Lanczos, 9},
    {Profile.SWScaleFlags.Spline, 10}
  };
}
}