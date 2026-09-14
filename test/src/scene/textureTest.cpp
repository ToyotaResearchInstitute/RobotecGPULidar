#include <gtest/gtest-death-test.h>

#include <helpers/geometryData.hpp>
#include <helpers/lidarHelpers.hpp>
#include <helpers/sceneHelpers.hpp>
#include <helpers/commonHelpers.hpp>
#include <helpers/textureHelpers.hpp>

#include <RGLFields.hpp>

#if RGL_BUILD_ROS2_EXTENSION
#include <rgl/api/extensions/ros2.h>
#endif

constexpr unsigned short UsMaxValue = 255;

struct TextureTest : public RGLTestWithParam<std::tuple<int, int, TextureTexelFormat>>
{};

INSTANTIATE_TEST_SUITE_P(Parametrized, TextureTest,
                         testing::Combine(testing::Values(1, 400), testing::Values(3, 16, 2000),
                                          testing::Values(0, UsMaxValue / 2, UsMaxValue)));

TEST_F(TextureTest, rgl_texture_invalid_argument)
{
	rgl_texture_t texture;
	std::vector<unsigned char> textureRawData;

	auto initializeArgumentsLambda = [&texture, &textureRawData]() {
		texture = nullptr;
		textureRawData = generateStaticColorTexture<TextureTexelFormat>(100, 100, 100);
	};
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create(nullptr, textureRawData.data(), 100, 100), "texture != nullptr");
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create(&texture, nullptr, 100, 100), "texels != nullptr");
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create(&texture, textureRawData.data(), -1, 100), "width > 0");
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create(&texture, textureRawData.data(), 100, 0), "height > 0");
}

TEST_F(TextureTest, rgl_texture_create_rgb_invalid_argument)
{
	rgl_texture_t texture;
	std::vector<uint8_t> textureRawData;

	auto initializeArgumentsLambda = [&texture, &textureRawData]() {
		texture = nullptr;
		textureRawData = generateStaticColorTextureRGB(100, 100, 10, 20, 30);
	};
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create_rgb(nullptr, textureRawData.data(), 100, 100), "texture != nullptr");
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create_rgb(&texture, nullptr, 100, 100), "texels != nullptr");
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create_rgb(&texture, textureRawData.data(), -1, 100), "width > 0");
	initializeArgumentsLambda();
	EXPECT_RGL_INVALID_ARGUMENT(rgl_texture_create_rgb(&texture, textureRawData.data(), 100, 0), "height > 0");
}

/**
 * Regression test for Texture::cleanup() reading uninitialized dTextureObject/dPixelArray when
 * createTextureObject() fails partway through (e.g. cudaMallocArray under memory pressure).
 * Width is chosen to exceed CUDA's max 2D texture dimension on any supported GPU, forcing that failure;
 * height is kept at 1 so the host-side texel buffer stays small.
 *
 * Run in a forked child, like ExternalLibraryTest.RclcppInitializeAndShutDownProperly, because
 * RGL_INTERNAL_EXCEPTION is unrecoverable by design (see canContinueAfterStatus in apiCommon.cpp).
 * It would otherwise poison every later test sharing this process, including this fixture's own
 * rgl_cleanup() teardown check.
 */
TEST_F(TextureTest, rgl_texture_create_rgb_reports_cuda_failure_without_crashing)
{
	::testing::GTEST_FLAG(death_test_style) = "threadsafe";
	ASSERT_EXIT(
	    {
		    constexpr int32_t width = 1 << 20;
		    constexpr int32_t height = 1;
		    auto textureRawData = generateStaticColorTextureRGB(width, height, 10, 20, 30);

		    rgl_texture_t texture = nullptr;
		    rgl_status_t status = rgl_texture_create_rgb(&texture, textureRawData.data(), width, height);

		    exit(status == RGL_INTERNAL_EXCEPTION && texture == nullptr ? 0 : 1);
	    },
	    ::testing::ExitedWithCode(0), "")
	    << "Expected rgl_texture_create_rgb to fail cleanly with RGL_INTERNAL_EXCEPTION when its CUDA "
	       "allocation fails, not crash the process.";
}

TEST_F(TextureTest, rgl_texture_create_rgb_succeeds)
{
	rgl_texture_t texture = nullptr;
	auto textureRawData = generateStaticColorTextureRGB(64, 32, 10, 20, 30);
	EXPECT_RGL_SUCCESS(rgl_texture_create_rgb(&texture, textureRawData.data(), 64, 32));
	EXPECT_RGL_SUCCESS(rgl_texture_destroy(texture));
}

