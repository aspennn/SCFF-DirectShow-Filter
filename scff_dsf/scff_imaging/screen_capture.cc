﻿// Copyright 2012-2013 Alalf <alalf.iQLc_at_gmail.com>
//
// This file is part of SCFF-DirectShow-Filter(SCFF DSF).
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

/// @file scff_imaging/screen_capture.cc
/// scff_imaging::ScreenCaptureの定義

#include "scff_imaging/screen_capture.h"

#include "scff_imaging/debug.h"
#include "scff_imaging/utilities.h"

namespace scff_imaging {

//=====================================================================
// scff_imaging::ScreenCapture
//=====================================================================

ScreenCapture::ScreenCapture(
    bool vertical_invert,
    int count,
    const LayoutParameter (&parameters)[kMaxProcessorSize])
    : Processor<void, AVPictureWithFillImage>(count),
      vertical_invert_(vertical_invert) {
  DbgLog((kLogMemory, kTrace,
          TEXT("ScreenCapture: NEW(%d)"),
          count));
  // 配列の初期化
  for (int i = 0; i < kMaxProcessorSize; i++) {
    parameters_[i] = parameters[i];
    dc_for_bitblt_[i] = nullptr;
    raster_operation_[i] = SRCCOPY;
  }
  // 以下のメンバは明示的に初期化していない
  // image_for_bitblt_
  // info_for_getdibits_
}

ScreenCapture::~ScreenCapture() {
  DbgLog((kLogMemory, kTrace,
          TEXT("ScreenCapture: DELETE")));
  // 管理しているインスタンスをすべて破棄
  // 破棄はプロセッサ→イメージの順
  for (int i = 0; i < kMaxProcessorSize; i++) {
    if (dc_for_bitblt_[i] != nullptr) {
      DeleteDC(dc_for_bitblt_[i]);
      dc_for_bitblt_[i] = nullptr;
    }
  }
  // No Child Processor
}

ErrorCodes ScreenCapture::ValidateParameter(int index) {
  if (parameters_[index].ignore_valid_window) {
    DbgLog((LOG_TRACE, kTrace,
      TEXT("Validparameter: Ignore")));
    return ErrorCodes::kNoError;
  }
  // パラメータ
  HWND window = parameters_[index].window;
  const int clipping_x = parameters_[index].clipping_x;
  const int clipping_y = parameters_[index].clipping_y;
  const int clipping_width = parameters_[index].clipping_width;
  const int clipping_height = parameters_[index].clipping_height;

  // 不正なWindow
  if (window == nullptr || !IsWindow(window) || IsIconic(window)) {
    return ErrorCodes::kScreenCaptureInvalidWindowError;
  }

  // ウィンドウ領域を取得
  int window_x = -1;
  int window_y = -1;
  int window_width = -1;
  int window_height = -1;
  utilities::GetWindowRectangle(window,
      &window_x, &window_y, &window_width, &window_height);

  // クリッピング開始座標がウィンドウ領域に含まれているか
  if (!utilities::Contains(window_x, window_y, window_width, window_height,
                           clipping_x, clipping_y,
                           clipping_width, clipping_height)) {
    return ErrorCodes::kScreenCaptureInvalidClippingRegionError;
  }

  return ErrorCodes::kNoError;
}

ErrorCodes ScreenCapture::InitByIndex(int index) {
  ASSERT(0<= index && index < size());

  // RGB0以外の出力はできない
  ASSERT(GetOutputImage(index)->pixel_format() == ImagePixelFormats::kRGB0);

  // パラメータのチェック
  const ErrorCodes error_parameter = ValidateParameter(index);
  if (error_parameter != ErrorCodes::kNoError) {
    return error_parameter;
  }

  const int capture_width = parameters_[index].clipping_width;
  const int capture_height = parameters_[index].clipping_height;

  //-------------------------------------------------------------------
  // 初期化の順番はイメージ→プロセッサの順
  //-------------------------------------------------------------------
  // Image
  //-------------------------------------------------------------------
  // DDBはウィンドウのあるディスプレイと同じ形式=RGB0
  const ErrorCodes error_image_for_bitblt =
      image_for_bitblt_[index].CreateFromWindow(
          capture_width, capture_height,
          parameters_[index].window);
  if (error_image_for_bitblt != ErrorCodes::kNoError) {
    return error_image_for_bitblt;
  }
  //-------------------------------------------------------------------
  // Processor
  //-------------------------------------------------------------------
  // nop
  //-------------------------------------------------------------------

  // 取り込み用BITMAPINFOを作成
  utilities::ImageToWindowsBitmapInfo(
      image_for_bitblt_[index],
      vertical_invert_,
      &(info_for_getdibits_[index]));

  // 取り込み用DCを作成 (SelectObjectで過去の値は放棄)
  HDC window_dc = GetDC(parameters_[index].window);
  dc_for_bitblt_[index] = CreateCompatibleDC(window_dc);
  SelectObject(dc_for_bitblt_[index], image_for_bitblt_[index].windows_ddb());
  ReleaseDC(parameters_[index].window, window_dc);

  // BitBltに渡すラスターオペレーションコードを作成
  if (parameters_[index].show_layered_window) {
    raster_operation_[index] = SRCCOPY | CAPTUREBLT;
  } else {
    raster_operation_[index] = SRCCOPY;
  }

  // エラーなし
  return ErrorCodes::kNoError;
}

ErrorCodes ScreenCapture::Init() {
  DbgLog((kLogTrace, kTraceInfo,
          TEXT("ScreenCapture: Init(%d)"),
          size()));

  for (int i = 0; i < size(); i++) {
    const ErrorCodes error = InitByIndex(i);
    if (error != ErrorCodes::kNoError) {
      // 一つでもエラーが起きたら全てエラー扱いとする
      return ErrorOccured(error);
    }
  }

  // 初期化は成功
  return InitDone();
}

void ScreenCapture::DrawCursor(HDC dc, HWND window,
                               int clipping_x, int clipping_y) {
  POINT cursor_screen_point;
  GetCursorPos(&cursor_screen_point);
  POINT cursor_client_point = cursor_screen_point;
  ScreenToClient(window, &cursor_client_point);
  const int cursor_x = cursor_client_point.x - clipping_x;
  const int cursor_y = cursor_client_point.y - clipping_y;

  CURSORINFO cursor_info;
  ZeroMemory(&cursor_info, sizeof(cursor_info));
  cursor_info.cbSize = sizeof(cursor_info);
  cursor_info.flags = CURSOR_SHOWING;

  ICONINFO icon_info;

  if (GetCursorInfo(&cursor_info) &&
      GetIconInfo(cursor_info.hCursor, &icon_info)) {
    DrawIcon(dc,
             cursor_x - icon_info.xHotspot,
             cursor_y - icon_info.yHotspot,
             cursor_info.hCursor);

    DeleteObject(icon_info.hbmColor);
    DeleteObject(icon_info.hbmMask);
  }
}

ErrorCodes ScreenCapture::Run() {
  // 何かエラーが発生している場合は何もしない
  if (GetCurrentError() != ErrorCodes::kNoError) {
    return GetCurrentError();
  }

  // 全てのウインドウのチェック
  for (int i = 0; i < size(); i++) {
    const ErrorCodes error_parameter = ValidateParameter(i);
    if (error_parameter != ErrorCodes::kNoError) {
      return ErrorOccured(error_parameter);
    }
  }

  // まとめてスクリーンキャプチャ
  for (int i = 0; i < size(); i++) {
    // オンスクリーンDCの取得期間は最小限にすること！
    // なおVGAのキャッシュは取り込み画像に比べて小さすぎるので、
    // キャッシュミス関連で気をつけるべきことはない
    HDC window_dc = GetDC(parameters_[i].window);
	// 黒で塗りつぶす
	RECT rect;
	SetRect(&rect, 0, 0, parameters_[i].clipping_width, parameters_[i].clipping_height);
	FillRect(dc_for_bitblt_[i], &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	BitBlt(dc_for_bitblt_[i],
           0, 0,
           parameters_[i].clipping_width, parameters_[i].clipping_height,
           window_dc,
           parameters_[i].clipping_x, parameters_[i].clipping_y,
           raster_operation_[i]);
    ReleaseDC(parameters_[i].window, window_dc);
  }

  // 以下オフスクリーンビットマップに対する操作
  for (int i = 0; i < size(); i++) {
    if (parameters_[i].show_cursor) {
      DrawCursor(dc_for_bitblt_[i], parameters_[i].window,
                 parameters_[i].clipping_x, parameters_[i].clipping_y);
    }

    // OutputImageへの書き込み
    GetDIBits(dc_for_bitblt_[i],
              image_for_bitblt_[i].windows_ddb(),
              0, parameters_[i].clipping_height,
              GetOutputImage(i)->raw_bitmap(),
              &(info_for_getdibits_[i]),
              DIB_RGB_COLORS);
  }

  // エラー発生なし
  return GetCurrentError();
}

}   // namespace scff_imaging
