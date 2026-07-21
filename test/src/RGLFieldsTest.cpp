#include <helpers/commonHelpers.hpp>
#include <helpers/testPointCloud.hpp>

#include <RGLFields.hpp>

class RGLFieldsTest : public RGLTest
{};

TEST_F(RGLFieldsTest, rgba_u8_is_four_bytes)
{
	EXPECT_EQ(getFieldSize(RGL_FIELD_RGBA_U8), 4);
	EXPECT_EQ(toString(RGL_FIELD_RGBA_U8), "RGBA_U8");
}

TEST_F(RGLFieldsTest, rgba_u8_round_trips_through_points_from_array_and_format)
{
	std::vector<rgl_field_t> fields{RGBA_U8};
	TestPointCloud inPointCloud(fields, 8);

	std::vector<RGBA8> values;
	for (int i = 0; i < 8; ++i) {
		values.push_back(RGBA8{static_cast<uint8_t>(i), static_cast<uint8_t>(i * 2), static_cast<uint8_t>(i * 3),
		                       static_cast<uint8_t>(i == 0 ? 0 : 255)});
	}
	inPointCloud.setFieldValues<RGBA_U8>(values);

	rgl_node_t inNode = inPointCloud.createUsePointsNode();
	rgl_node_t formatNode = nullptr;
	ASSERT_RGL_SUCCESS(rgl_node_points_format(&formatNode, fields.data(), fields.size()));
	ASSERT_RGL_SUCCESS(rgl_graph_node_add_child(inNode, formatNode));
	ASSERT_RGL_SUCCESS(rgl_graph_run(inNode));

	TestPointCloud outPointCloud = TestPointCloud::createFromFormatNode(formatNode, fields);
	auto outValues = outPointCloud.getFieldValues<RGBA_U8>();
	ASSERT_EQ(outValues.size(), values.size());
	for (size_t i = 0; i < values.size(); ++i) {
		EXPECT_EQ(outValues[i].r, values[i].r);
		EXPECT_EQ(outValues[i].g, values[i].g);
		EXPECT_EQ(outValues[i].b, values[i].b);
		EXPECT_EQ(outValues[i].a, values[i].a);
	}
}