TEST_F(TextureTest, rgl_entity_set_color_texture_rejects_grayscale_texture)
{
	rgl_texture_t grayscaleTexture = nullptr;
	auto grayscaleData = generateStaticColorTexture<TextureTexelFormat>(4, 4, 128);
	EXPECT_RGL_SUCCESS(rgl_texture_create(&grayscaleTexture, grayscaleData.data(), 4, 4));

	rgl_mesh_t mesh = makeCubeMesh();
	rgl_entity_t entity = nullptr;
	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));

	EXPECT_RGL_INVALID_ARGUMENT(rgl_entity_set_color_texture(entity, grayscaleTexture),
	                            "color texture must be created via rgl_texture_create_rgb");
}

TEST_F(TextureTest, rgl_entity_set_intensity_texture_rejects_rgb_texture)
{
	rgl_texture_t rgbTexture = nullptr;
	auto rgbData = generateStaticColorTextureRGB(4, 4, 10, 20, 30);
	EXPECT_RGL_SUCCESS(rgl_texture_create_rgb(&rgbTexture, rgbData.data(), 4, 4));

	rgl_mesh_t mesh = makeCubeMesh();
	rgl_entity_t entity = nullptr;
	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));

	EXPECT_RGL_INVALID_ARGUMENT(rgl_entity_set_intensity_texture(entity, rgbTexture),
	                            "intensity texture must be created via rgl_texture_create");
}

TEST_F(TextureTest, rgl_entity_set_color_texture_accepts_rgb_texture)
{
	rgl_texture_t rgbTexture = nullptr;
	auto rgbData = generateStaticColorTextureRGB(4, 4, 10, 20, 30);
	EXPECT_RGL_SUCCESS(rgl_texture_create_rgb(&rgbTexture, rgbData.data(), 4, 4));

	rgl_mesh_t mesh = makeCubeMesh();
	rgl_entity_t entity = nullptr;
	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));

	EXPECT_RGL_SUCCESS(rgl_entity_set_color_texture(entity, rgbTexture));
}

TEST_P(TextureTest, rgl_texture_reading)
{
	auto [width, height, value] = GetParam();

	rgl_texture_t texture = nullptr;
	rgl_entity_t entity = nullptr;
	rgl_mesh_t mesh = makeCubeMesh();
	auto textureRawData = generateStaticColorTexture<TextureTexelFormat>(width, height, value);

	EXPECT_RGL_SUCCESS(rgl_texture_create(&texture, textureRawData.data(), width, height));
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(mesh, cubeUVs, ARRAY_SIZE(cubeUVs)));

	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));
	EXPECT_RGL_SUCCESS(rgl_entity_set_intensity_texture(entity, texture));

	// Create RGL graph pipeline.
	rgl_node_t useRaysNode = nullptr, raytraceNode = nullptr, compactNode = nullptr, yieldNode = nullptr;

	std::vector<rgl_mat3x4f> rays = {// Ray must be incident perpendicular to the surface to receive all intensity
	                                 Mat3x4f::TRS({0, 0, 0}, {0, 0, 0}).toRGL()};

	std::vector<rgl_field_t> yieldFields = {INTENSITY_F32};

	EXPECT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, rays.data(), rays.size()));
	EXPECT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr));
	EXPECT_RGL_SUCCESS(rgl_node_points_compact_by_field(&compactNode, RGL_FIELD_IS_HIT_I32));
	EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, yieldFields.data(), yieldFields.size()));

	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(raytraceNode, compactNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(compactNode, yieldNode));

	EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));

	std::vector<::Field<INTENSITY_F32>::type> outIntensity;

	int32_t outCount, outSizeOf;
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_size(yieldNode, INTENSITY_F32, &outCount, &outSizeOf));
	EXPECT_EQ(outSizeOf, getFieldSize(INTENSITY_F32));

	outIntensity.resize(outCount);

	EXPECT_RGL_SUCCESS(rgl_graph_get_result_data(yieldNode, INTENSITY_F32, outIntensity.data()));

	for (int i = 0; i < outCount; ++i) {
		EXPECT_NEAR(((float) value), outIntensity.at(i), EPSILON_F);
	}
}

