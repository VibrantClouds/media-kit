// This file is a part of media_kit
// (https://github.com/media-kit/media-kit).
//
// Copyright © 2021 & onwards, Hitesh Kumar Saini <saini123hitesh@gmail.com>.
// All rights reserved.
// Use of this source code is governed by MIT license that can be found in the
// LICENSE file.
#include "d3d11_surface_manager.h"

#include <iterator>
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")

D3D11SurfaceManager::D3D11SurfaceManager(int32_t width, int32_t height)
    : width_(width), height_(height) {
  const D3D_FEATURE_LEVEL feature_levels[] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
      D3D_FEATURE_LEVEL_9_3,
  };
  // Not |D3D11_CREATE_DEVICE_SINGLETHREADED|: d3d11va decodes on this device
  // from libmpv's decoder thread, & libmpv turns on multithread protection.
  auto hr = ::D3D11CreateDevice(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, feature_levels,
      static_cast<UINT>(std::size(feature_levels)), D3D11_SDK_VERSION,
      &device_, nullptr, &device_context_);
  if (FAILED(hr)) {
    throw std::runtime_error("Unable to create Windows Direct3D device.");
  }
  CreateTextures(width_, height_, internal_texture_, texture_, handle_);
  mutex_ = ::CreateMutex(NULL, FALSE, NULL);
}

D3D11SurfaceManager::~D3D11SurfaceManager() {
  ::CloseHandle(mutex_);
}

void D3D11SurfaceManager::SetSize(int32_t width, int32_t height) {
  if (width == width_ && height == height_) {
    return;
  }
  // Created before taking the mutex, so a failure leaves the current textures
  // in place & the mutex free for |Read|.
  Microsoft::WRL::ComPtr<ID3D11Texture2D> internal_texture, texture;
  HANDLE handle = nullptr;
  CreateTextures(width, height, internal_texture, texture, handle);
  // |Read| may be copying between the textures on Flutter's raster thread.
  ::WaitForSingleObject(mutex_, INFINITE);
  width_ = width;
  height_ = height;
  internal_texture_.Swap(internal_texture);
  texture_.Swap(texture);
  handle_ = handle;
  ::ReleaseMutex(mutex_);
}

void D3D11SurfaceManager::Draw(std::function<void()> callback) {
  ::WaitForSingleObject(mutex_, INFINITE);
  // |mpv_render_context_render| submits its work & returns, so there is no
  // |glFinish| style wait here.
  callback();
  ::ReleaseMutex(mutex_);
}

void D3D11SurfaceManager::Read() {
  ::WaitForSingleObject(mutex_, INFINITE);
  device_context_->CopyResource(texture_.Get(), internal_texture_.Get());
  device_context_->Flush();
  ::ReleaseMutex(mutex_);
}

void D3D11SurfaceManager::CreateTextures(
    int32_t width,
    int32_t height,
    Microsoft::WRL::ComPtr<ID3D11Texture2D>& internal_texture,
    Microsoft::WRL::ComPtr<ID3D11Texture2D>& texture,
    HANDLE& handle) {
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  // libmpv draws into |internal_texture|, & |Read| copies each frame to the
  // shared |texture|, so Flutter never reads a frame that is half drawn.
  auto hr = device_->CreateTexture2D(&desc, nullptr, &internal_texture);
  if (FAILED(hr)) {
    throw std::runtime_error("Unable to create Direct3D texture.");
  }
  desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
  hr = device_->CreateTexture2D(&desc, nullptr, &texture);
  if (FAILED(hr)) {
    throw std::runtime_error("Unable to create Direct3D texture.");
  }
  Microsoft::WRL::ComPtr<IDXGIResource> resource;
  hr = texture.As(&resource);
  if (FAILED(hr) || FAILED(resource->GetSharedHandle(&handle))) {
    throw std::runtime_error("Unable to retrieve Direct3D shared HANDLE.");
  }
}
