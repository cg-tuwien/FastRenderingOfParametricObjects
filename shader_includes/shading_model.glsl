

// +------------------------------------------------------------------------------+
// |        Shade Function														  |
// |																			  |
// | REQUIREMENTS BEFORE INCLUDING IT:											  |
// |  1)   #define SHADE_ADDITIONAL_PARAMS										  |
// |       e.g.: #define SHADE_ADDITIONAL_PARAMS , vec2 dx, vec2 dy 			  |
// |  2)   #define SAMPLE  														  |
// |       e.g.: #define SAMPLE textureGrad										  |
// |  3)   #define SAMPLE_ADDITIONAL_PARAMS										  |
// |       e.g.: #define SAMPLE_ADDITIONAL_PARAMS , dx, dy						  |
// +------------------------------------------------------------------------------+

#define M_INV_PI 0.318309886183790671537767526745


// Applies the non-linearity that maps linear RGB to sRGB
float linear_to_srgb(float linear) {
    return (linear <= 0.0031308) ? (12.92 * linear) : (1.055 * pow(linear, 1.0 / 2.4) - 0.055);
}

// Inverse of linear_to_srgb()
float srgb_to_linear(float non_linear) {
    return (non_linear <= 0.04045) ? ((1.0 / 12.92) * non_linear) : pow(non_linear * (1.0 / 1.055) + 0.055 / 1.055, 2.4);
}

// Turns a linear RGB color (i.e. rec. 709) into sRGB
vec3 linear_rgb_to_srgb(vec3 linear) {
    return vec3(linear_to_srgb(linear.r), linear_to_srgb(linear.g), linear_to_srgb(linear.b));
}

// Inverse of linear_rgb_to_srgb()
vec3 srgb_to_linear_rgb(vec3 srgb) {
    return vec3(srgb_to_linear(srgb.r), srgb_to_linear(srgb.g), srgb_to_linear(srgb.b));
}

// Logarithmic tonemapping operator. Input and output are linear RGB.
vec3 tonemap(vec3 linear) {
    float max_channel = max(max(1.0, linear.r), max(linear.g, linear.b));
    return linear * ((1.0 - 0.02 * log2(max_channel)) / max_channel);
}

// This is the glTF BRDF for dielectric materials, exactly as described here:
// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#appendix-b-brdf-implementation
// \param incoming The normalized incoming light direction.
// \param outgoing The normalized outgoing light direction.
// \param normal The normalized shading normal.
// \param roughness An artist friendly roughness value between 0 and 1
// \param base_color The albedo used for the Lambertian diffuse component
// \return The BRDF for the given directions.
vec3 gltf_dielectric_brdf(vec3 incoming, vec3 outgoing, vec3 normal, float roughness, vec3 base_color) {
    float ni = dot(normal, incoming);
    float no = dot(normal, outgoing);
    // Early out if incoming or outgoing direction are below the horizon
    if (ni <= 0.0 || no <= 0.0)
        return vec3(0.0);
    // Save some work by not actually computing the half-vector. If the half-
    // vector were h, ih = dot(incoming, h) and
    // sqrt(nh_ih_2 / ih_2) = dot(normal, h).
    float ih_2 = dot(incoming, outgoing) * 0.5 + 0.5;
    float sum = ni + no;
    float nh_ih_2 = 0.25 * sum * sum;
    float ih = sqrt(ih_2);

    // Evaluate the GGX normal distribution function
    float roughness_2 = roughness * roughness;
    float roughness_4  = roughness_2 * roughness_2;
    float roughness_flip = 1.0 - roughness_4;
    float denominator = ih_2 - nh_ih_2 * roughness_flip;
    float ggx = (roughness_4 * M_INV_PI * ih_2) / (denominator * denominator);
    // Evaluate the "visibility" (i.e. masking-shadowing times geometry terms)
    float vi = ni + sqrt(roughness_4 + roughness_flip * ni * ni);
    float vo = no + sqrt(roughness_4 + roughness_flip * no * no);
    float v = 1.0 / (vi * vo);
    // That completes the specular BRDF
    float specular = v * ggx;

    // The diffuse BRDF is Lambertian
    vec3 diffuse = M_INV_PI * base_color;

    // Evaluate the Fresnel term using the Fresnel-Schlick approximation
    const float ior = 1.5;
    const float f0 = ((1.0 - ior) / (1.0 + ior)) * ((1.0 - ior) / (1.0 + ior));
    float ih_flip = 1.0 - ih;
    float ih_flip_2 = ih_flip * ih_flip;
    float fresnel = f0 + (1.0 - f0) * ih_flip * ih_flip_2 * ih_flip_2;

    // Mix the two components
    return mix(diffuse, vec3(specular), fresnel);
}

