
// +------------------------------------------------------------------------------+
// |   Parametric Curve Helpers:                                                  |
// +------------------------------------------------------------------------------+

// Gets texture coordinates per parametric curve
vec2 getParamTexCoords(float u, float v, int curveIndex, uvec3 userData, vec3 posWS) {
    vec2 texCoords = vec2(u, v);
    switch (curveIndex) {
            // +--------------------+
            // | Yarn Curves        |
            // +--------------------+
	    case 14: 
	    case 15: 
	    case 16: 
	    case 17: 
            texCoords = vec2(3.55 - posWS.x / 7.38, posWS.y / 7.4);
            break;
    }
    return texCoords;
}

// Gets user parameters for shading
vec3 getParamShadingUserParams(float u, float v, int curveIndex, uvec3 userData, vec3 posWS) {
    vec3 shadingUserParams = vec3(0.0);
    switch (curveIndex) {
        case 4:
            // +-------------------------------------------------+
            // |   Terrain:                                      |
            // +-------------------------------------------------+
            shadingUserParams = posWS;
            break;
        case 18:
        case 19:
        case 20:
    
            shadingUserParams.y = smoothstep(TWO_PI, TWO_PI + TWO_PI, u);

            // TODO: 
            //// Add nodule along the ellipse:
            //float eper   = (TWO_PI / numNodE);
            //float eshift = theta - nodOffsetE;
            //float ern    = eshift / eper - round(eshift / eper);
            //float enh    = exp(-pow(nodWidthE / numNodE, spikynessE) * ern * ern) * nodHeightE / 20.f;

            //// Add nodule along the spiral:
            //float sper   = (TWO_PI / numNodS);
            //float sshift = ellipsePos - nodOffsetS;
            //float srn    = sshift / sper - round(sshift / sper);
            //float snh    = exp(-pow(nodWidthS / numNodS, spikynessS) * srn * srn) * nodHeightS / 20.f;
            break;
    }
    return shadingUserParams;
}


// Calculates the normal at posWS, based on the position of two of its 
// neighbor, namely:
//  - neighborUPosWS ... Position that results from evaluating the parametric curve
//                       direction of positively increasing u parameter.
//  - neighborVPosWS ... Position that results from evaluating the parametric curve
//                       direction of positively increasing v parameter.
vec3 calculateNormalWS(vec3 posWS, vec3 neighborUPosWS, vec3 neighborVPosWS, int curveIndex, uvec3 userData) {
    vec3 n = cross(posWS - neighborUPosWS, neighborVPosWS - posWS);
    n = normalize(n);
    return n;
}

// +------------------------------------------------------------------------------+
// |   Functions:                                                                 |
// +------------------------------------------------------------------------------+

float sinc(float x) /* Supporting sinc function */
{
    if (abs(x) < 1.0e-4)
        return 1.0;
    else
        return (sin(x) / x);
}

float factorial(int a_number)
{
    switch(a_number) {
        case  0: return 1.0;
        case  1: return 1.0;
        case  2: return 2.0;
        case  3: return 6.0;
        case  4: return 24.0;
        case  5: return 120.0;
        case  6: return 720.0;
        case  7: return 5040.0;
        case  8: return 40320.0;
        case  9: return 362880.0;
        case 10: return 3628800.0;
        case 11: return 39916800.0;
        case 12: return 479001600.0;
        case 13: return 6227020800.0;
        case 14: return 87178291200.0;
        case 15: return 1307674368000.0;
        case 16: return 20922789888000.0;
        case 17: return 355687428096000.0;
        case 18: return 6402373705728000.0;
        case 19: return 121645100408832000.0;
        case 20: return 2432902008176640000.0;
        case 21: return 51090942171709440000.0;
        case 22: return 1124000727777607680000.0;
        case 23: return 25852016738884976640000.0;
        case 24: return 620448401733239439360000.0;
        case 25: return 15511210043330985984000000.0;
        case 26: return 403291461126605635584000000.0;
        case 27: return 10888869450418352160768000000.0;
        case 28: return 304888344611713860501504000000.0;
        case 29: return 8841761993739701954543616000000.0;
        case 30: return 265252859812191058636308480000000.0;
        case 31: return 8222838654177922817725562880000000.0;
        case 32: return 263130836933693530167218012160000000.0;
        case 33: return 8683317618811886495518194401280000000.0;
    }
    return 0.0;
}

