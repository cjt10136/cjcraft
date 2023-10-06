#include "command.h"

void copy_buffer(vk::CommandBuffer cb, Buffer src, Buffer dst) noexcept
{
  cb.copyBuffer(src.handle,
                dst.handle,
                {{gsl::narrow<unsigned long long>(src.offset),
                  gsl::narrow<unsigned long long>(dst.offset),
                  gsl::narrow<unsigned long long>(src.size)}});
}