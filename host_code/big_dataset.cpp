#include "big_dataset.hpp"
#include <regex>
#include <random>
#include <filesystem>

std::vector<std::array<float, 92>> get_big_sh_dataset(int dataSizeX, int dataSizeY) // 92 is divisible by 4 and provides enough space for all them coefficients
{
    std::ifstream ifs("assets/wmfod.mif", std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) throw std::runtime_error("Could not open file");
    const auto fileSize = std::filesystem::file_size("assets/wmfod.mif");
    ifs.seekg(0);

	// load header and payload
    size_t headerSize = 4160;
    std::string header(headerSize, '\0');
    std::vector<char> payload(fileSize - headerSize, 0);
    ifs.read(&header[0], headerSize);
    if(header.substr(headerSize - 5, 3) != "END") throw std::runtime_error("Header wrong");
    ifs.read(&payload[0], fileSize - headerSize);
    ifs.close();

    if(payload.size() != 324576008) throw std::runtime_error("Payload size wrong");

    std::regex dim_regex("dim: ([0-9]+),([0-9]+),([0-9]+),([0-9]+)");
    std::smatch dim_match;
    std::regex_search(header, dim_match, dim_regex);
    std::vector<int> dims{ std::stoi(dim_match[1]), std::stoi(dim_match[2]), std::stoi(dim_match[3]), std::stoi(dim_match[4]) };

    std::regex layout_regex("layout: ([+\\-0-9]+),([+\\-0-9]+),([+\\-0-9]+),([+\\-0-9]+)");
    std::smatch layout_match;
    std::regex_search(header, layout_match, layout_regex);
    std::vector<int> layout{ std::abs(std::stoi(layout_match[1])), std::abs(std::stoi(layout_match[2])), std::abs(std::stoi(layout_match[3])), std::abs(std::stoi(layout_match[4])) };

    std::vector<float> linear_array(reinterpret_cast<float*>(payload.data()), reinterpret_cast<float*>(payload.data() + sizeof(float) * std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<int>())));
    for (float& val : linear_array) {
        val *= 35.0;
    }
    if(linear_array.size() != 81144000) throw std::runtime_error("Linear array size wrong");
    if(layout != std::vector<int>{1, 2, 3, 0}) throw std::runtime_error("Layout wrong");

    std::vector<std::array<float, 92>> dataset;
    int z = dims[2] / 2;
    for (int x = 0; x < dims[0] && x < dataSizeX; ++x) {
        for (int y = 0; y < dims[1] && y < dataSizeY; ++y) {
            std::vector<int> strides{ dims[3], dims[3] * dims[0], dims[3] * dims[0] * dims[1], 1 };
            size_t offset = x * strides[0] + y * strides[1] + z * strides[2];
            if (strides[3] != 1) throw std::runtime_error("Stride not 1");
            std::vector<float> glyph(linear_array.begin() + offset, linear_array.begin() + offset + strides[3] * dims[3]);
          
            std::mt19937 rng(239784);
            std::normal_distribution<float> dist(0.0, 0.07);

            constexpr size_t band_10_glyphs = 2 * 10 + 1;
            constexpr size_t band_12_glyphs = 2 * 12 + 1;
            std::vector<float> band_10(band_10_glyphs);
            std::vector<float> band_12(band_12_glyphs);
            for (size_t i = 0; i < band_10.size(); ++i) band_10[i] = dist(rng);
            for (size_t i = 0; i < band_12.size(); ++i) band_12[i] = dist(rng);

            glyph.insert(glyph.end(), band_10.begin(), band_10.end());
            glyph.insert(glyph.end(), band_12.begin(), band_12.end());

            std::array<float, 92> sh_coeffs;
            for (size_t i = 0; i < 92-1; ++i) sh_coeffs[i] = glyph[i];
            dataset.emplace_back(sh_coeffs);
        }
    }
    return dataset;
}