float P(float l, float m, float x)
{
	// evaluate an Associated Legendre Polynomial P(l,m,x) at x
	float pmm = 1.0;

	if (m > 0.0) {
		float somx2 = sqrt((1.0 - x) * (1.0 + x));
		float fact = 1.0;

		for (float i = 1.0; i <= m; i = i + 1.0) {
			pmm *= (-fact) * somx2;
			fact += 2.0;
		}
	}

	if (l == m) {
		return pmm;
	}

	float pmmp1 = x * (2.0 * m + 1.0) * pmm;

	if (l == m + 1.0) {
		return pmmp1;
	}

	float pll = 0.0;
	for (float ll = m + 2.0; ll <= l; ll = ll + 1.0) {
		pll = ((2.0 * ll - 1.0) * x * pmmp1 - (ll + m - 1.0) * pmm) / (ll - m);
		pmm = pmmp1;
		pmmp1 = pll;
	}

	return pll;
}

float K(float l, float m)
{
	// renormalisation constant for SH function
	float temp = ((2.0 * l + 1.0) * factorial(int(l - m))) / (4.0 * PI * factorial(int(l + m)));

	return sqrt(temp);
}

float SH(float l, float m, float theta, float phi)
{
	// return a point sample of a Spherical Harmonic basis function
	// l is the band, range [0..N]
	// m in the range [-l..l]
	// theta in the range [0..Pi]
	// phi in the range [0..2*Pi]
	const float sqrt2 = 1.41421356237f;

	if (m == 0.0) {
		return K(l, 0.0) * P(l, m, cos(theta));
		// return 1.0;
	}
	else {
		if (m > 0.0) {
			return sqrt2 * K(l, m) * cos(m * phi) * P(l, m, cos(theta));
		}
		else {
			return sqrt2 * K(l, -m) * sin(-m * phi) * P(l, -m, cos(theta));
		}
	}
}

float HomoSH(int l, int m, float theta, float phi)
{
    vec3 pos = to_sphere(theta, phi);
    float x = pos.x;
    float y = pos.y;
    float z = pos.z;

    switch(l) {
        case 0:
            switch (m) {
                case 0: return 1.0 / (2.0 * SQRT_PI);
            }
            break;
        case 2:
            switch (m) {
                case -2: return sqrt(15.0 / (16.0 * PI)) * (x*x - y*y);
                case -1: return sqrt(15.0 / ( 4.0 * PI)) * x * z;
                case  0: return sqrt( 5.0 / (16.0 * PI)) * (3.0 * z*z);
                case  1: return sqrt(15.0 / ( 4.0 * PI)) * y * z;
                case  2: return sqrt(15.0 / ( 4.0 * PI)) * x * y;
            }
            break;
        case 4:
            switch (m) {
                case -4: return  3.0 / 16.0 * sqrt(35.0 / PI) * (x*x*x*x - 6.0*x*x*y*y + y*y*y*y);
                case -3: return  3.0 /  8.0 * sqrt(70.0 / PI) * x*(x*x - 3.0*y*y)*z;
                case -2: return  3.0 /  8.0 * sqrt( 5.0 / PI) * (x*x - y*y) * (7.0*z*z);
                case -1: return  3.0 /  8.0 * sqrt(10.0 / PI) * x*z*(7.0*z*z - 3.0);
                case  0: return  3.0 / (16.0 * SQRT_PI) * (35.0*z*z*z*z - 30.0*z*z + 3.0);
                case  1: return -3.0 / 8.0 * sqrt(10.0 / PI) * y*z*(7.0*z*z - 3.0);
                case  2: return  3.0 / 4.0 * sqrt( 5.0 / PI) * x*y*(7.0*z*z);
                case  3: return  3.0 / 8.0 * sqrt(70.0 / PI) * (y*y - 3.0*x*x)*y*z;
                case  4: return  3.0 / 4.0 * sqrt(35.0 / PI) * x*y*(x*x - y*y);
            }
            break;
    }
}

