#version 450

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 color;

void main()
{
   color = texture(tex_sampler, uv / 16);
   if (color.w == 0) {
     discard;
   }
}