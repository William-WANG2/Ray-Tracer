//! \file       material.cc
//! \brief      Material class

#include "core/material/material.h"
#include "core/ray.h"

namespace olio {
namespace core {

Material::Material(const std::string &name) :
  Node{}
{
  name_ = name.size() ? name : "Material";
}

}  // namespace core
}  // namespace olio
