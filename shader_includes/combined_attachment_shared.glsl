
// ###### HELPER FUNCTIONS ###############################

// Converts sRGB color values into linear space:
// Apply this after reading colors from textures.
float sRGB_to_linear(float srgbColor)
{
    if (srgbColor <= 0.04045) {
		return srgbColor / 12.92;
	}
    return pow((srgbColor + 0.055) / (1.055), 2.4);
}

// Converts sRGB color values into linear space:
// Apply this after reading colors from textures.
vec3 sRGB_to_linear(vec3 srgbColor)
{
	vec3 linear;
	linear.r = sRGB_to_linear(srgbColor.r);
	linear.g = sRGB_to_linear(srgbColor.g);
	linear.b = sRGB_to_linear(srgbColor.b);
	return linear;
}

// Converts color values from linear space into sRGB:
// Apply this before storing colors in textures.
float linear_to_sRGB(float linearColor)
{
	if (linearColor <= 0.0031308) {
		return 12.92 * linearColor;
	}
	float a = 0.055;
	return (1 + a) * pow(linearColor, 1 / 2.4) - a;
}

// Converts color values from linear space into sRGB:
// Apply this before storing colors in textures.
vec3 linear_to_sRGB(vec3 linearColor)
{
	vec3 srgb;
	srgb.r = linear_to_sRGB(linearColor.r);
	srgb.g = linear_to_sRGB(linearColor.g);
	srgb.b = linear_to_sRGB(linearColor.b);
	return srgb;
}

// +------------------------------------------------------------------------------+
// |                                                                              |
// |   Dependencies:                                                              |
// |    - ui64_conv.glsl included                                                 |
// |    - GL_EXT_shader_image_int64                                               |
// |    - A bound resource of the following type and name:                        |
// |          uniform u64image2D uCombinedAttachment;                             |
// |                                                                              |
// +------------------------------------------------------------------------------+

uint64_t getUint64ClearDepth()
{
	float clearDepth = 1.0;
	uint64_t depthEncoded = depth_to_ui64(clearDepth);
	return depthEncoded;
}

uint64_t getUintClearColor()
{
	vec4  clearColor = vec4(0.0);
	uint64_t colorEncoded = color_to_ui64(clearColor);
	return colorEncoded;
}

uint64_t getUint64ClearValue()
{
	uint64_t clearValue = combine_depth_and_color(getUint64ClearDepth(), getUintClearColor());
	return clearValue;
}

bool isClearDepth(uint64_t valueInQuestion)
{
	uint64_t depthEncoded, colorEncoded;
	extract_depth_and_color_encoded(valueInQuestion, depthEncoded, colorEncoded);
	return depthEncoded == getUint64ClearDepth();
}

bool isClearValue(uint64_t valueInQuestion)
{
	return valueInQuestion == getUint64ClearValue();
}

uint64_t getUint64ForFramebuffer(float depth, vec4 color)
{
	uint64_t depthEncoded = depth_to_ui64(depth);
	uint64_t colorEncoded = color_to_ui64(color);
	uint64_t data = combine_depth_and_color(depthEncoded, colorEncoded);
	return data;
}

uint64_t getUint64ForFramebuffer(float depth, vec3 color)
{
	return getUint64ForFramebuffer(depth, vec4(color, 1.0));
}

void writeToCombinedAttachment(ivec2 screenCoords, uint64_t data)
{
	uvec2 imSize = imageSize(uCombinedAttachment);
	if (screenCoords.x >= imSize.x || screenCoords.y >= imSize.y) {
		return;
	}

#if STATS_ENABLED
    if (ubo.mGatherPipelineStats) {
		uint64_t origValue = imageAtomicMin(uCombinedAttachment, screenCoords, data);
		if (isClearDepth(origValue)) {
			atomicAdd(uCounters.mCounters[1], 1);
		}
		atomicAdd(uCounters.mCounters[2], 1);
    }
    else {
		imageAtomicMin(uCombinedAttachment, screenCoords, data);
	}

    if (ubo.mHeatMapEnabled) {
        imageAtomicAdd(uHeatmapImage, screenCoords, 1);
    }
#else
	imageAtomicMin(uCombinedAttachment, screenCoords, data);
#endif
}

void writeToCombinedAttachment(ivec2 screenCoords, float depth, vec4 color)
{
	writeToCombinedAttachment(screenCoords, getUint64ForFramebuffer(depth, color));
}

void writeToCombinedAttachment(ivec2 screenCoords, float depth, vec3 color)
{
	writeToCombinedAttachment(screenCoords, depth, vec4(color, 1.0));
}

#if SUPERSAMPLING
// Source: https://stackoverflow.com/questions/43075153/atomic-moving-average-on-image
// 
// writeSupersampled means:
//  - take min of the depth
//  - average the colors
void writeSupersampled(ivec2 screenCoords, float depth, vec4 color)
{
    uint64_t newVal = getUint64ForFramebuffer(depth, color);
    uint64_t prevStoredVal = 0; // First imageAtomicCompSwap will fail for sure, but that's okay
    uint64_t curStoredVal;

    // Loop as long as destination value gets changed by other threads:
    while((curStoredVal = imageAtomicCompSwap(uCombinedAttachment, screenCoords, prevStoredVal, newVal))
          != prevStoredVal)
    {
        prevStoredVal = curStoredVal;

		uint64_t depthEncoded, colorEncoded;
		extract_depth_and_color(curStoredVal, depthEncoded, colorEncoded);
		float storedDepth = ui64_to_depth(depthEncoded);
		vec4  storedColor = ui64_to_color(colorEncoded);

		vec4 tmpStored = ubo.mInverseProjMatrix * vec4(vec2(screenCoords)/vec2(ubo.mResolutionAndCulling.xy), storedDepth, 1.0);
		float linearStoredDepth = tmpStored.z / tmpStored.w;
		vec4 tmpIncoming = ubo.mInverseProjMatrix * vec4(vec2(screenCoords)/vec2(ubo.mResolutionAndCulling.xy), depth, 1.0);
		float linearIncomingDepth = tmpIncoming.z / tmpIncoming.w;

		const float epsilon = 0.01;
        
		// 2 cases of not to be averaged:
		if (linearIncomingDepth < linearStoredDepth - epsilon) {
			break;
        }
		if (linearIncomingDepth > linearStoredDepth + epsilon) {
			continue;
        }

		// To be averaged => Calc min/average:
		float minDepth = min(storedDepth, depth);
		vec4  avgColor = mix(storedColor, color, 0.5);
		newVal = getUint64ForFramebuffer(minDepth, avgColor);
    }
}
#endif
