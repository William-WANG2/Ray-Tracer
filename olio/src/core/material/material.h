//! \file       material.h
//! \brief      Material class

#pragma once

#include <memory>
#include <string>
#include "core/types.h"
#include "core/node.h"

namespace olio {
namespace core {

class Ray;
class HitRecord;

//! \class Material
//! \brief Material class
class Material : public Node {
public:
  OLIO_NODE(Material)
  explicit Material(const std::string &name=std::string());
protected:
};

}  // namespace core
}  // namespace olio
