#include <cstdio>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>

struct float3 {
	float x, y, z;
	float r, g, b;

	float3() = default;

	float3(float x, float y, float z)
		: x(x), y(y), z(z), r(x), g(y), b(z) {}
};

static std::string GetFilePath(const std::string& fileName);
static void WriteImageToFile(const std::vector<std::vector<float3>>& image, const std::string& name);

void CreateTestImage();

int main() {
	CreateTestImage();
}

void CreateTestImage() {
	const int width = 64;
	const int height = 64;
	std::vector<std::vector<float3>> image(width, std::vector<float3>(height));

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float r = (float)x / (width - 1);
			float g = (float)y / (height - 1);
			image[x][y] = float3(r, g, 0);
		}
	}

	WriteImageToFile(image, "art");
}

static std::string GetFilePath(const std::string& fileName) {
	std::filesystem::path outputDir = "Output";

	if (!std::filesystem::exists(outputDir)) {
		std::filesystem::create_directory(outputDir);
	}

	return (outputDir / fileName).string();
}
static void WriteImageToFile(const std::vector<std::vector<float3>>& image, const std::string& name) {
	std::ofstream writer(GetFilePath(name + ".txt"), std::ios::binary);

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
