
#pragma once

#include <memory>
#include <string>
#include "core/geometry/surface.h"
#include "core/geometry/trimesh.h"


namespace olio {
namespace core {

class BVHTriMeshFace : public Surface {
public:
  OLIO_NODE(BVHTriMeshFace)

  explicit BVHTriMeshFace(TriMesh::Ptr mesh, TriMesh::FaceHandle fh);

  bool Hit(const Ray &ray, Real tmin, Real tmax, HitRecord &hit_record) override;
  AABB GetBoundingBox(bool force_recompute=false) override;
protected:
  TriMesh::Ptr mesh_;
  TriMesh::FaceHandle fh_;
private:
};

}  // namespace core
}  // namespace olio
