
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

// +----------------------+
// | Seashell variations: |
// +----------------------+

vec3 get_seashell1(float u, float v)
{
	return getSeashellPos(u, v, 87.0, 29.0, 4.309, 4.7, -39.944, 58.839, 9.0, 20.0, 1.0, 1.5, 2.0);
}

vec3 get_seashell2(float u, float v)
{
	return getSeashellPos(u, v, 86.5, 10.0, 6.071, 5.153, -39.944, 1.0, 9.0, 11.0, 1.0, 1.5, 1.5);
}

vec3 get_seashell3(float u, float v)
{
	return getSeashellPos(u, v, 86.5, 10.0, 6.071, 5.153, -39.944, 100.0, 50.0, 70.0, 0.5, 4.0, 5.0);
}
