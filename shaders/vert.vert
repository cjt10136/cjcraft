#version 450

layout(binding = 0) uniform Uniform {
  mat4 view_proj;
} ubo;

layout(push_constant) uniform Push {
  vec3 offset;
} push;

layout(location = 0) in uint data;

layout(location = 0) out vec2 uv;

void main()
{
  uv = vec2(data & 0xfu, (data >> 4) & 0xfu);

  uint pos_y = (data >> 24) & 0xffu;
  uint pos_x = (data >> 16) & 0xffu;
  uint pos_z = (data >> 8) & 0xffu;
  gl_Position = ubo.view_proj * vec4(vec3(pos_x, pos_y, pos_z) + push.offset, 1.0);
}