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

/// @file base/scff_output_pin_implement.cc
/// SCFFOutputPinの実装(Interfaceのみ)

#include "base/scff_output_pin.h"

#include "base/debug.h"
#include "base/constants.h"

//=====================================================================
// 各種インターフェースの実装
//=====================================================================

//---------------------------------------------------------------------
// IQualityControl
//---------------------------------------------------------------------

/// @retval E_FAIL
STDMETHODIMP SCFFOutputPin::Notify(IBaseFilter *self, Quality quality) {
  /// @attention Notifyは別スレッドから呼ばれることを確認
  return E_FAIL;
}

//---------------------------------------------------------------------
// IAMStreamConfig
//---------------------------------------------------------------------

/// - CoTaskMemAllocではプロセス単位の既定ヒープを使用してメモリを確保する
/// - COM内ではmallocよりもCoTaskMemAllocを使うべきである
/// @retval E_POINTER
/// @retval E_INVALIDARG
/// @retval VFW_S_NO_MORE_ITEMS
/// @retval E_OUTOFMEMORY
/// @retval S_OK
STDMETHODIMP SCFFOutputPin::GetPreferredFormat(int position,
                                 AM_MEDIA_TYPE **media_type) {
  CheckPointer(media_type, E_POINTER);

  CMediaType media_type_from_pin;
  HRESULT result = GetMediaType(position, &media_type_from_pin);
  if (FAILED(result)) return result;

  // ここで*media_typeに確保したメモリは呼び出し元が開放してくれる
  *media_type = static_cast<AM_MEDIA_TYPE*>(
                  CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));

  // CMediaTypeのインスタンスは代入でAM_MEDIA_TYPEへコピー可能
  **media_type = media_type_from_pin;

  // pbFormatはポインタなので新しく生成して内容をコピーしておく
  (**media_type).pbFormat = static_cast<BYTE*>(
                  CoTaskMemAlloc((**media_type).cbFormat));
  memcpy((**media_type).pbFormat, media_type_from_pin.Format(),
          (**media_type).cbFormat);

  return S_OK;
}

/// @retval E_POINTER
/// @retval E_INVALIDARG
/// @retval VFW_S_NO_MORE_ITEMS
/// @retval E_OUTOFMEMORY
/// @retval S_OK
STDMETHODIMP SCFFOutputPin::GetFormat(AM_MEDIA_TYPE **media_type) {
  DbgLog((kLogTrace, kTraceDebug,
          TEXT("[pin]<IAMStreamConfig> -> format")));
  return GetPreferredFormat(0, media_type);
}

/// @retval S_OK
/// @retval E_POINTER
STDMETHODIMP SCFFOutputPin::GetNumberOfCapabilities(
    int *count, int *size) {
  CheckPointer(count, E_POINTER);
  CheckPointer(size, E_POINTER);

  *count = kSupportedFormatsCount;
  *size = sizeof(VIDEO_STREAM_CONFIG_CAPS);

  DbgLog((kLogTrace, kTraceDebug,
          TEXT("[pin]<IAMStreamConfig> -> num of caps(%d)"),
          *count));
  return S_OK;
}