// Use-case test. Create texture, mesh and entity. Set texture to entity and run graph pipeline.
// As a result, we should get the cube with assigned gradient-checkerboard texture.
TEST_P(TextureTest, rgl_texture_use_case)
{
	auto [width, height, value] = GetParam();
	rgl_texture_t texture;
	rgl_mesh_t mesh;
	rgl_entity_t entity;

	// Create mesh with assigned texture.
	auto textureRawData = generateCheckerboardTexture<TextureTexelFormat>(width, height);
	mesh = makeCubeMesh();

	EXPECT_RGL_SUCCESS(rgl_texture_create(&texture, textureRawData.data(), width, height));
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(mesh, cubeUVs, 8));

	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));
	EXPECT_RGL_SUCCESS(rgl_entity_set_intensity_texture(entity, texture));

	//Create RGL graph pipeline.
	rgl_node_t useRaysNode = nullptr, raytraceNode = nullptr;

	std::vector<rgl_mat3x4f> rays = makeLidar3dRays(360, 360, 0.36, 0.36);

	std::vector<rgl_field_t> yieldFields = {XYZ_VEC3_F32, INTENSITY_F32, IS_HIT_I32};

	EXPECT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, rays.data(), rays.size()));
	EXPECT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr));

	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));

	EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));

	// Infinite loop to publish pointcloud to ROS2 topic for visualization purposes. Uncomment to use. Use wisely.
#ifdef RGL_BUILD_ROS2_EXTENSION

	//	rgl_node_t formatNode = nullptr, ros2publishNode = nullptr;
	//
	//	EXPECT_RGL_SUCCESS(rgl_node_points_format(&formatNode, yieldFields.data(), yieldFields.size()));
	//	EXPECT_RGL_SUCCESS(rgl_node_points_ros2_publish(&ros2publishNode, "pointcloud", "rgl"));
	//
	//	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(raytraceNode, formatNode));
	//	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(formatNode, ros2publishNode));
	//
	//	while(true)
	//		{
	//			EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));
	//			printf("Publishing pointcloud...\n");
	//		}

#endif
}

TEST_P(TextureTest, rgl_color_texture_reading)
{
	auto [width, height, value] = GetParam();
	const uint8_t r = value;
	const uint8_t g = 255 - value;
	const uint8_t b = value / 2;

	rgl_texture_t texture = nullptr;
	rgl_entity_t entity = nullptr;
	rgl_mesh_t mesh = makeCubeMesh();
	auto textureRawData = generateStaticColorTextureRGB(width, height, r, g, b);

	EXPECT_RGL_SUCCESS(rgl_texture_create_rgb(&texture, textureRawData.data(), width, height));
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(mesh, cubeUVs, ARRAY_SIZE(cubeUVs)));

	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));
	EXPECT_RGL_SUCCESS(rgl_entity_set_color_texture(entity, texture));

	rgl_node_t useRaysNode = nullptr, raytraceNode = nullptr, compactNode = nullptr, yieldNode = nullptr;

	std::vector<rgl_mat3x4f> rays = {Mat3x4f::TRS({0, 0, 0}, {0, 0, 0}).toRGL()};
	std::vector<rgl_field_t> yieldFields = {RGBA_U8};

	EXPECT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, rays.data(), rays.size()));
	EXPECT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr));
	EXPECT_RGL_SUCCESS(rgl_node_points_compact_by_field(&compactNode, RGL_FIELD_IS_HIT_I32));
	EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, yieldFields.data(), yieldFields.size()));

	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(raytraceNode, compactNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(compactNode, yieldNode));

	EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));

	int32_t outCount, outSizeOf;
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_size(yieldNode, RGBA_U8, &outCount, &outSizeOf));
	EXPECT_EQ(outSizeOf, getFieldSize(RGBA_U8));

	std::vector<RGBA8> outColor(outCount);
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_data(yieldNode, RGBA_U8, outColor.data()));

	for (int i = 0; i < outCount; ++i) {
		EXPECT_EQ(outColor.at(i).channels.r, r);
		EXPECT_EQ(outColor.at(i).channels.g, g);
		EXPECT_EQ(outColor.at(i).channels.b, b);
		EXPECT_EQ(outColor.at(i).channels.a, 255);
	}
}

