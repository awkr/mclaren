#version 460 core

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_fragment_shader_barycentric : require

//layout (early_fragment_tests) in;

 struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cut_off;
    float outer_cut_off;
};

layout (location = 0) in  vec2 tex_coord;
layout (location = 1) in  vec3 normal;
layout (location = 2) in noperspective vec3 bary_coord;
layout (location = 3) flat in uint texture_index;
layout (location = 4) in float w;

layout (location = 0) out vec4 frag_color;

layout (set = 0, binding = 1) uniform DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
} dir_light;

layout (set = 0, binding = 3) uniform sampler2D textures[];

float line_width = 3.0;
vec3 lineColor = vec3(1.0, 0.0, 0.0);
vec3 color = vec3(0.0, 1.0, 0.0);

float edge_factor() {
    vec3 bc = bary_coord / w; // 执行透视校正：除以w进行校正
    bc = bc / (bc.x + bc.y + bc.z); // 归一化重心坐标

    //vec3 d = fwidth(bc); // 计算屏幕空间导数

    vec3 x = dFdx(bc);
    vec3 y = dFdy(bc);
    vec3 d = sqrt(x * x + y * y);

    //vec3 f = step(d * line_width, bc); // 使用校正后的重心坐标进行边缘检测
    vec3 f = smoothstep(vec3(0.0), d * line_width, bc); // 使用校正后的重心坐标进行边缘检测
    return min(min(f.x, f.y), f.z);
}

void main() {
    //frag_color = vec4(min(vec3(edge_factor()), vec3(1.0, 1.0, 1.0)), 1.0);
    //frag_color = vec4(mix(lineColor, color, edge_factor()), 1.0);
    frag_color = vec4(vec3(gl_FragCoord.z), 1.0);
}
