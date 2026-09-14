#pragma once

#include <vector>
#include <math.h>
#include <RGLFields.hpp>

template<typename T>
static std::vector<T> generateStaticColorTexture(int width, int height, T value)
{
	auto texels = std::vector<T>(width * height);

	for (int i = 0; i < width * height; ++i) {
		texels[i] = (T) value;
	}
	return texels;
}

template<typename T>
static std::vector<T> generateCheckerboardTexture(int width, int height)
{
	// Generate a sample texture with a grid pattern 16x16.
	int xGridSize = ceil(width / 16.0f);
	int yGridSize = ceil(height / 16.0f);
	int xStep = 0;
	int yStep = 0;

	auto texels = std::vector<T>(width * height);

	for (int i = 0; i < width; ++i) {
		for (int j = 0; j < height; ++j) {
			texels[i * height + j] = (T) yStep * 0.5f + (T) xStep * 0.5f;
			if (j % yGridSize == 0) {
				yStep += yGridSize;
			}
		}
		yStep = 0;
		if (i % xGridSize == 0) {
			xStep += xGridSize;
		}
	}

	return texels;
}

static std::vector<uint8_t> generateStaticColorTextureRGB(int width, int height, uint8_t r, uint8_t g, uint8_t b)
{
	const size_t numTexels = static_cast<size_t>(width) * height;
	constexpr size_t NUM_CHANNELS = 3;
	auto texels = std::vector<uint8_t>(numTexels * NUM_CHANNELS);
	for (size_t i = 0; i < numTexels; ++i) {
		texels[i * NUM_CHANNELS + 0] = r;
		texels[i * NUM_CHANNELS + 1] = g;
		texels[i * NUM_CHANNELS + 2] = b;
	}
	return texels;
}

static std::vector<uint8_t> generateCheckerboardTextureRGB(int width, int height)
{
	auto gray = generateCheckerboardTexture<TextureTexelFormat>(width, height);
	auto rgb = std::vector<uint8_t>(gray.size() * 3);
	for (size_t i = 0; i < gray.size(); ++i) {
		rgb[i * 3 + 0] = gray[i];
		rgb[i * 3 + 1] = gray[i];
		rgb[i * 3 + 2] = gray[i];
	}
	return rgb;
}