// +--------------------------------------------------------------+
// |                                                              |
// |   This file contains a set of useful shape functions         |
// |   which (hopefully ^^) can be used to combine different      |
// |   shapes and stitch together stuff in code (more) easily.    |
// |                                                              |
// |   Has been ported-over from ParametricCurvesPlayground       |
// |                                                              |
// +--------------------------------------------------------------+

// But first...
#define HALF_PI   1.57079632679
#define PI        3.14159265359
#define TWO_PI    6.28318530718
#define SQRT_PI   1.77245385091

// +--------------------------------------------------------------+
// |                                                              |
// |   Noise utils                                                |
// |                                                              |
// +--------------------------------------------------------------+

vec4 mod289(vec4 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x)
{
    return mod289(((x*34.0)+10.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
    return 1.79284291400159 - 0.85373472095314 * r;
}

vec2 fade(vec2 t) {
    return t*t*t*(t*(t*6.0-15.0)+10.0);
}

// Classic Perlin noise, stolen from here: https://stegu.github.io/webgl-noise/webdemo/
float cnoise(vec2 P)
{
    vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod289(Pi); // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;

    vec4 i = permute(permute(ix) + iy);

    vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
    vec4 gy = abs(gx) - 0.5 ;
    vec4 tx = floor(gx + 0.5);
    gx = gx - tx;

    vec2 g00 = vec2(gx.x,gy.x);
    vec2 g10 = vec2(gx.y,gy.y);
    vec2 g01 = vec2(gx.z,gy.z);
    vec2 g11 = vec2(gx.w,gy.w);

    vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;  
    g01 *= norm.y;  
    g10 *= norm.z;  
    g11 *= norm.w;  

    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));

    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}

// Classic Perlin noise, periodic variant
float pnoise(vec2 P, vec2 rep)
{
    vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod(Pi, rep.xyxy); // To create noise with explicit period
    Pi = mod289(Pi);        // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;

    vec4 i = permute(permute(ix) + iy);

    vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
    vec4 gy = abs(gx) - 0.5 ;
    vec4 tx = floor(gx + 0.5);
    gx = gx - tx;

    vec2 g00 = vec2(gx.x,gy.x);
    vec2 g10 = vec2(gx.y,gy.y);
    vec2 g01 = vec2(gx.z,gy.z);
    vec2 g11 = vec2(gx.w,gy.w);

    vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;  
    g01 *= norm.y;  
    g10 *= norm.z;  
    g11 *= norm.w;  

    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));

    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}

// +--------------------------------------------------------------+
// |                                                              |
// |   Math utils                                                 |
// |                                                              |
// +--------------------------------------------------------------+

// Get a vector which is orthogonal to the given vector v:
vec3 orthogonal(vec3 v) 
{
	if (v.x == 0 && v.y == 0) {
		return vec3(1, 0, 0);
	} else {
		return normalize(vec3(v.y, -v.x, 0));
	}
}

// Compute value^2
float pow2(float value) {
	return value * value;
}

// Compute value^3
float pow3(float value) {
	return value * value * value;
} 

// Compute pow(float, uint)
float pow(float value, uint power) {
    float result = 1.0;
    for (uint i = 1; i <= power; ++i) {
        result *= value;
    }
    return result;
}

float norm2(vec3 v) {
	return dot(v, v);
}

float norm(vec3 v) {
	return sqrt(norm2(v));
}

// Pair to hold two float values: first and second
struct pair
{
    float first;
    float second;
};

// Pair to hold two int values: first and second
struct ipair
{
    int first;
    int second;
};

// Pair to hold two uint values: first and second
struct upair
{
    uint first;
    uint second;
};

// Swaps a and b in place:
void swap(inout float a, inout float b)
{
    float temp = a;
    a          = b;
    b          = temp;
}

// Swaps a and b in place:
void swap(inout int a, inout int b)
{
    int temp = a;
    a        = b;
    b        = temp;
}