TEST_F(TextureTest, rgl_intensity_and_color_texture_independent_simultaneous_reading)
{
	// Distinct, non-trivial values for both textures so a bug that shares/overwrites state between them would be caught.
	constexpr TextureTexelFormat intensityValue = 77;
	constexpr uint8_t r = 12, g = 200, b = 99;

	rgl_texture_t intensityTexture = nullptr;
	rgl_texture_t colorTexture = nullptr;
	rgl_entity_t entity = nullptr;
	rgl_mesh_t mesh = makeCubeMesh();

	auto intensityTextureRawData = generateStaticColorTexture<TextureTexelFormat>(4, 4, intensityValue);
	auto colorTextureRawData = generateStaticColorTextureRGB(4, 4, r, g, b);

	EXPECT_RGL_SUCCESS(rgl_texture_create(&intensityTexture, intensityTextureRawData.data(), 4, 4));
	EXPECT_RGL_SUCCESS(rgl_texture_create_rgb(&colorTexture, colorTextureRawData.data(), 4, 4));
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(mesh, cubeUVs, ARRAY_SIZE(cubeUVs)));

	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));
	EXPECT_RGL_SUCCESS(rgl_entity_set_intensity_texture(entity, intensityTexture));
	EXPECT_RGL_SUCCESS(rgl_entity_set_color_texture(entity, colorTexture));

	// Create RGL graph pipeline.
	rgl_node_t useRaysNode = nullptr, raytraceNode = nullptr, compactNode = nullptr, yieldNode = nullptr;

	std::vector<rgl_mat3x4f> rays = {// Ray must be incident perpendicular to the surface to receive all intensity
	                                 Mat3x4f::TRS({0, 0, 0}, {0, 0, 0}).toRGL()};

	std::vector<rgl_field_t> yieldFields = {INTENSITY_F32, RGBA_U8};

	EXPECT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, rays.data(), rays.size()));
	EXPECT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr));
	EXPECT_RGL_SUCCESS(rgl_node_points_compact_by_field(&compactNode, RGL_FIELD_IS_HIT_I32));
	EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, yieldFields.data(), yieldFields.size()));

	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(raytraceNode, compactNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(compactNode, yieldNode));

	EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));

	int32_t outIntensityCount, outIntensitySizeOf;
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_size(yieldNode, INTENSITY_F32, &outIntensityCount, &outIntensitySizeOf));
	EXPECT_EQ(outIntensitySizeOf, getFieldSize(INTENSITY_F32));

	std::vector<::Field<INTENSITY_F32>::type> outIntensity(outIntensityCount);
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_data(yieldNode, INTENSITY_F32, outIntensity.data()));

	int32_t outColorCount, outColorSizeOf;
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_size(yieldNode, RGBA_U8, &outColorCount, &outColorSizeOf));
	EXPECT_EQ(outColorSizeOf, getFieldSize(RGBA_U8));

	std::vector<RGBA8> outColor(outColorCount);
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_data(yieldNode, RGBA_U8, outColor.data()));

	ASSERT_EQ(outIntensityCount, outColorCount);
	for (int i = 0; i < outIntensityCount; ++i) {
		EXPECT_NEAR(((float) intensityValue), outIntensity.at(i), EPSILON_F);
		EXPECT_EQ(outColor.at(i).channels.r, r);
		EXPECT_EQ(outColor.at(i).channels.g, g);
		EXPECT_EQ(outColor.at(i).channels.b, b);
		EXPECT_EQ(outColor.at(i).channels.a, 255);
	}
}

TEST_P(TextureTest, rgl_color_texture_use_case)
{
	auto [width, height, value] = GetParam();
	rgl_texture_t texture;
	rgl_mesh_t mesh;
	rgl_entity_t entity;

	auto textureRawData = generateCheckerboardTextureRGB(width, height);
	mesh = makeCubeMesh();

	EXPECT_RGL_SUCCESS(rgl_texture_create_rgb(&texture, textureRawData.data(), width, height));
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(mesh, cubeUVs, 8));

	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));
	EXPECT_RGL_SUCCESS(rgl_entity_set_color_texture(entity, texture));

	rgl_node_t useRaysNode = nullptr, raytraceNode = nullptr;
	std::vector<rgl_mat3x4f> rays = makeLidar3dRays(360, 360, 0.36, 0.36);
	std::vector<rgl_field_t> yieldFields = {XYZ_VEC3_F32, RGBA_U8, IS_HIT_I32};

	EXPECT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, rays.data(), rays.size()));
	EXPECT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));
	EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));
}

TEST_F(TextureTest, rgl_color_texture_no_texture_assigned_fallback)
{
	rgl_mesh_t mesh = makeCubeMesh();
	rgl_entity_t entity = nullptr;
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(mesh, cubeUVs, ARRAY_SIZE(cubeUVs)));
	EXPECT_RGL_SUCCESS(rgl_entity_create(&entity, nullptr, mesh));
	// Note: no rgl_entity_set_color_texture call.

	rgl_node_t useRaysNode = nullptr, raytraceNode = nullptr, compactNode = nullptr, yieldNode = nullptr;
	std::vector<rgl_mat3x4f> rays = {Mat3x4f::TRS({0, 0, 0}, {0, 0, 0}).toRGL()};
	std::vector<rgl_field_t> yieldFields = {RGBA_U8};

	EXPECT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, rays.data(), rays.size()));
	EXPECT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr));
	EXPECT_RGL_SUCCESS(rgl_node_points_compact_by_field(&compactNode, RGL_FIELD_IS_HIT_I32));
	EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, yieldFields.data(), yieldFields.size()));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(raytraceNode, compactNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(compactNode, yieldNode));
	EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));

	int32_t outCount, outSizeOf;
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_size(yieldNode, RGBA_U8, &outCount, &outSizeOf));
	std::vector<RGBA8> outColor(outCount);
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_data(yieldNode, RGBA_U8, outColor.data()));

	ASSERT_GT(outCount, 0);
	for (int i = 0; i < outCount; ++i) {
		EXPECT_EQ(outColor.at(i).channels.r, 0);
		EXPECT_EQ(outColor.at(i).channels.g, 0);
		EXPECT_EQ(outColor.at(i).channels.b, 0);
		EXPECT_EQ(outColor.at(i).channels.a, 0);
	}
}