// The index of the highest used band of the spherical harmonics basis. Must be
// even, at least 2 and at most 12.
#define SH_DEGREE 12
// The number of spherical harmonics basis functions
#define SH_COUNT (((SH_DEGREE + 1) * (SH_DEGREE + 2)) / 2)
float SH_COEFFS[SH_COUNT] = {
	  -0.2739740312099,  0.2526670396328,  1.8922271728516,  0.2878578901291, -0.5339795947075, -0.2620058953762
#if SH_DEGREE >= 4
	,  0.1580424904823,  0.0329004973173, -0.1322413831949, -0.1332057565451,  1.0894461870193, -0.6319401264191, -0.0416776277125, -1.0772529840469,  0.1423762738705
#endif
#if SH_DEGREE >= 6
	,  0.7941166162491,  0.7490307092667, -0.3428381681442,  0.1024847552180, -0.0219132602215,  0.0499043911695,  0.2162453681231,  0.0921059995890, -0.2611238956451,  0.2549301385880, -0.4534865319729,  0.1922748684883, -0.6200597286224
#endif
#if SH_DEGREE >= 8
	, -0.0532187558711, -0.3569841980934,  0.0293972902000, -0.1977960765362, -0.1058669015765,  0.2372217923403, -0.1856198310852, -0.3373193442822, -0.0750469490886,  0.2146576642990, -0.0490148440003,  0.1288588196039,  0.3173974752426,  0.1990085393190, -0.1736343950033, -0.0482443645597, 0.1749017387629
#endif
#if SH_DEGREE >= 10
	, -0.0151847425660,  0.0418366046081,  0.0863263587216, -0.0649211244490,  0.0126096132283,  0.0545089217982, -0.0275142164626,  0.0399986574832, -0.0468244261610, -0.1292105653111, -0.0786858322658, -0.0663828464882,  0.0382439706831, -0.0041550330365, -0.0502800566338, -0.0732471630735, 0.0181751900972, -0.0090119333757, -0.0604443282359, -0.1469985252752, -0.0534046899715
#endif
#if SH_DEGREE >= 12
	, -0.0896672753415, -0.0130841364808, -0.0112942893801,  0.0272257498541,  0.0626717616331, -0.0222197983050, -0.0018541504308, -0.1653251944056,  0.0409697402846,  0.0749921454327, -0.0282830872616,  0.0006909458525,  0.0625599842287,  0.0812529816082,  0.0914693020772, -0.1197222726745, 0.0376277453183, -0.0832617004142, -0.0482175038043, -0.0839003635737, -0.0349423908400, 0.1204519568256, 0.0783745984003, 0.0297401205976, -0.0505947662525
#endif
};


void eval_sh_4(out float out_shs[15], vec3 point) {
    float x, y, z, z2, c0, s0, c1, s1, d, a, b;
    x = point[0];
    y = point[1];
    z = point[2];
    z2 = z * z;
    d = 0.282094792;
    out_shs[0] = d;
    a = z2 - 0.333333333;
    d = 0.946174696 * a;
    out_shs[3] = d;
    b = z2 * (a - 0.266666667);
    a = b - 0.257142857 * a;
    d = 3.702494142 * a;
    out_shs[10] = d;
    c1 = x;
    s1 = y;
    d = -1.092548431 * z;
    out_shs[2] = -c1 * d;
    out_shs[4] = s1 * d;
    a = (z2 - 0.2) * z;
    b = a - 0.228571429 * z;
    d = -4.683325805 * b;
    out_shs[9] = -c1 * d;
    out_shs[11] = s1 * d;
    c0 = x * c1 - y * s1;
    s0 = y * c1 + x * s1;
    d = 0.546274215;
    out_shs[1] = c0 * d;
    out_shs[5] = s0 * d;
    a = z2 - 0.142857143;
    d = 3.311611435 * a;
    out_shs[8] = c0 * d;
    out_shs[12] = s0 * d;
    c1 = x * c0 - y * s0;
    s1 = y * c0 + x * s0;
    d = -1.77013077 * z;
    out_shs[7] = -c1 * d;
    out_shs[13] = s1 * d;
    c0 = x * c1 - y * s1;
    s0 = y * c1 + x * s1;
    d = 0.625835735;
    out_shs[6] = c0 * d;
    out_shs[14] = s0 * d;
}

