#version 450

// ================================================================

// Input Texture Atlas.
layout(binding = 0) uniform sampler2D atlas;

// Input RGBA values.
layout(location = 0) in vec4 in_RGBA;

// Input UV values.
layout(location = 1) in vec2 in_UV;

// Input T value.
layout(location = 2) flat in uint in_T;

// Output RGBA values.
layout(location = 0) out vec4 out_RGBA;

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

// Fragment Shader entry-point.
void main()
{
    if (in_T == 1)
    {
        vec4 tex_color = texture(atlas, in_UV);
        out_RGBA = tex_color * in_RGBA;
    }
    else
    {
        out_RGBA = in_RGBA;
    }
}

// ================================================================
