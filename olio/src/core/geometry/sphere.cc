//! \file       sphere.cc
//! \brief      Sphere class

#include "core/geometry/sphere.h"
#include <spdlog/spdlog.h>
#include "core/ray.h"
#include "core/face_geouv.h"
#include <cmath>

namespace olio {
namespace core {

Sphere::Sphere(const std::string &name) :
  Surface{}
{
  name_ = name.size() ? name : "Sphere";
}


Sphere::Sphere(const Vec3r &center, Real radius, const std::string &name) :
  Surface{},
  center_{center},
  radius_{radius}
{
  name_ = name.size() ? name : "Sphere";
}


void
Sphere::SetCenter(const Vec3r &center)
{
  center_ = center;
  bound_dirty_ = true;
}


void
Sphere::SetRadius(Real radius)
{
  radius_ = radius;
  bound_dirty_ = true;
}


AABB
Sphere::GetBoundingBox(bool force_recompute)
{
  // if bound is clean, just return existing bbox_
  if (!force_recompute && !IsBoundDirty())
    return bbox_;

  // compute sphere bbox
  bbox_.Reset();
  Vec3r radius3{radius_, radius_, radius_};
  bbox_.ExpandBy(AABB{center_ - radius3, center_ + radius3});
  bound_dirty_ = false;
  return bbox_;
}


bool
Sphere::Hit(const Ray &ray, Real tmin, Real tmax, HitRecord &hit_record)
{
  auto p0 = ray.GetOrigin() - center_;
  auto v = ray.GetDirection();
  auto a = v.squaredNorm();
  auto b = 2 * p0.dot(v);
  auto c = p0.squaredNorm() - radius_ * radius_;

  // solve quadratic equation to find t
  auto a2 = 2 * a;
  auto discriminant = b * b - 2 * a2 * c;
  if (discriminant < 0)
    return false;
  auto s = static_cast<Real>(sqrt(discriminant));
  auto t = (-b - s) / a2;
  if (t < tmin)
    t = (-b + s) / a2;
  if (t < tmin || t > tmax)
    return false;

  // fill hit record
  const Vec3r &hit_point = ray.At(t);
  hit_record.SetRayT(t);
  hit_record.SetPoint(hit_point);
  hit_record.SetNormal(ray, (hit_point - center_).normalized());
  hit_record.SetSurface(GetPtr());

  Real phi = atan2(hit_point[1] - center_[1], hit_point[0]- center_[0]);
  phi = phi >= 0 ? phi : phi+k2Pi;
  Real theta = acos((hit_point[2]- center_[2])/(hit_point-center_).norm());
  Vec2r uv{phi/k2Pi, theta/kPi};
  FaceGeoUV face_geo_uv{-1, Vec2r{-1, -1}, uv};
  hit_record.SetFaceGeoUV(face_geo_uv);

  return true;
}

}  // namespace core
}  // namespace olio