void getSpiralPosition(float theta, float A, float alphaRad, float betaRad, float scale, out vec3 pos, out float radius)
{
    radius = A * exp(theta * (1.0 / tan(alphaRad)));
	pos = vec3(
	    /* x: */ radius * sin(theta),
	    /* y: */ radius * cos(theta),
		/* z: */ radius);
	pos.x *= sin(betaRad) * scale;
	pos.y *= sin(betaRad) * scale;
	pos.z *= -cos(betaRad) * scale;
};

// gets position on the surface of a sheashell:
// u, v parrameters =^= ellipsePos, theta
// @param alphaDeg      angle of tangent in degrees
// @param betaDeg       enlarging angle in z in degrees
// @param a             radius of ellipse at 0 deg
// @param b             radius of ellipse at 90 deg
// @param muDeg         rotation of ellipse along side-to-side axis relative to face in degrees
// @param nodWidthE     nodule width relative to opening
// @param numNodE       number of nodules along ellipse direction
// @param numNodS       number of nodules along spiral direction
// @param nodHeightF    nodule height scale factor
// @param spikynessE    how spiky the nodules shall be along ellipse direction
// @param spikynessS    how spiky the nodules shall be along spiral direction
vec3 getSeashellPos(float ellipsePos, float theta, float alphaDeg, float betaDeg, float a, float b, float muDeg, float nodWidthE, float numNodE, float numNodS, float nodHeightF, float spikynessE, float spikynessS)
{
    float scale = 0.01;

    // spiral parameters
    float A        = 25.0;                 // distance from origin at theta = 0
    float coils    = 8.0;                  // number coils
    float alphaRad = alphaDeg * (PI / 180.0);  // angle of tangent (offset from 90 degrees)
    float betaRad  = betaDeg * (PI / 180.0);  // enlarging angle in z

    // ellipse opening parameters
    float phi     =  60.674 * (PI / 180.0);    // rotation of ellipse along z-axis (front-to-back axis relative to face)
    float omega   =   7.584 * (PI / 180.0);    // rotation of ellipse along y-axis (top-to-bottom axis relative to face)
    float mu      = muDeg * (PI / 180.0);      // rotation of ellipse along x-axis (side-to-side axis relative to face)

    // nodule parameters
    //    ellipse:
    float nodHeightE = 20.0 * nodHeightF;
    float nodOffsetE = 52.331 * (PI / 180.0);
    //    spiral:
    float nodHeightS =   5.674 * nodHeightF;
    float nodOffsetS =  40.65 * (PI / 180.f);
    float nodWidthS  = 125.4;

    // (float theta, float A, float alphaRad, float betaRad, float scale)
    vec3 spiralPos;
    float spiralRadius;
    getSpiralPosition(ellipsePos, A, alphaRad, betaRad, scale, spiralPos, spiralRadius);

    // float re  = pow(pow2(cos(theta) / a) + pow2(sin(theta) / b), -0.5f);
    vec3 pos = vec3(a * sin(theta), b * cos(theta), 0.0);
    // Position correctly for initial position:
    swap(pos.x, pos.z);
    // Tilt:
    pos = rotate_x(pos, -phi);
    pos = rotate_y(pos, -mu);
    pos = rotate_z(pos, -omega);
    // Rotate according to spiral position:
    pos = rotate_z(pos, -ellipsePos);

    // Add nodule along the ellipse:
    float eper   = (TWO_PI / numNodE);
    float eshift = theta - nodOffsetE;
    float ern    = eshift / eper - round(eshift / eper);
    float enh    = exp(-pow(nodWidthE / numNodE, spikynessE) * ern * ern) * nodHeightE / 20.f;

    // Add nodule along the spiral:
    float sper   = (TWO_PI / numNodS);
    float sshift = ellipsePos - nodOffsetS;
    float srn    = sshift / sper - round(sshift / sper);
    float snh    = exp(-pow(nodWidthS / numNodS, spikynessS) * srn * srn) * nodHeightS / 20.f;

    pos *= spiralRadius / A * (scale + scale * enh * snh);
    // Translate the ellipse according to the spiral position:
    pos += spiralPos;
    
    swap(pos.y, pos.z);
    return pos;
}