// Swaps a and b in place:
void swap(inout uint a, inout uint b)
{
    uint temp = a;
    a         = b;
    b         = temp;
}

/** Rotates v around the x-axis by phi radians
 * \brief R_x
 * \param v Vector to be rotated
 * \param phi Angle to be rotated by
 * \return Rotated vector
 */
vec3 rotate_x(vec3 v, float phi) 
{
	const float cos_phi = cos(phi);
	const float sin_phi = sin(phi);
	return vec3(
		/* x: */ v.x, 
		/* y: */ cos_phi * v.y - sin_phi * v.z, 
		/* z: */ sin_phi * v.y + cos_phi * v.z
    );
}

/** Rotates v around the y-axis by phi radians
 * \brief R_y
 * \param v Vector to be rotated
 * \param phi Angle to be rotated by
 * \return Rotated vector
 */
vec3 rotate_y(vec3 v, float phi) 
{
	const float cos_phi = cos(phi);
	const float sin_phi = sin(phi);
	return vec3(
		/* x: */ cos_phi * v.x + sin_phi * v.z, 
		/* y: */ v.y, 
		/* z: */ -sin_phi * v.x + cos_phi * v.z
    );
}

/** Rotates v around the z-axis by phi radians
 * \brief R_z
 * \param v Vector to be rotated
 * \param phi Angle to be rotated by
 * \return Rotated vector
 */
vec3 rotate_z(vec3 v, float phi) {
	const float cos_phi = cos(phi);
	const float sin_phi = sin(phi);
	return vec3(
		/* x: */ cos_phi * v.x - sin_phi * v.y, 
		/* y: */ sin_phi * v.x + cos_phi * v.y, 
		/* z: */ v.z
    );
}

// +--------------------------------------------------------------+
// |                                                              |
// |   Shape functions                                            |
// |                                                              |
// +--------------------------------------------------------------+

/** Basically does not modify anything. Outputs a and b on the plane at z=0.
 * \brief R2 -> R3 plane
 * \param a x coordinate
 * \param b y coordinate
 * \return a and b on the plane at z=0
 */
vec3 to_plane(float a, float b) 
{ 
    return vec3(a, 0.0, b);
}

/** Outputs a scaled plane on y=0.
 * \brief R2 -> R3 plane with scaling
 * \param a x coordinate
 * \param b y coordinate
 * \param scaleA scale of x coordinates
 * \param scaleB scale of y coordinates
 * \return a and b scaled on the plane at z=0
 */
vec3 to_plane(float a, float b, float scaleA, float scaleB)
{
    return vec3(
        /* x: */ a * scaleA,
        /* y: */ 0.0,
        /* z: */ b * scaleB
    );
}

/** Moves the input points onto a circle with radius 1 on y=0.
 * \brief R1 -> R3 circle
 * \param phi Expected input range: [0, 2*PI]
 * \return Position according to the parametric circle equation
 */
vec3 to_circle(float phi)
{
    return vec3(
        /* x: */ sin(phi),
        /* y: */ 0.0,
        /* z: */ cos(phi)
    );
}

/** Moves the input points onto a circle with radius r on y=0.
 * \brief R1 -> R3 circle with radius
 * \param phi Expected input range: [0, 2*PI]
 * \param r Radius of the circle
 * \return Position according to the parametric circle equation
 */
vec3 to_circle(float phi, float r)
{
    return vec3(
        /* x: */ r * sin(phi),
        /* y: */ 0.0,
        /* z: */ r * cos(phi)
    );
}

/** Turns a given circle on the xz-plane into an ellipse.
 * \brief x*a, y*1, z*b
 * \param circle Input coordinates of a circle
 * \param a Ellipse parameter x
 * \param b Ellipse parameter z
 * \return Modified coordinates of the circle, now an ellipse
 */
