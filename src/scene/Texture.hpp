// Copyright 2023 Robotec.AI
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <APIObject.hpp>
#include <math/Vector.hpp>
#include <rgl/api/core.h>

enum class TextureKind
{
	GRAYSCALE, // Single-channel intensity texture (existing behavior)
	RGB,       // 3-channel color texture, padded to RGBA internally for tex2D
};

struct Texture : APIObject<Texture>
{
	friend APIObject<Texture>;

	~Texture();

	Vec2i getResolution() const { return resolution; }

	size_t getWidth() const { return resolution.x(); }

	size_t getHeight() const { return resolution.y(); }

	cudaTextureObject_t getTextureObject() const { return dTextureObject; }

	TextureKind getKind() const { return kind; }


private:
	Texture(const void* texels, int width, int height) : Texture(texels, width, height, TextureKind::GRAYSCALE) {}

	Texture(const void* texels, int width, int height, TextureKind kind);

	Texture(const Texture&) = delete;            // non construction-copyable
	Texture& operator=(const Texture&) = delete; // non copyable

	void createTextureObject(const void* texels, int width, int height);

	void cleanup();

	Vec2i resolution{-1};
	TextureKind kind{TextureKind::GRAYSCALE};

	// Zero-initialized so that cleanup() is safe to call even if createTextureObject() throws
	// before these are assigned (e.g. cudaMallocArray failing under GPU memory pressure).
	cudaTextureObject_t dTextureObject{0};
	cudaArray_t dPixelArray{nullptr};
};