vec4 paramToWS(float u, float v, int curveIndex, uvec3 userData)
{
    if (ubo.mGatherPipelineStats) {
        atomicAdd(uCounters.mCounters[0], 1);
    }

    vec3 object;
    switch (curveIndex) {
        case 0:
            // +-------------------------------------------------+
            // |   Plane:                                        |
            // +-------------------------------------------------+
            object = to_plane(u, v);
            swap(object.y, object.z); // corresponds to rotate_x(plane, HALF_PI);
            break;
        case 1:
            // +-------------------------------------------------+
            // |   Sphere:                                       |
            // +-------------------------------------------------+
            object = to_sphere(u, v);
            break;
        case 2:
            // +-------------------------------------------------+
            // |   Palm Tree Trunk:                              |
            // +-------------------------------------------------+
            {
                const float thickness = 0.5;
                const float numSawtoothElements = 15.0;

                const int numControlPoints = 5;
	            vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS];
                controlPoints[0] = vec3( 0.0, 0.0  - 5.0,   0.0);
                controlPoints[1] = vec3( 0.0, 1.0  - 5.0,   0.0);
                controlPoints[2] = vec3( 2.0, 4.0  - 5.0,   1.0);
                controlPoints[3] = vec3( 0.0, 8.0  - 5.0,   0.0);
                controlPoints[4] = vec3(-1.0, 10.0 - 5.0, -1.0);

                vec3 pos     = bezier_value_at(numControlPoints, controlPoints, u);
                vec3 upwards = normalize(bezier_slope_at(numControlPoints, controlPoints, u));
			
                // Manually make a pipe (without using the pipeify function, but same steps):
                vec3 nTrunk = orthogonal(upwards);

	    	    // Generate palm tree trunk shape:
                float sawtooth = (u * numSawtoothElements) - floor(u * numSawtoothElements);
                float sawtoothscale = sawtooth * 0.3 + 1.0;

                // Let the tree trunk be a bit thicker at the bottom:
                float thicknessscale = 1.0 / (u + 1.0) + 0.5;

                vec3 treeTrunkPos = pos + rotate_around_axis(nTrunk * thickness * sawtoothscale * thicknessscale, upwards, v);
                object = treeTrunkPos;
            }
            break;
        case 3:
            // +-------------------------------------------------+
            // |   Water:                                        |
            // +-------------------------------------------------+
            {
                float  ut = -PI * 3.5 + u * PI * 2;
		        float  vt = -PI * 2 + v * PI * 1.5;
                object = vec3(
                    5.0f * (u - 0.5f),
		            1.0f + sin(ut) * 0.5f + sin(vt) * 0.13f + sin(vt * 0.3 + ut * 0.2) + cos(ut) * 0.05f,
		            5.0f * (v - 0.5f)
                );
            }
            break;
        case 4:
            // +-------------------------------------------------+
            // |   Terrain:                                      |
            // +-------------------------------------------------+
            {
                float f = 0.1; // frequency
                float hillHeight = 5.0;
                float x = u;
                float z = v;
                float a = 2.0 * hillHeight * cnoise(vec2(x, z) * f / 2.0);
                float b = hillHeight * cnoise(vec2(z, x) * f);
                float c = 0.5 * hillHeight * cnoise(vec2(x, z) * f * 2.0);
                object = vec3(x, a + b + c, z);
            }
            break;
        case 5:
            // +-------------------------------------------------+
            // |   SH Basis Function:                            |
            // +-------------------------------------------------+
            {
                //float f = 0.0;
                //int lmax = 5;
                //for (int l = 0; l <= lmax; ++l) {
                //    for (int m = -l; m <= l; ++m) {
                //        f += SH(l, m, u, v);
                //    }
                //}
                float f = SH(int(ubo.mDebugSlidersi[0]), int(ubo.mDebugSlidersi[1]), u, v);
                if (isnan(f)) debugPrintfEXT("f is nan");
                f = abs(f);
                f = mix(f, 1.0, ubo.mDebugSliders[0]);

                object = to_sphere(u, v, /* radius: */ f);
                // ^ TODO: Ask David why I can't do this:
                //         1) object = to_sphere(u, v);
                //         2) object *= f;
                // => Skaling is wrong. Why?
            }
            break;
        case 6:
            // +-------------------------------------------------+
            // |   Funky Plane:                                  |
            // |                                                 |
            // |   Expected input uv ranges:                     |
            // |   u = [0, 1)                                    |
            // |   v = [0, 1)                                    |
            // +-------------------------------------------------+
            {
                float s = u;
                float t = v;
                const float scale = 10.0;
                const float height = 0.105;

                float time = ubo.mAbsoluteTime;
                //float time = 123.0;
                float su = s - 0.5;
                float tu = t - 0.5;
                float d = (su * su + tu * tu);

                // NOTE: It's very important for perf to explicitly specify float literals (e.g. 2.0f)
                float z = height * sin(scale * s + time) * cos(scale * t + time) 
                            + cos(2.0 * time) * 10.0 * height * exp(-1000.0 * d);

                object = vec3(
                    2.0 * (-s + 0.5), 
                    z,
                    2.0 * (-t + 0.5)
                );
            }
            break;
        case 7:
            // +-------------------------------------------------+
            // |   Extra Funky Plane:                            |
            // +-------------------------------------------------+
            {
                float s = u;
                float t = v;
                const float scale = 10.0;
                const float height = 0.105;

                float time = ubo.mAbsoluteTime;
                //float time = 123.0;
                float su = s - 0.5;
                float tu = t - 0.5;
                float d = (su * su + tu * tu);

                // NOTE: It's very important for perf to explicitly specify float literals (e.g. 2.0f)
                float z = height * sin(scale * s + time) * cos(scale * t + time) 
	                + cos(2.0 * time) * 10.0 * height * exp(-1000.0 * d)
	                + 0.002 * sin(2.0 * PI * 300.0 * s) * cos(2.0 * PI * 300.0 * t);

                object = vec3(
                    2.0 * (-s + 0.5), 
                    z,
                    2.0 * (-t + 0.5)
                );
            }
            break;
        case 8:
            // +-------------------------------------------------+
            // |   Julia's Parametric Heart:                     |
            // |                                                 |
            // |   Expected input uv ranges:                     |
            // |   u = [0, PI)                                   |
            // |   v = [0, TWO_PI)                               |
            // +-------------------------------------------------+
            {
                object = vec3(
                    sin(u) * (15.0 * sin(v) - 4.0 * sin(3.0 * v)),
                    sin(u) * (15.0 * cos(v) - 5 * cos(2 * v) - 2 * cos(3 * v) - cos(2.0 * v)),
                    8.0 * cos(u)
                ) * 0.07;
            }
            break;
        case 9:
            // +-------------------------------------------------+
            // |   Johi's Heart:                                 |
            // |                                                 |
            // |   Expected input uv ranges:                     |
            // |   u = [0, PI)                                   |
            // |   v = [0, TWO_PI)                               |
            // +-------------------------------------------------+
            {
                object = to_sphere(u, v);

                if (u < HALF_PI) {
                    object.y *= 1.0 - (cos(sqrt(sqrt(abs(object.x * PI * 0.7)))) * 0.8);
                }
                else {
                    object.x *= sin(u) * sin(u);
                }
                object.x *= 0.9;
                object.z *= 0.4;

            }
            break;
        case 10:
            // +-------------------------------------------------+
            // |   Spherehog:                                    |
            // |                                                 |
            // |   Expected input uv ranges:                     |
            // |   u = [0, PI)                                   |
            // |   v = [0, TWO_PI)                               |
            // +-------------------------------------------------+
            {
                object = to_sphere(u, v);

                const float NUMSPIKES       = 10.0;
                const float SPIKENARROWNESS = 100.0;
                const float spikeheight     = 0.5;

                float repeatU = u / PI * NUMSPIKES - round(u / PI * NUMSPIKES);
                float repeatV = v / PI * NUMSPIKES - round(v / PI * NUMSPIKES);
                float d       = repeatU * repeatU + repeatV * repeatV;
                float r       = 1.0 + exp(-d * SPIKENARROWNESS) * spikeheight;

                object *= r;
            }
        case 11:
            // +-------------------------------------------------+
            // |   Spiky Heart:                                  |
            // |                                                 |
            // |   Expected input uv ranges:                     |
            // |   u = [0, PI)                                   |
            // |   v = [0, TWO_PI)                               |
            // +-------------------------------------------------+
            {
                object = to_sphere(u, v);

                if (u < HALF_PI) {
                    object.y *= 1.0 - (cos(sqrt(sqrt(abs(object.x * PI * 0.7)))) * 0.8);
                }
                else {
                    object.x *= sin(u) * sin(u);
                }
                object.x *= 0.90;
                object.z *= 0.40;

                const float PI_ACHTERL = PI / 8.0;
                if (u > PI_ACHTERL && u < PI - PI_ACHTERL - PI_ACHTERL) {
                    // Just copy&passteh from above:
                    const float NUMSPIKES       = 10.0;
                    const float SPIKENARROWNESS = 100.0;
                    const float spikeheight     = 0.4;

                    float repeatU = u / PI * NUMSPIKES - round(u / PI * NUMSPIKES);
                    float repeatV = v / PI * NUMSPIKES - round(v / PI * NUMSPIKES);
                    float d       = repeatU * repeatU + repeatV * repeatV;
                    float r       = 1.0 + exp(-d * SPIKENARROWNESS) * spikeheight;

                    object *= r;
                }
            }
            break;
        case 12:
            // +-------------------------------------------------+
            // |   SH Glyph:                                     |
            // |                                                 |
            // |   SH coefficients from here:                    |
            // |       https://www.shadertoy.com/view/dlGSDV     |
            // |                                                 |
            // |   Expected input uv ranges:                     |
            // |   u = [0, PI)                                   |
            // |   v = [0, TWO_PI)                               |
            // +-------------------------------------------------+
            {
                float time = ubo.mAbsoluteTime;

//	            int weightIndex = 0;
//	            float f = 0.; // Initialize to 0 and then add all the coefficients from the different bands
//	            for (int l = 0; l <= SH_DEGREE; l += 2) {
//		            for (int m = -l; m <= l; ++m) {
//			            float coeffScale = HomoSH(l, m, u, v);
//			            // float coeffScale = 1.0;
//			            //coeffScale = abs(coeffScale); // TODO: not sure if needed
//
//			            // static
//			             f += SH_COEFFS[weightIndex++] * coeffScale;
//
//			            // animated
////			            f += sin(float(0.02 * time * weightIndex)) * SH_COEFFS[weightIndex++] * coeffScale;
//		            }
//	            }

                float out_shs[15];
                eval_sh_4(out_shs, to_sphere(u, v));
                float f = 0.0;
			    for (int i = 0; i < 15; i++) {
                    f += out_shs[i] * SH_COEFFS[i];
                }

                object = to_sphere(u, v, f);
                float tmp = object.y;
                object.y = object.z;
                object.z = tmp;
            }
            break;
	    case 13: 
            // +--------------------+
            // | Michi Ball         |
            // +--------------------+
            object = to_sphere(u, v, 3.0);
            break;
	    case 14: 
	    case 15: 
            // +--------------------+
            // | Yarn Curve         |
            // +--------------------+
            {
                const float rowId = float(userData[0]);
                const float threadRadius = uintBitsToFloat(userData[2]);

                const float rowOffset = 5.835;
                const float loopRoundness = 1.354;
                const float loopHeight = 3.710;
                const float loopDepth = 0.619;

                vec3 gamma_t = yarn_curve(u, loopRoundness, loopHeight, loopDepth);
                gamma_t.y += rowId * rowOffset;

                vec3 nextGamma_t = yarn_curve(u + 0.001 /* <---- TODO */, loopRoundness, loopHeight, loopDepth);
				nextGamma_t.y += rowId * rowOffset;

                // Manually pipeify:
				vec3 fwd    = normalize(nextGamma_t - gamma_t);
				vec3 side   = orthogonal(fwd);
				vec3 newPos = gamma_t + rotate_around_axis(side * threadRadius, fwd, v);
				
                object = newPos;
            }
            break;
	    case 16: 
	    case 17: 
            // +--------------------+
            // | Fiber Curve        |
            // +--------------------+
            {
                const float rowId = float(userData[0]);
                const float fiberId = float(userData[1]);
                const float threadRadius = uintBitsToFloat(userData[2]);

                const float rowOffset = 5.835;
                const float loopRoundness = 1.354;
                const float loopHeight = 4.2;
                const float loopDepth = 0.619;

                const float yarnRadius = 0.5;
                const float fiberTwist = 5.0;

                vec3 eta_t = fiber_curve(u, loopRoundness, loopHeight, loopDepth, yarnRadius, fiberTwist, fiberId);
                eta_t.y += rowId * rowOffset;

                vec3 nextEta_t = fiber_curve(u + 0.001, loopRoundness, loopHeight, loopDepth, yarnRadius, fiberTwist, fiberId);
				nextEta_t.y += rowId * rowOffset;

                // Manually pipeify:
				vec3 fwd    = normalize(nextEta_t - eta_t);
				vec3 side   = orthogonal(fwd);
				vec3 newPos = eta_t + rotate_around_axis(side * threadRadius, fwd, v);
				
                object = newPos;
            }
            break;
            // +----------------------+
            // | Seashell variations: |
            // +----------------------+
        case 18:
            object = getSeashellPos(u, v, 87.0, 29.0, 4.309, 4.7, -39.944, 58.839, 9.0, 20.0, 1.0, 1.5, 2.0);
            break;
        case 19:
            object = getSeashellPos(u, v, 86.5, 10.0, 6.071, 5.153, -39.944, 1.0, 9.0, 11.0, 1.0, 1.5, 1.5);
            break;
        case 20:
            object = getSeashellPos(u, v, 86.5, 10.0, 6.071, 5.153, -39.944, 100.0, 50.0, 70.0, 0.5, 4.0, 5.0);
            break;
    }

    if (curveIndex == 15 || curveIndex == 17) {
        // +---------------------------+
        // | Animated Fiber/Yarn Curve |
        // +---------------------------+
        float time = ubo.mAbsoluteTime;
        float y = smoothstep(1500.0, 0.0, object.y);
        float x = object.x / 35.0;
        float a1 = 0.2;
        float a2 = 0.5;
        object.z += y * (100.0 * a1 * sin(x*0.5 - time) - a2 * sin(x*1.11 - time));
    }
    
    // Return homogeneous world space:
    return vec4(object, 1.0);
}

