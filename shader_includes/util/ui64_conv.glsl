uint64_t color_channel_to_ui64(float color)
{
    return uint64_t(color * 255.0);
}

uint64_t color_to_ui64(vec4 color)
{
    uint64_t R = 0xFF & uint64_t(255.0 * color.x);
    uint64_t G = 0xFF & uint64_t(255.0 * color.y);
    uint64_t B = 0xFF & uint64_t(255.0 * color.z);
    uint64_t A = 0xFF & uint64_t(255.0 * color.a);
    return (R << 24) | (G << 16) | (B << 8) | A;
}

uint64_t depth_to_ui64(float depth)
{
    return uint64_t(floatBitsToInt(depth));
}

float ui64_to_color_channel(uint64_t colorChannelEncoded)
{
    return float(colorChannelEncoded) / 255.0;
}

vec4 ui64_to_color(uint64_t colorEncoded)
{
    return vec4(ui64_to_color_channel(0xFF & (colorEncoded >> 24)), ui64_to_color_channel(0xFF & (colorEncoded >> 16)), ui64_to_color_channel(0xFF & (colorEncoded >> 8)),
        ui64_to_color_channel(0xFF & (colorEncoded)));
}

float ui64_to_depth(uint64_t depthEncoded)
{
    return intBitsToFloat(int(depthEncoded));
}

uint64_t combine_depth_and_color(uint64_t depthEncoded, uint64_t colorEncoded)
{
    return (depthEncoded << 32) | colorEncoded;
}

void extract_depth_and_color_encoded(in uint64_t inp, out uint64_t depthEncoded, out uint64_t colorEncoded)
{
    depthEncoded = 0xFFFFFFFF & (inp >> 32);
    colorEncoded = 0xFFFFFFFF & inp;
}

void extract_depth_and_color(in uint64_t inp, out float depth, out vec4 color)
{
    uint64_t depthEncoded;
    uint64_t colorEncoded;
    extract_depth_and_color_encoded(inp, depthEncoded, colorEncoded);
    depth = ui64_to_depth(depthEncoded);
    color = ui64_to_color(colorEncoded);
}