vec3 shade(int matIndex, vec3 albedo, vec3 shadingUserParams, vec3 normalWS, vec2 texCoords SHADE_ADDITIONAL_PARAMS)
{
    if (matIndex < -1 || matIndex > 10000) {
		return linear_rgb_to_srgb(vec3(0.0, 0.0, 0.0));
    }

	//if (-1 == matIndex) {
 //       // Evaluate shading for a directional light
 //       vec3 color = vec3(1.0);
 //       vec3 base_color = srgb_to_linear_rgb(abs(normalize(positionWS)));
 //       const vec3 incoming = normalize(vec3(1.23, -4.56, 7.89));
 //       float ambient = 0.04;
 //       float exposure = 4.0;
 //       vec3 outgoing = -normalize((ubo.mViewMatrix * vec4(positionWS, 1.0)).xyz); // TODO: to camera direction
 //       vec3 normalVS = normalize(mat3(ubo.mViewMatrix) * normalWS);
 //       vec3 brdf = gltf_dielectric_brdf(incoming, outgoing, normalVS, 0.45, base_color);
 //       color = exposure * (brdf * max(0.0, dot(incoming, normalVS)) + base_color * ambient);
 //       return color;
 //   }

    if (-1 == matIndex) { // ========== DEBUG VISUALIZATION BEGIN ============
		float illu = max(0.2, dot(normalWS, normalize(vec3(1.0, 1.0, 1.0))));
		return linear_rgb_to_srgb(albedo * illu);
        //vec3 outgoing = normalize(normalWS);
        //vec3 base_color = srgb_to_linear_rgb(vec3(0.2, 0.5, 1.0));
        //const vec3 incoming = normalize(vec3(1.23, 7.89, 4.56));
        //float ambient = 0.04;
        //float exposure = 4.0;
        //vec3 brdf = gltf_dielectric_brdf(incoming, outgoing, normalWS, 0.45, base_color);
        //vec3 color = exposure * (brdf * max(0.0, dot(incoming, normalWS)) + base_color * ambient);
        //return linear_rgb_to_srgb(tonemap(color));

    } // ========== DEBUG VISUALIZATION END ============

    if (0 == matIndex) { // ========== CHECKERBOARD BEGIN ============
		vec3 checker = SAMPLE(textures[materialsBuffer.materials[matIndex].mDiffuseTexIndex], texCoords  SAMPLE_ADDITIONAL_PARAMS ).rgb;
		const vec3 lightDir = normalize(vec3(0.5, 1.0, -0.5));
		float nDotL = max(0.25, dot(normalWS, lightDir));
		return linear_rgb_to_srgb((checker * vec3(0.85) + vec3(0.15)) * nDotL);
	} // ========== CHECKERBOARD END ============

    if (1 == matIndex) { // ========== TERRAIN MAT BEGIN ============
		vec3 tex1 = SAMPLE(textures[materialsBuffer.materials[matIndex].mDiffuseTexIndex], texCoords       SAMPLE_ADDITIONAL_PARAMS ).rgb;
		vec3 tex2 = SAMPLE(textures[materialsBuffer.materials[matIndex].mAmbientTexIndex], texCoords * 0.3 SAMPLE_ADDITIONAL_PARAMS ).rgb;
		vec3 tex3 = SAMPLE(textures[materialsBuffer.materials[matIndex].mHeightTexIndex ], texCoords       SAMPLE_ADDITIONAL_PARAMS ).rgb;

		float vHeight = ubo.mDebugSliders[1] * shadingUserParams.y;
		vec3 terrainColor = mix(tex1, tex3, clamp(vHeight, 0.0, 1.0));
		terrainColor = mix(terrainColor, tex2, clamp(-vHeight * 0.5, 0.0, 1.0));

		const vec3 lightDir = normalize(vec3(0.5, 1.0, -0.5));
		float nDotL = max(0.25, dot(normalWS, lightDir));

		return linear_rgb_to_srgb(terrainColor * nDotL);
	} // ========== TERRAIN MAT END ============

    if (2 == matIndex) { // ========== SEASHELL MAT BEGIN ============
		//texCoords *= vec2(0.555, 0.222);
		//vec3 tex1 = SAMPLE(textures[materialsBuffer.materials[matIndex].mDiffuseTexIndex], texCoords       SAMPLE_ADDITIONAL_PARAMS ).rgb;
		//vec3 tex2 = SAMPLE(textures[materialsBuffer.materials[matIndex].mHeightTexIndex ], texCoords       SAMPLE_ADDITIONAL_PARAMS ).rgb;

		//float vHeight = ubo.mDebugSliders[1] * shadingUserParams.y;
		//vec3 seashellColor = mix(tex1, tex2, clamp(vHeight, 0.0, 1.0));
        vec3 seashellColor = vec3(0.7);

		const vec3 lightDir = normalize(vec3(0.5, 1.0, -0.5));
		float nDotL = max(0.25, dot(normalWS, lightDir));

		return linear_rgb_to_srgb(seashellColor * nDotL);
	} // ========== SEASHELL MAT END ============

	{ // ========== STANDARD MAT BEGIN ============
		vec2 diffTexTiling  = materialsBuffer.materials[matIndex].mDiffuseTexOffsetTiling.zw;
		vec2 diffTexOffset  = materialsBuffer.materials[matIndex].mDiffuseTexOffsetTiling.xy;
		vec2 diffTexCoords  = texCoords * diffTexTiling + diffTexOffset;
		vec3 diffuseTex     = SAMPLE(textures[materialsBuffer.materials[matIndex].mDiffuseTexIndex], diffTexCoords  SAMPLE_ADDITIONAL_PARAMS ).rgb;
		
		vec3 color = diffuseTex;

		float ambient = 0.1;
		vec3 diffuse  = materialsBuffer.materials[matIndex].mDiffuseReflectivity.rgb;
		vec3 toLight  = normalize(vec3(1.0, 1.0, 0.5));
		vec3 illum    = vec3(ambient) + diffuse * max(0.0, dot(normalize(normalWS), toLight));
		color *= illum;

		return linear_rgb_to_srgb(color);
    } // ========== STANDARD MAT END ============
}