vec3 to_ellipse(vec3 circle, float a, float b)
{
    return vec3(
        /* x: */ circle.x * a,
        /* y: */ circle.y,
        /* z: */ circle.z * b
    );
}

/** Moves the points onto the sphere surface
 * \brief R2 -> R3 sphere
 * \param u Expected input range: [0, PI]
 * \param v Expected input range: [0, 2*PI]
 * \return Position according to the parametric sphere equation
 */
vec3 to_sphere(float u, float v)
{
    return vec3(
        /* x: */ sin(u) * cos(v),
        /* y: */ cos(u),
        /* z: */ sin(u) * sin(v)
    );
}

/** Moves the points onto the sphere surface with a given radius.
 * \brief R2 -> R3 sphere with radius
 * \param u Expected input range: [0, PI]
 * \param v Expected input range: [0, 2*PI]
 * \param r Radius of the sphere
 * \return Position according to the parametric sphere equation
 */
vec3 to_sphere(float u, float v, float r)
{
    return vec3(
        /* x: */ r * sin(u) * cos(v),
        /* y: */ r * cos(u),
        /* z: */ r * sin(u) * sin(v)
    );
}

/** Moves the points onto the cone surface
 * \brief R2 -> R3 cone
 * \param u Expected input range: [0, 1]
 * \param v Expected input range: [0, 2*PI]
 * \return Position according to the parametric cone equation
 */
vec3 to_cone(float u, float v)
{
    float r = u;
    return vec3(
        /* x: */ r * -cos(v),
        /* y: */ r,
        /* z: */ r * sin(v)
    );
}

/** Creates an interval of a given width and offset.
 * \brief Step function which is 1 for width, and 0 in between
 * \param x Input variable
 * \param offset To offset the start of the width-sized step
 * \param interval Interval after which to repeat the step
 * \param width width of where the step function is 1
 * \return Either 1 or 0
 */
int is_in_interval(float x, float offset, float interval, float width) 
{ 
    return mod((x + offset), interval) < width ? 1 : 0; 
}

/** Rotates vector v around an axis by theta radians
 * \brief Rodrigues' rotation formula
 * \param v a vector in 3D space
 * \param axis a unit vector describing the axis of rotation
 * \param theta the angle (in radians) that v rotates around k
 * \return A rotated vector.
 */
vec3 rotate_around_axis(vec3 v, vec3 axis, float theta)
{
    const float cosTheta = cos(theta);
    const float sinTheta = sin(theta);
    return v * cosTheta + cross(axis, v) * sinTheta + axis * dot(axis, v) * (1.0 - cosTheta);
}

/**
 * \brief Generates positions on a pipe surface.
 * \param pos Current pipe position
 * \param forwardVec Vector pointing in pipe diretion
 * \param tCircle An interpolation parameter for the circle, range [0, 2*PI]
 * \param radius Thickness of the pipe
 * \return A coordinate which lies on the pipe surface.
 */
vec3 pipeify(vec3 pos, vec3 forwardVec, float tCircle, float radius)
{
    vec3 n = orthogonal(forwardVec);
    return pos + rotate_around_axis(n * radius, forwardVec, tCircle);
}

/**
 * \brief Generates positions on a pipe surface.
 * \param pos Current pipe position
 * \param forwardVec Vector pointing in pipe diretion
 * \param tCircle An interpolation parameter for the circle, range [0, 2*PI]
 * \param radius Thickness of the pipe
 * \return A coordinate which lies on the pipe surface.
 */
vec3 palmtreeify(vec3 pos, vec3 forwardVec, float tCircle, float radius)
{
    vec3 n = orthogonal(forwardVec);
    return pos + rotate_around_axis(n * radius, forwardVec, tCircle);
}

/**	Calculates the binomial coefficient, a.k.a. n over k
 */