/// @retval E_POINTER
/// @retval E_INVALIDARG
/// @retval E_OUTOFMEMORY
/// @retval S_FALSE 指定されたインデックスの値が大きすぎる
/// @retval S_OK
STDMETHODIMP SCFFOutputPin::GetStreamCaps(
                              int position,
                              AM_MEDIA_TYPE **media_type,
                              BYTE *stream_caps) {
  CheckPointer(media_type, E_POINTER);
  CheckPointer(stream_caps, E_POINTER);

  if (position < 0) return E_INVALIDARG;

  HRESULT result = GetPreferredFormat(position, media_type);
  if (result == VFW_S_NO_MORE_ITEMS) return S_FALSE;
  if (FAILED(result)) return result;

  ASSERT((**media_type).formattype == FORMAT_VideoInfo);

  VIDEO_STREAM_CONFIG_CAPS *config_caps =
    reinterpret_cast<VIDEO_STREAM_CONFIG_CAPS*>(stream_caps);
  VIDEOINFO *video_info =
    reinterpret_cast<VIDEOINFO*>((**media_type).pbFormat);

  ZeroMemory(config_caps, sizeof(VIDEO_STREAM_CONFIG_CAPS));
  config_caps->VideoStandard        = 0;
  config_caps->guid                 = FORMAT_VideoInfo;
  config_caps->CropAlignX           = 1;
  config_caps->CropAlignY           = 1;
  config_caps->CropGranularityX     = 1;
  config_caps->CropGranularityY     = 1;
  config_caps->OutputGranularityX   = 1;
  config_caps->OutputGranularityY   = 1;
  config_caps->InputSize.cx         = video_info->bmiHeader.biWidth;
  config_caps->InputSize.cy         = abs(video_info->bmiHeader.biHeight);
  config_caps->MinOutputSize.cx     = kMinOutputWidth;
  config_caps->MinOutputSize.cy     = kMinOutputHeight;
  config_caps->MaxOutputSize.cx     = kMaxOutputWidth;
  config_caps->MaxOutputSize.cy     = kMaxOutputHeight;
  config_caps->MinCroppingSize      = config_caps->InputSize;
  config_caps->MaxCroppingSize      = config_caps->InputSize;
  config_caps->MinBitsPerSecond     = 1;
  config_caps->MaxBitsPerSecond     = 1;
  config_caps->MinFrameInterval     = kMinFrameInterval;
  config_caps->MaxFrameInterval     = kMaxFrameInterval;
  config_caps->StretchTapsX         = 0;
  config_caps->StretchTapsY         = 0;
  config_caps->ShrinkTapsX          = 0;
  config_caps->ShrinkTapsY          = 0;

  DbgLog((kLogTrace, kTraceDebug,
          TEXT("[pin]<IAMStreamConfig> -> stream caps(%d)"),
          position));
  return S_OK;
}

/// @retval E_POINTER
/// @retval E_UNEXPECTED
/// @retval E_INVALIDARG
/// @retval E_UNEXPECTED
/// @retval S_OK
STDMETHODIMP SCFFOutputPin::SetFormat(
                              AM_MEDIA_TYPE *media_type) {
  CheckPointer(media_type, E_POINTER);
  // コピーコンストラクタを利用してコピー
  CMediaType media_type_instance = *media_type;

  HRESULT result = SetMediaType(&media_type_instance);
  if (FAILED(result)) return result;

  DbgLog((kLogTrace, kTraceDebug,
          TEXT("[pin]<IAMStreamConfig> <- format")));
  return S_OK;
}

//---------------------------------------------------------------------
// IKsPropertySet
//---------------------------------------------------------------------

/// @retval E_NOTIMPL
STDMETHODIMP SCFFOutputPin::Set(REFGUID property_set_guid, DWORD property_id,
                              LPVOID instance_data, DWORD instance_data_size,
                              LPVOID property_data, DWORD property_data_size) {
  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: Set")));
  return E_NOTIMPL;
}

/// @retval E_PROP_SET_UNSUPPORTED
/// @retval E_PROP_ID_UNSUPPORTED
/// @retval E_POINTER
/// @retval E_UNEXPECTED
/// @retval S_OK
STDMETHODIMP SCFFOutputPin::Get(REFGUID property_set_guid, DWORD property_id,
                              LPVOID instance_data, DWORD instance_data_size,
                              LPVOID property_data, DWORD property_data_size,
                              DWORD *returned_data_size) {
  if (property_set_guid != AMPROPSETID_Pin) {
    return E_PROP_SET_UNSUPPORTED;
  }
  if (property_id != AMPROPERTY_PIN_CATEGORY) {
    return E_PROP_ID_UNSUPPORTED;
  }
  if (property_data == nullptr && returned_data_size == nullptr) {
    return E_POINTER;
  }
  if (returned_data_size != nullptr) {
    *returned_data_size = sizeof(GUID);
  }
  if (property_data == nullptr) {
    // 呼び出し元はサイズだけ知りたい。
    return S_OK;
  }
  if (property_data_size < sizeof(GUID)) {
    // バッファが小さすぎる。
    return E_UNEXPECTED;
  }

  // このPinはPIN_CATEGORY_CAPTUREである
  GUID *guid = reinterpret_cast<GUID*>(property_data);
  *guid = PIN_CATEGORY_CAPTURE;

  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: Get")));

  return S_OK;
}