TEST_F(TextureTest, rgl_color_texture_validity_flag_mixed_entities)
{
	// Two separate cubes: one with a color texture, one without. One ray at each.
	rgl_mesh_t texturedMesh = makeCubeMesh();
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(texturedMesh, cubeUVs, ARRAY_SIZE(cubeUVs)));
	rgl_entity_t texturedEntity = nullptr;
	EXPECT_RGL_SUCCESS(rgl_entity_create(&texturedEntity, nullptr, texturedMesh));
	rgl_texture_t texture = nullptr;
	auto textureRawData = generateStaticColorTextureRGB(4, 4, 200, 100, 50);
	EXPECT_RGL_SUCCESS(rgl_texture_create_rgb(&texture, textureRawData.data(), 4, 4));
	EXPECT_RGL_SUCCESS(rgl_entity_set_color_texture(texturedEntity, texture));

	rgl_mesh_t plainMesh = makeCubeMesh();
	EXPECT_RGL_SUCCESS(rgl_mesh_set_texture_coords(plainMesh, cubeUVs, ARRAY_SIZE(cubeUVs)));
	rgl_entity_t plainEntity = nullptr;
	EXPECT_RGL_SUCCESS(rgl_entity_create(&plainEntity, nullptr, plainMesh));
	rgl_mat3x4f plainEntityPose = Mat3x4f::TRS({10, 0, 0}).toRGL();
	EXPECT_RGL_SUCCESS(rgl_entity_set_transform(plainEntity, &plainEntityPose));

	rgl_node_t useRaysNode = nullptr, raytraceNode = nullptr, compactNode = nullptr, yieldNode = nullptr;
	std::vector<rgl_mat3x4f> rays = {Mat3x4f::TRS({0, 0, 0}).toRGL(), Mat3x4f::TRS({10, 0, 0}).toRGL()};
	std::vector<rgl_field_t> yieldFields = {RGBA_U8};

	EXPECT_RGL_SUCCESS(rgl_node_rays_from_mat3x4f(&useRaysNode, rays.data(), rays.size()));
	EXPECT_RGL_SUCCESS(rgl_node_raytrace(&raytraceNode, nullptr));
	EXPECT_RGL_SUCCESS(rgl_node_points_compact_by_field(&compactNode, RGL_FIELD_IS_HIT_I32));
	EXPECT_RGL_SUCCESS(rgl_node_points_yield(&yieldNode, yieldFields.data(), yieldFields.size()));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(useRaysNode, raytraceNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(raytraceNode, compactNode));
	EXPECT_RGL_SUCCESS(rgl_graph_node_add_child(compactNode, yieldNode));
	EXPECT_RGL_SUCCESS(rgl_graph_run(raytraceNode));

	int32_t outCount, outSizeOf;
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_size(yieldNode, RGBA_U8, &outCount, &outSizeOf));
	ASSERT_EQ(outCount, 2);
	std::vector<RGBA8> outColor(outCount);
	EXPECT_RGL_SUCCESS(rgl_graph_get_result_data(yieldNode, RGBA_U8, outColor.data()));

	// First ray hits the textured cube: valid color.
	EXPECT_EQ(outColor.at(0).channels.a, 255);
	EXPECT_EQ(outColor.at(0).channels.r, 200);
	EXPECT_EQ(outColor.at(0).channels.g, 100);
	EXPECT_EQ(outColor.at(0).channels.b, 50);

	// Second ray hits the untextured cube: no color data.
	EXPECT_EQ(outColor.at(1).channels.a, 0);
	EXPECT_EQ(outColor.at(1).channels.r, 0);
	EXPECT_EQ(outColor.at(1).channels.g, 0);
	EXPECT_EQ(outColor.at(1).channels.b, 0);
}
