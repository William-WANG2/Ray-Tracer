//! \file       bvh_node.cc
//! \brief      BVHNode class

#include <spdlog/spdlog.h>
#include "core/ray.h"
#include "core/material/material.h"
#include "core/geometry/bvh_node.h"

namespace olio {
namespace core {

using namespace std;

BVHNode::BVHNode(const std::string &name) :
  Surface{}
{
  name_ = name.size() ? name : "BVHNode";
}


AABB
BVHNode::GetBoundingBox(bool force_recompute)
{
  // if bound is clean, just return existing bbox_
  if (!force_recompute && !IsBoundDirty())
    return bbox_;

  bbox_.Reset();
  if (left_)
    bbox_.ExpandBy(left_->GetBoundingBox(force_recompute));
  if (right_)
    bbox_.ExpandBy(right_->GetBoundingBox(force_recompute));
  // spdlog::info("{}: {}", GetName(), bbox_);
  bound_dirty_ = false;
  return bbox_;
}


bool
BVHNode::Hit(const Ray &ray, Real tmin, Real tmax, HitRecord &hit_record)
{
  // ======================================================================
  // *** Homework: Implement function
  // ======================================================================
  // ***** START OF YOUR CODE (DO NOT DELETE/MODIFY THIS LINE) *****
  if (bbox_.Hit(ray, tmin, tmax)) {
    // if we are a leaf: try to intersect the surface - record hit and
    // adjust tmax if necessary, and return true or false
    // if(right_ == nullptr) {
    //   if(left_->GetBoundingBox().Hit(ray, tmin, tmax)) {
    //     return left_->Hit(ray, tmin, tmax, hit_record);
    //   }
    //   return false;
    // }
    // // OTHERWISE:
    // else {


    bool is_hit = false;
    if(left_ != nullptr && left_->Hit(ray, tmin, tmax, hit_record)) {
      tmax = hit_record.GetRayT();
      is_hit = true;
    }
    if(right_ != nullptr && right_->Hit(ray, tmin, tmax, hit_record)) {
      is_hit = true;
    }
    return is_hit;

      
    // }
  }
  else {
    return false;
  }
  return false;  //!< remove this line and add your own code
  // ***** END OF YOUR CODE (DO NOT DELETE/MODIFY THIS LINE) *****
}


BVHNode::Ptr
BVHNode::BuildBVH(std::vector<Surface::Ptr> surfaces, const string &name)
{
  spdlog::info("Building BVH ({})", name);

  // error checking
  auto surface_count = surfaces.size();
  if (!surface_count)
    return nullptr;

  // make sure we have valid bboxes for surfaces
  for (auto surface : surfaces) {
    if (surface)
      surface->GetBoundingBox();
  }

  // build bvh
  uint split_axis = 0;
  auto bvh_node = BuildBVH(surfaces, 0, surface_count, split_axis, name);

  // compute bboxes
  if (bvh_node)
    bvh_node->GetBoundingBox();

  spdlog::info("Done building BVH ({})", name);
  return bvh_node;
}


BVHNode::Ptr
BVHNode::BuildBVH(std::vector<Surface::Ptr> &surfaces, size_t start,
                  size_t end, uint split_axis, const std::string &name)
{
  // ======================================================================
  // *** Homework: Implement function
  // ======================================================================
  // ***** START OF YOUR CODE (DO NOT DELETE/MODIFY THIS LINE) *****
  BVHNode::Ptr bvh_node = BVHNode::Create();
  size_t N = end - start;
  if (N == 1) {
    bvh_node->left_ = surfaces[start];
    bvh_node->right_ = nullptr;
  }
  else if(N == 2) {
    bvh_node->left_ = surfaces[start];
    bvh_node->right_ = surfaces[start+1];
  }
  else {
    // sort
    sort(&surfaces[start], &surfaces[end], [split_axis](Surface::Ptr surface_1, Surface::Ptr surface_2)
    {
        if(!surface_1 || !surface_2) {
          return false;
        }
        Real box_1_along_axis = surface_1->GetBoundingBox().GetMin()[split_axis];
        Real box_2_along_axis = surface_2->GetBoundingBox().GetMin()[split_axis];

        return (box_1_along_axis < box_2_along_axis);
    });
    size_t mid = (start+end)/2;
    bvh_node->left_ = BuildBVH(surfaces, start, mid, (split_axis+1)%3);
    bvh_node->right_ = BuildBVH(surfaces, mid, end, (split_axis+1)%3);
  }
  return bvh_node;
  // return nullptr;  //!< remove this line and add your own code
  // ***** END OF YOUR CODE (DO NOT DELETE/MODIFY THIS LINE) *****
}

}  // namespace core
}  // namespace olio