vec4 toCS(vec4 posWS)
{
    vec4 posCS = ubo.mViewProjMatrix * posWS;
    return posCS;
}

vec4 clampCS(vec4 posCS)
{
    float w = abs(posCS.w);
    posCS.x = clamp(posCS.x,  -w, w);
    posCS.y = clamp(posCS.y,  -w, w);
    posCS.z = clamp(posCS.z, 0.0, w);
    return posCS;
}

vec3 toScreen(vec4 posCS)
{
    posCS = clampCS(posCS);
    return cs_to_viewport(posCS, ubo.mResolutionAndCulling.xy);
}

// The following code has been stolen from:
//   Direct3D Rendering Cookbook
//   By : Justin Stenning

// Calculate point upon Bezier curve and return
void DeCasteljau(float u, vec3 p0, vec3 p1, vec3 p2, vec3 p3, out vec3 p, out vec3 t)
{
    vec3 q0 = mix(p0, p1, u);
    vec3 q1 = mix(p1, p2, u);
    vec3 q2 = mix(p2, p3, u);
    vec3 r0 = mix(q0, q1, u);
    vec3 r1 = mix(q1, q2, u);
    t = r0 - r1; // tangent
    p = mix(r0, r1, u);
}
// Bicubic interpolation of cubic Bezier surface
void DeCasteljauBicubic(vec2 uv, vec3 p[16], out vec3 result, out vec3 normal)
{
    // Interpolated values (e.g. points)
    vec3 p0, p1, p2, p3;
    // Tangents (derivatives)
    vec3 t0, t1, t2, t3;
    // Calculate tangent and positions along each curve
    DeCasteljau(uv.x, p[ 0], p[ 1], p[ 2], p[ 3], p0, t0);
    DeCasteljau(uv.x, p[ 4], p[ 5], p[ 6], p[ 7], p1, t1);
    DeCasteljau(uv.x, p[ 8], p[ 9], p[10], p[11], p2, t2);
    DeCasteljau(uv.x, p[12], p[13], p[14], p[15], p3, t3);
    // Calculate final position and tangents across surface
    vec3 du, dv, tmp;
    DeCasteljau(uv.y, p0, p1, p2, p3, result, dv);
    DeCasteljau(uv.y, t0, t1, t2, t3, du, tmp);
    // du represents tangent
    // dv represents bitangent
    normal = normalize(cross(du, dv));
}
