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

#include <scene/Texture.hpp>
#include <cuda_runtime.h>
#include <vector>
#include "RGLFields.hpp"

API_OBJECT_INSTANCE(Texture);

Texture::Texture(const void* texels, int width, int height, TextureKind kind)
try : resolution(width, height), kind(kind) {
	createTextureObject(texels, width, height);
}
catch (...) {
	cleanup();
	throw;
}

void Texture::createTextureObject(const void* texels, int width, int height)
{
	cudaResourceDesc res_desc = {};

	cudaChannelFormatDesc channel_desc = {};
	const void* uploadTexels = texels;
	std::vector<uchar4> paddedRgba; // Kept alive until cudaMemcpy2DToArray below
	int32_t pitch = 0;

	if (kind == TextureKind::GRAYSCALE) {
		channel_desc = cudaCreateChannelDesc<TextureTexelFormat>();
		pitch = width * static_cast<int32_t>(sizeof(TextureTexelFormat));
	} else {
		// RGB input is tightly packed (3 bytes/px); CUDA texture objects only support tex2D fetch
		// types with 1, 2 or 4 components, so pad to RGBA on the host before uploading.
		const auto* rgb = static_cast<const uint8_t*>(texels);
		paddedRgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height));
		for (size_t i = 0; i < paddedRgba.size(); ++i) {
			paddedRgba[i] = uchar4{rgb[i * 3 + 0], rgb[i * 3 + 1], rgb[i * 3 + 2], 255};
		}
		uploadTexels = paddedRgba.data();
		channel_desc = cudaCreateChannelDesc<uchar4>();
		pitch = width * static_cast<int32_t>(sizeof(uchar4));
	}

	// TODO prybicki
	// Should we leave it like this, or add new copiers in DeivceBuffer.hpp?
	// Current copyFromExternal and ensureDeviceCanFit are not working with cudaArray_t
	CHECK_CUDA(cudaMallocArray(&dPixelArray, &channel_desc, width, height));

	CHECK_CUDA(cudaMemcpy2DToArray(dPixelArray, 0, 0, uploadTexels, pitch, pitch, height, cudaMemcpyHostToDevice));

	res_desc.resType = cudaResourceTypeArray;
	res_desc.res.array.array = dPixelArray;

	cudaTextureDesc tex_desc = {};

	tex_desc.addressMode[0] = cudaAddressModeWrap;
	tex_desc.addressMode[1] = cudaAddressModeWrap;
	tex_desc.filterMode = cudaFilterModePoint;
	tex_desc.readMode = cudaReadModeElementType;
	tex_desc.normalizedCoords = 1;
	tex_desc.maxAnisotropy = 1;
	tex_desc.maxMipmapLevelClamp = 99;
	tex_desc.minMipmapLevelClamp = 0;
	tex_desc.mipmapFilterMode = cudaFilterModePoint;
	tex_desc.borderColor[0] = 1.0f;

	CHECK_CUDA(cudaCreateTextureObject(&dTextureObject, &res_desc, &tex_desc, nullptr));
}

Texture::~Texture() { cleanup(); }

void Texture::cleanup()
{
	cudaDestroyTextureObject(dTextureObject);
	dTextureObject = 0;
	if (dPixelArray != nullptr) {
		cudaFreeArray(dPixelArray);
		dPixelArray = nullptr;
	}
}
