//! \file       light.cc
//! \brief      Node class

#include "core/light/light.h"
#include "core/ray.h"
#include "core/material/phong_material.h"
#include <cmath>
// #include <random>

// std::random_device rd;  
// std::mt19937 gen(rd()); 
// std::uniform_real_distribution<> dis(0, 1.0);

namespace olio {
namespace core {

using namespace std;

Light::Light(const std::string &name) :
  Node{name}
{
  name_ = name.size() ? name : "Light";
}


Vec3r
Light::Illuminate(const HitRecord &/*hit_record*/, const Vec3r &/*view_vec*/,
                  Surface::Ptr /*scene*/) const
{
  return Vec3r{0, 0, 0};
}


AmbientLight::AmbientLight(const std::string &name) :
  Light{name}
{
  name_ = name.size() ? name : "AmbientLight";
}


AmbientLight::AmbientLight(const Vec3r &ambient, const std::string &name) :
  Light{name},
  ambient_{ambient}
{
  name_ = name.size() ? name : "AmbientLight";
}


Vec3r
AmbientLight::Illuminate(const HitRecord &hit_record, const Vec3r &/*view_vec*/,
                         Surface::Ptr /*scene*/) const
{
  // only process phong materials
  auto surface = hit_record.GetSurface();
  if (!surface)
    return Vec3r{0, 0, 0};
  auto phong_material = dynamic_pointer_cast<PhongMaterial>(surface->
                                                            GetMaterial());
  if (!phong_material)
    return Vec3r{0, 0, 0};
  return ambient_.cwiseProduct(phong_material->GetAmbient());
}


PointLight::PointLight(const std::string &name) :
  Light{name}
{
  name_ = name.size() ? name : "PointLight";
}


PointLight::PointLight(const Vec3r &position, const Vec3r &intensity,
                       const std::string &name) :
  Light{name},
  position_{position},
  intensity_{intensity}
{
  name_ = name.size() ? name : "PointLight";
}


Vec3r
PointLight::Illuminate(const HitRecord &hit_record, const Vec3r &view_vec,
                       Surface::Ptr scene) const
{
  // evaluate hit points material
  Vec3r black{0, 0, 0};

  // create a shadow ray to the point light and check for occlusion
  const auto &hit_position = hit_record.GetPoint();
  Ray shadow_ray{hit_position, GetPosition() - hit_position};
  HitRecord shadow_record;
  if (scene->Hit(shadow_ray, kEpsilon, 1, shadow_record)) {
    return black;
  }

  // only process phong materials
  auto surface = hit_record.GetSurface();
  if (!surface)
    return black;
  auto phong_material = dynamic_pointer_cast<PhongMaterial>(surface->
                                                            GetMaterial());
  if (!phong_material)
    return black;

  // compute irradiance at hit point
  const Vec3r &normal = hit_record.GetNormal();
  Vec3r light_vec = position_ - hit_position;
  auto distance2 = light_vec.squaredNorm();
  light_vec.normalize();
  auto denominator = std::max(kEpsilon2, distance2);
  Vec3r irradiance = intensity_ * fmax(0.0f, normal.dot(light_vec))/denominator;

  // compute how much the material absorts light
  const Vec3r &attenuation = phong_material->Evaluate(hit_record, light_vec,
                                                      view_vec);
  return irradiance.cwiseProduct(attenuation);
}




AreaLight::AreaLight(const Vec3r &center, const Vec3r &normal, const Vec3r &u, Real len, const Vec3r &intensity, const std::string &name) :
  center_{center},
  normal_{normal},
  u_{u},
  intensity_{intensity},
  len_{len}
{
  v_ = (u_.cross(normal_)).normalized();
}

std::vector<Vec3r> AreaLight::GeneratePoints() const {
  int size_grid = static_cast<int>(round(sqrt(shadow_samples_)));
  std::vector<Vec3r> points;
  points.reserve(static_cast<unsigned long>(size_grid*size_grid));
  Real fraction = static_cast<Real>(1)/size_grid;
  for(int i=1; i<=size_grid; i++) {
    for(int j=1; j<=size_grid; j++) {
      Vec3r left_top = center_ - len_*(0.5*u_ + 0.5*v_);
      Vec3r u_shift = ((i-1) + static_cast<Real>(rand())/(RAND_MAX+1.0f))*u_*len_*fraction;
      Vec3r v_shift = ((j-1) + static_cast<Real>(rand())/(RAND_MAX+1.0f))*v_*len_*fraction;
      // Vec3r u_shift = ((i-1)*fraction + i*fraction*dis(gen))*u_*len_;
      // Vec3r v_shift = ((j-1)*fraction + j*fraction*dis(gen))*v_*len_;
      points.push_back(left_top + u_shift + v_shift);
    }
  }
  return points;
}

Vec3r
AreaLight::Illuminate(const HitRecord &hit_record, const Vec3r &view_vec,
                       Surface::Ptr scene) const
{
  // evaluate hit points material
  Vec3r black{0, 0, 0};

  // create a shadow ray to the point light and check for occlusion
  const auto hit_position = hit_record.GetPoint();

  std::vector<Vec3r> points = this->GeneratePoints();
  Real S = pow(len_, 2);
  int N = static_cast<int>(pow(round(sqrt(shadow_samples_)), 2));

  Vec3r color{0, 0, 0};

  // only process phong materials
  auto surface = hit_record.GetSurface();
  if (!surface)
    return black;
  auto phong_material = dynamic_pointer_cast<PhongMaterial>(surface->
                                                            GetMaterial());
  if (!phong_material)
    return black;

  for(auto & point: points) {
    Ray shadow_ray{hit_position, point - hit_position};
    HitRecord shadow_record;
    if (scene->Hit(shadow_ray, kEpsilon, 1, shadow_record)) {
      continue;
    }

    const Vec3r &normal = hit_record.GetNormal();
    Vec3r light_vec = point - hit_position;
    auto distance2 = light_vec.squaredNorm();
    light_vec.normalize();
    auto denominator = std::max(kEpsilon2, distance2);

    Real cos_alpha = normal_.dot(-light_vec);
    Real cos_theta = normal.dot(light_vec);

    // compute how much the material absorts light
    const Vec3r &attenuation = phong_material->Evaluate(hit_record, light_vec,
                                                      view_vec);
    
    Vec3r irradiance = intensity_ * fmax(0.0f, cos_theta) * fmax(0.0f, cos_alpha)/denominator;
    color += attenuation.cwiseProduct(irradiance);
  }

  color = color * S / N;
  return color;
}



}  // namespace core
}  // namespace olio
