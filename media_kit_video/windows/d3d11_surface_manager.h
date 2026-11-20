// This file is a part of media_kit
// (https://github.com/media-kit/media-kit).
//
// Copyright © 2021 & onwards, Hitesh Kumar Saini <saini123hitesh@gmail.com>.
// All rights reserved.
// Use of this source code is governed by MIT license that can be found in the
// LICENSE file.

#ifndef D3D11_SURFACE_MANAGER_H_
#define D3D11_SURFACE_MANAGER_H_

#include <Windows.h>

#include <d3d11.h>
#include <wrl.h>

#include <cstdint>
#include <functional>

#include "surface_manager.h"

// Owns the Direct3D 11 device & textures for libmpv's D3D11 render API
// (|MPV_RENDER_API_TYPE_D3D11|, libmpv 0.42 or newer). libmpv renders straight
// into |texture|, & |Read| copies it to the texture Flutter opens by |handle|.
class D3D11SurfaceManager : public SurfaceManager {
 public:
  const int32_t width() const override { return width_; }
  const int32_t height() const override { return height_; }
  const HANDLE handle() const override { return handle_; }
  ID3D11Device* device() const { return device_.Get(); }
  ID3D11Texture2D* texture() const { return internal_texture_.Get(); }

  D3D11SurfaceManager(int32_t width, int32_t height);

  ~D3D11SurfaceManager() override;

  void SetSize(int32_t width, int32_t height) override;

  void Draw(std::function<void()> callback) override;

  void Read() override;

 private:
  void CreateTextures(int32_t width,
                      int32_t height,
                      Microsoft::WRL::ComPtr<ID3D11Texture2D>& internal_texture,
                      Microsoft::WRL::ComPtr<ID3D11Texture2D>& texture,
                      HANDLE& handle);

  int32_t width_ = 1;
  int32_t height_ = 1;
  HANDLE handle_ = nullptr;

  // Sync |Draw| & |Read| calls. libmpv uses the immediate context while it
  // renders, & no other thread may use it at the same time.
  HANDLE mutex_ = nullptr;
  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> internal_texture_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture_;
};

#endif  // D3D11_SURFACE_MANAGER_H_
