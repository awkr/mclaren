#version 460 core

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_fragment_shader_barycentric : require

#include "global_state.glsl"

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
layout (location = 0) out vec4 frag_color;

layout (set = 1, binding = 0) uniform sampler2D tex;
layout (set = 2, binding = 0) uniform DirLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} dir_light;

void main() {
    // frag_color = texture(tex, tex_coord);

    // const vec3 base_color = vec3(0.9, 0.9, 0.9);
    const vec3 base_color = texture(tex, tex_coord).rgb;
    vec3 ambient = dir_light.ambient * base_color;
    vec3 norm = normalize(normal);
    vec3 light_dir = normalize(-dir_light.direction);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = dir_light.diffuse * (diff * base_color);
    // float diffuse = max(dot(normal, global_state.sunlight_dir), 0.0);
    // vec4 color = vec4(base_color * (diffuse + 0.1f), 1.0);
    vec3 result = ambient + diffuse;
    frag_color = vec4(result, 1.0);
    return;

/*
    // frag_color = vec4(gl_BaryCoordEXT, 1.0);

//    const vec3 bary_coord = gl_BaryCoordNoPerspEXT;
    const vec3 bary_coord = gl_BaryCoordEXT;

    //
    // 获取重心坐标的最小值，代表片段离三角形边界的距离（在重心坐标系下）
    float minBary = min(bary_coord.x, min(bary_coord.y, bary_coord.z));

    // 获取当前片段的位置，gl_FragCoord.xy 是窗口坐标系中的片段坐标
    vec2 fragCoord = gl_FragCoord.xy;

    // 获取片段相对于屏幕分辨率的近似宽度（以像素为单位）
    float edgeThickness = fwidth(minBary);// 计算片段到边缘的屏幕空间宽度
//    float pixelWidth = 1.0;
    float pixelWidth = 0.25;

    // 如果片段距离边缘接近 1 个像素宽，则渲染为线框
    if (minBary < pixelWidth * edgeThickness) {
        frag_color = vec4(255.0 / 255.0, 151.0 / 255.0, 0.0 / 255.0, 1.0);
    } else {
        frag_color = color;
    }

    //
//    // 计算最小的重心坐标，表示片段与边缘的距离
//    float minBary = min(bary_coord.x, min(bary_coord.y, bary_coord.z));
//
//    // 使用重心坐标的导数（梯度）计算屏幕空间下的边缘宽度
//    vec3 dBarydx = dFdx(bary_coord);
//    vec3 dBarydy = dFdy(bary_coord);
//
//    // 计算屏幕空间的边缘宽度，使用 max 来避免过小的宽度导致的错误
//    float edgeWidth = max(length(dBarydx), length(dBarydy));
//    float pixelWidth = 1.0;
//
//    // 如果片段位于线框区域内，渲染线框颜色；否则渲染实体颜色
//    if (minBary < pixelWidth * edgeWidth) {
//        frag_color = vec4(255.0 / 255.0, 151.0 / 255.0, 0.0 / 255.0, 1.0);
//    } else {
//        frag_color = color;
//    }

*/
}