int binomial_coefficient(int n, int k)
{
    // return n.0actorial() / ((n - k).0actorial() * k.0actorial());

    // Optimized method, see http://stackoverflow.com/questions/9619743/how-to-calculate-binomial-coefficents-for-large-numbers
    // (n C k) and (n C (n-k)) are the same, so pick the smaller as k:
    if (k > n - k) {
        k = n - k;
    }
    int result = 1;
    for (int i = 1; i <= k; ++i) {
        result *= n - k + i;
        result /= i;
    }
    return result;
}

/// <summary>
/// Calculates the bernstein polynomial b_{i,n}(t)
/// </summary>
/// <returns>
/// The polynomial.
/// </returns>
/// <param name='i'>
/// The i parameter
/// </param>
/// <param name='n'>
/// The n parameter
/// </param>
/// <param name='t'>
/// t
/// </param>
float bernstein_polynomial(int i, int n, float t)
{
    // TODO: Verify that I've ported correctly
    return binomial_coefficient(n, i) * pow(t, i) * pow(1.0 - t, n-i);
}

#define NUM_BEZIER_CONTROL_POINTS   8

/** Evaluates the position of a Bezier curve with the given control points at position t.
 * \brief Bezier curve at t.
 * \param numControlPoints Number of control points passed via the second parameter.
 * \param controlPoints Pointer to an array of numControlPoints values, i.e., e.g.: vec3 array[numControlPoints]
 * \param t Where to get the interpolated Bezier curve value. Range: [0, 1]
 * \return A position.
 */
vec3 bezier_value_at(int numControlPoints, vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS], float t)
{
    // TODO: Use that casting-trick to enable an arbitrary number of control points?!
    int    n = numControlPoints - 1;
    vec3 sum = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i <= n; ++i) {
        sum += bernstein_polynomial(i, n, t) * controlPoints[i];
    }
    return sum;
}

/** Evaluates the slope (i.e. forward direction) of a Bezier curve with the given control points at position t.
 * \brief Bezier curve slope at t.
 * \param numControlPoints Number of control points passed via the second parameter.
 * \param controlPoints Pointer to an array of numControlPoints values, i.e., e.g.: vec3 array[numControlPoints]
 * \param t Where to get the interpolated Bezier curve slope. Range: [0, 1]
 * \return A direction vector.
 */
vec3 bezier_slope_at(int numControlPoints, vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS], float t)
{
    int    n         = numControlPoints - 1;
    int    nMinusOne = n - 1;
    vec3 sum = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i <= nMinusOne; ++i) {
        sum += (controlPoints[i + 1] - controlPoints[i]) * bernstein_polynomial(i, nMinusOne, t);
    }
    return float(n) * sum;
}

/** Evaluates the position of a Catmull-Rom spline with the given control points at position t.
 * \brief Catmull-Rom spline at t.
 * \param numControlPoints Number of control points passed via the second parameter.
 * \param controlPoints Pointer to an array of numControlPoints values, i.e., e.g.: vec3 array[numControlPoints]
 *                      Attention: First and last control points MUST be duplicated!
 *						E.g., for a curve with 4 control points, the array must be of size 6!
 * \param t Where to get the interpolated Catmull-Rom spline value. Range: [0, 1]
 * \return A position.
 */
vec3 catmull_rom_value_at(int numControlPoints, vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS], float t)
{
    int   numSections = numControlPoints - 3;
    float tns         = t * numSections;
    int   section     = min(int(tns), numSections - 1);
    float u           = tns - float(section);

    vec3 a = controlPoints[section];
    vec3 b = controlPoints[section + 1];
    vec3 c = controlPoints[section + 2];
    vec3 d = controlPoints[section + 3];

    return .5 * ((-a + 3.0 * b - 3.0 * c + d) * (u * u * u) + (2.0 * a - 5.0 * b + 4.0 * c - d) * (u * u) + (-a + c) * u + 2.0 * b);
}

