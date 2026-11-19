// This file is a part of media_kit
// (https://github.com/media-kit/media-kit).
//
// Copyright © 2021 & onwards, Hitesh Kumar Saini <saini123hitesh@gmail.com>.
// All rights reserved.
// Use of this source code is governed by MIT license that can be found in the
// LICENSE file.

#ifndef SURFACE_MANAGER_H_
#define SURFACE_MANAGER_H_

#include <Windows.h>

#include <cstdint>
#include <functional>

// A Direct3D 11 texture that libmpv draws into and Flutter reads through a
// DXGI shared |HANDLE|. Implemented by |D3D11SurfaceManager| (libmpv's D3D11
// render API) & |ANGLESurfaceManager| (libmpv's OpenGL render API over ANGLE).
class SurfaceManager {
 public:
  virtual ~SurfaceManager() = default;

  virtual const int32_t width() const = 0;
  virtual const int32_t height() const = 0;
  virtual const HANDLE handle() const = 0;

  virtual void SetSize(int32_t width, int32_t height) = 0;

  virtual void Draw(std::function<void()> callback) = 0;

  virtual void Read() = 0;
};

#endif  // SURFACE_MANAGER_H_
