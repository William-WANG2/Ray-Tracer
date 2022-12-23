//! \file       texture.cc
//! \brief      Texture class

#include "core/texture/texture.h"

namespace olio {
namespace core {

Texture::Texture(const std::string &name) :
  Node{}
{
  name_ = name.size() ? name : "Texture";
}


SolidTexture::SolidTexture(const Vec3r &color, const std::string &name) :
  Texture{},
  color_{color}
{
  name_ = name.size() ? name : "SolidTexture";
}

}  // namespace core
}  // namespace olio