/** Evaluates the slope (i.e. forward direction) of a Catmull-Rom spline with the given control points at position t.
 * \brief Catmull-Rom spline slope at t.
 * \param numControlPoints Number of control points passed via the second parameter.
 * \param controlPoints Pointer to an array of numControlPoints values, i.e., e.g.: vec3 array[numControlPoints]
 *                      Attention: First and last control points MUST be duplicated!
 *						E.g., for a curve with 4 control points, the array must be of size 6!
 * \param t Where to get the interpolated Catmull-Rom spline slope. Range: [0, 1]
 * \return A direction vector.
 */
vec3 catmull_rom_slope_at(int numControlPoints, vec3 controlPoints[NUM_BEZIER_CONTROL_POINTS], float t)
{
    int   numSections = numControlPoints - 3;
    float tns         = t * numSections;
    int   section     = min(int(tns), numSections - 1);
    float u           = tns - float(section);

    vec3 a = controlPoints[section];
    vec3 b = controlPoints[section + 1];
    vec3 c = controlPoints[section + 2];
    vec3 d = controlPoints[section + 3];

    return 1.5 * (-a + 3.0 * b - 3.0 * c + d) * (u * u) + (2.0 * a - 5.0 * b + 4.0 * c - d) * u + .5 * c - .5 * a;
}

vec3 yarn_curve(float t, float a, float h, float d)
{
    vec3 gamma_t;
    gamma_t.x = t + a * sin(2. * t);
    gamma_t.y = h * cos(t);
    gamma_t.z = d * cos(2. * t);
    return gamma_t;
}

void frenet_frame(float t, float a, float h, float d, out vec3 e1, out vec3 e2, out vec3 e3)
{
    // Code from:
    // A Simple Parametric Model of Plain-Knit Yarns
    // by Keenan Crane, 2023
    // https://github.com/keenancrane/plain-knit-yarn

    float u_t, v_t, x_t, y_t;

    e1.x = 1.0 + 2.0 * a * cos(2.0 * t);
    e1.y = -h * sin(t);
    e1.z = -2.0 * d * sin(2.0 * t);

    u_t = norm2(e1);
    v_t = 2.0 * h * h * cos(t) * sin(t) + 16.0 * d * d * cos(2.0 * t) * sin(2.0 * t) - 8.0 * a * (1.0 + 2.0 * a * cos(2.0 * t)) * sin(2.0 * t);
    x_t = 1.0 / sqrt(u_t);
    y_t = v_t / (2.0 * pow(u_t, 3.0 / 2.0));

    e2.x = y_t * (-1.0 - 2.0 * a * cos(2.0 * t)) - x_t * 4.0 * a * sin(2.0 * t);
    e2.y = y_t * h * sin(t) - x_t * h * cos(t);
    e2.z = y_t * 2.0 * d * sin(2.0 * t) - x_t * 4.0 * d * cos(2.0 * t);

    e1 = x_t * e1;               // scale
    e2 = (1.0 / norm(e2)) * e2; // scale
    e3 = cross(e1, e2);
}

vec3 fiber_curve(float t, float a, float h, float d, float r, float omega, float phi)
{
    // Code from:
    // A Simple Parametric Model of Plain-Knit Yarns
    // by Keenan Crane, 2023
    // https://github.com/keenancrane/plain-knit-yarn

    vec3 eta_t;
    vec3 e1, e2, e3;
    float  theta_t;

    vec3 gamma_t = yarn_curve(t, a, h, d);
    frenet_frame(t, a, h, d, e1, e2, e3); // Note: e1, e2, and e3 are out parameters
    theta_t = t * omega - 2. * cos(t) + phi;

    eta_t.x = gamma_t.x + r * (cos(theta_t) * e2.x + sin(theta_t) * e3.x);
    eta_t.y = gamma_t.y + r * (cos(theta_t) * e2.y + sin(theta_t) * e3.y);
    eta_t.z = gamma_t.z + r * (cos(theta_t) * e2.z + sin(theta_t) * e3.z);

    return eta_t;
}
