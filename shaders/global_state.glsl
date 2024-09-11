layout (set = 0, binding = 0) uniform GlobalState {
    mat4 view;
    mat4 projection;
    vec3 sunlight_dir; // in world space
    // vec4 sunlight_color; // sunlight color and intensity ( power )
} global_state;
