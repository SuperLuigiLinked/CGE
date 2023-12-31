#version 450

// ================================================================

// Input XYXW values.
layout(location = 0) in vec4 in_XYZW;

// Input UV values.
layout(location = 1) in vec2 in_UV;

// Input ST values.
layout(location = 2) in uvec2 in_ST;

// Output RGBA values.
layout(location = 0) out vec4 out_RGBA;

// Output UV values.
layout(location = 1) out vec2 out_UV;

// Output T value.
layout(location = 2) out uint out_T;

// ================================================================

float from_srgb(const float val)
{
    return val < 0.04045 ? val / 12.92 : pow((val + 0.055) / 1.055, 2.4);
}

float to_srgb(const float val)
{
    return val < 0.04045 / 12.92 ? val * 12.92 : pow(val, 1.0 / 2.4) * 1.055 - 0.055;
}

// ================================================================

// Vertex Shader entry-point.
void main()
{
    // Use only the XYZ-coordinates.
    gl_Position = vec4(in_XYZW.xyz, 1.0);
    
    uint s = in_ST.x;
    uint t = in_ST.y;

    float r = float((s >> 16) & 255) / 255.0;
    float g = float((s >>  8) & 255) / 255.0;
    float b = float((s      ) & 255) / 255.0;
    float a = float((s >> 24) & 255) / 255.0;

    // Gamma Correction
    r = from_srgb(r);
    g = from_srgb(g);
    b = from_srgb(b);

    // Pass-through the RGBA Values. They will be interpolated between each point.
    out_RGBA = vec4(r, g, b, a);

    // Pass-through the UV Values. They will be interpolated between each point.
    out_UV = in_UV;

    // Pass-through the T Value.
    out_T = t;
}

// ================================================================
