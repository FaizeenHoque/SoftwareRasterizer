#include <algorithm>
#include <cstdio>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <cmath>

const int width = 64;
const int height = 64;

struct float3 {
	float x, y, z;
	float r, g, b;

	float3() = default;

	float3(float x, float y, float z)
		: x(x), y(y), z(z), r(x), g(y), b(z) {}

	float3 operator+(const float3& other) const {
		return float3(x + other.x, y + other.y, z + other.z);
	}
	float3 operator-(const float3& other) const {
		return float3(x - other.x, y - other.y, z-other.z);
	}
	float3 operator/(const float3& other) const {
		return float3(x / other.x, y / other.y, z / other.z);
	}
	float3 operator*(const float3& other) const {
		return float3(x * other.x, y * other.y, z * other.z);
	}
};
struct float2 {
	float x, y;

	float2() = default;
	float2(float x, float y)
		: x(x), y(y) {}

	float2 operator+(const float2& other) const {
		return float2(x + other.x, y + other.y);
	}
	float2 operator-(const float2& other) const {
		return float2(x - other.x, y - other.y);
	}
	float2 operator/(const float2& other) const {
		return float2(x / other.x, y / other.y);
	}
	float2 operator*(const float2& other) const {
		return float2(x * other.x, y * other.y);
	}

	float2& operator+=(const float2& other) {
		x += other.x;
		y += other.y;
		return *this;
	}
};

float Dot(float2 a, float2 b) {
	return a.x * b.x + a.y * b.y;
}
float2 Perpendicular(float2 vec) {
	return float2(vec.y, -vec.x);
}

bool PointOnRightSideOfLine(float2 a, float2 b, float2 p) {
	float2 ab = b - a;
	float2 abPerp = Perpendicular(ab);
	float2 ap = p - a;
	return Dot(ap, abPerp) >= 0;
}
bool PointInTriangle(float2 a, float2 b, float2 c, float2 p) {
	bool sideAB = PointOnRightSideOfLine(a, b, p);
	bool sideBC = PointOnRightSideOfLine(b, c, p);
	bool sideAC = PointOnRightSideOfLine(c, a, p);
	return sideAB == sideBC && sideBC == sideAC;
}

static std::string GetFilePath(const std::string& fileName) {
	std::filesystem::path outputDir = "Output";

	if (!std::filesystem::exists(outputDir)) {
		std::filesystem::create_directory(outputDir);
	}

	return (outputDir / fileName).string();
}
static void WriteImageToFile(const std::vector<std::vector<float3>>& image, const std::string& name) {
	std::ofstream writer(GetFilePath(name), std::ios::binary);

	uint32_t width = (uint32_t)image.size();
	uint32_t height = (uint32_t)image[0].size();
	uint32_t byteCounts[3] = { 14, 40, width * height * 4 }; // BMP header, DIP header, data

	// ------ Headers ------
	writer.write("BM", 2);

	uint32_t fileSize = byteCounts[0] + byteCounts[1] + byteCounts[2]; // total file size
	writer.write((const char*)&fileSize, sizeof(fileSize));

	uint32_t unused = 0;
	writer.write((const char*)&unused, sizeof(unused));

	uint32_t dataOffset = byteCounts[0] + byteCounts[1]; // data offset (from start)
	writer.write((const char*)&dataOffset, sizeof(dataOffset));

	writer.write((const char*)&byteCounts[1], sizeof(byteCounts[1])); // DIP header size
	writer.write((const char*)&width, sizeof(width)); // image width
	writer.write((const char*)&height, sizeof(height)); // image height

	uint16_t colourPlanes = 1;
	writer.write((const char*)&colourPlanes, sizeof(colourPlanes));

	uint16_t bitsPerPixel = 8 * 4; // 1 byte per channel, plus 1 for alignment
	writer.write((const char*)&bitsPerPixel, sizeof(bitsPerPixel));

	uint32_t compression = 0; // RGB format, no compression
	writer.write((const char*)&compression, sizeof(compression));

	writer.write((const char*)&byteCounts[2], sizeof(byteCounts[2])); // data size

	char pad16[16] = { 0 }; // print resolution and palette info (ignoring)
	writer.write(pad16, sizeof(pad16));

	// ------ Data ------
	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			float3 col = image[x][y];
			uint8_t r = (uint8_t)(col.r * 255);
			uint8_t g = (uint8_t)(col.g * 255);
			uint8_t b = (uint8_t)(col.b * 255);
			uint8_t pad = 0;

			writer.write((const char*)&b, 1);
			writer.write((const char*)&g, 1);
			writer.write((const char*)&r, 1);
			writer.write((const char*)&pad, 1);
		}
	}
}

void Render(
	const std::vector<float2>& points,
	const std::vector<float3>& triangleCols,
	std::vector<std::vector<float3>>& image
) {
	for (int i = 0; i < points.size(); i += 3) {
		float2 a = points[i];
		float2 b = points[i + 1];
		float2 c = points[i + 2];

		float minX = std::min(std::min(a.x, b.x), c.x);
		float minY = std::min(std::min(a.y, b.y), c.y);
		float maxX = std::max(std::max(a.x, b.x), c.x);
		float maxY = std::max(std::max(a.y, b.y), c.y);

		int blockStartX = std::clamp((int)minX, 0, (int)image.size() - 1);
		int blockStartY = std::clamp((int)minY, 0, (int)image[0].size() - 1);
		int blockEndX = std::clamp((int)std::ceil(maxX), 0, (int)image.size() - 1);
		int blockEndY = std::clamp((int)std::ceil(maxY), 0, (int)image[0].size() - 1);

		for (int y = blockStartY; y <= blockEndY; ++y) {
			for (int x = blockStartX; x <= blockEndX; ++x) {
				if (!PointInTriangle(a, b, c, float2(x, y))) {
					continue;
				}

				image[x][y] = triangleCols[i / 3];
			}
		}
	}
}

void CreateTestImages() {
	float2 trianglePosition(1, 1);

	for (int currentFrame = 0; currentFrame < 5; ++currentFrame) {
		std::vector image(width, std::vector<float3>(height));
		trianglePosition.x += 10;

		float2 a(0.2f * width, 0.2f * height);
		float2 b(0.7f * width, 0.4f * height);
		float2 c(0.4f * width, 0.8f * height);

		a += trianglePosition;
		b += trianglePosition;
		c += trianglePosition;

		std::vector<float2> points = {
			a, b, c
		};

		std::vector<float3> triangleCols = {
			float3(0, 0, 1)
		};

		Render(points, triangleCols, image);

		std::string filename =
			"frame_" + std::to_string(currentFrame) + ".bmp";

		WriteImageToFile(image, filename);
	}
}


int main() {
	CreateTestImages();
}