/// @retval E_PROP_SET_UNSUPPORTED
/// @retval E_PROP_ID_UNSUPPORTED
/// @retval S_OK
STDMETHODIMP SCFFOutputPin::QuerySupported(REFGUID property_set_guid,
                              DWORD property_id, DWORD *support_type) {
  if (property_set_guid != AMPROPSETID_Pin) {
    return E_PROP_SET_UNSUPPORTED;
  }
  if (property_id != AMPROPERTY_PIN_CATEGORY) {
    return E_PROP_ID_UNSUPPORTED;
  }
  if (support_type != nullptr) {
    // このプロパティの取得はサポートしているが、設定はサポートしていない
    *support_type = KSPROPERTY_SUPPORT_GET;
  }

  return S_OK;
}

//---------------------------------------------------------------------
// IAMLatency
//---------------------------------------------------------------------

/// @retval S_OK
/// @retval E_POINTER
STDMETHODIMP SCFFOutputPin::GetLatency(REFERENCE_TIME *latency) {
  CheckPointer(latency, E_POINTER);

  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: GetLatency")));

  /// @todo(me) 値が不正確。固定値ではないはず。
  /// @attention 浮動小数点数の比較
  if (fps_ > 0.0) {
    *latency = ToFrameInterval(fps_);
  } else {
    *latency = 0LL;
  }

  return S_OK;
}

//---------------------------------------------------------------------
// IAMPushSource
//---------------------------------------------------------------------

/// @retval E_POINTER
/// @retval S_OK
STDMETHODIMP SCFFOutputPin::GetPushSourceFlags(ULONG *flags) {
  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: GetPushSourceFlags")));

  CheckPointer(flags, E_POINTER);
  *flags = 0;   // 設定なし = ライブソース
  return S_OK;
}

/// @retval E_NOTIMPL
STDMETHODIMP SCFFOutputPin::SetPushSourceFlags(ULONG flags) {
  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: SetPushSourceFlags")));

  return E_NOTIMPL;
}

/// @retval S_OK
STDMETHODIMP SCFFOutputPin::GetMaxStreamOffset(REFERENCE_TIME *max_offset) {
  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: GetMaxStreamOffset")));

  CheckPointer(max_offset, E_POINTER);
  // ダミーとして非常に大きな値を設定(１年)
  *max_offset = UNITS * (60 * 60 * 24 * 365);
  return S_OK;
}

/// @retval S_OK
STDMETHODIMP SCFFOutputPin::SetMaxStreamOffset(REFERENCE_TIME max_offset) {
  // max_offsetは設定できない
  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: SetMaxStreamOffset")));

  return S_OK;
}

/// @retval S_OK
/// @retval E_POINTER
STDMETHODIMP SCFFOutputPin::GetStreamOffset(REFERENCE_TIME *offset) {
  CheckPointer(offset, E_POINTER);

  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: GetStreamOffset")));

  *offset = offset_;
  return S_OK;
}

/// @retval S_OK
STDMETHODIMP SCFFOutputPin::SetStreamOffset(REFERENCE_TIME offset) {
  /// @todo(me) %lldではなく%"PRId64"が適切だがコンパイルエラーになる
  DbgLog((kLogTrace, kTraceDebug,
          TEXT("SCFFOutputPin: SetStreamOffset(%lld)"), offset));
  offset_ = offset;
  return S_OK;
}
