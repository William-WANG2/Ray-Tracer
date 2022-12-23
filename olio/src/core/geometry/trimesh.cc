//! \file       trimesh.cc
//! \brief      TriMesh class

#include "core/geometry/trimesh.h"
#include <spdlog/spdlog.h>
#include "core/ray.h"
#include "core/material/material.h"
#include "core/geometry/triangle.h"
#include "core/face_geouv.h"
#include "core/geometry/bvh_trimesh_face.h"
#include <vector>

namespace olio {
namespace core {
//Vec2r &copyOfUV={0,0};
using namespace std;
namespace fs=boost::filesystem;
TriMesh::TriMesh(const std::string &name) :
  OMTriMesh{},
  Surface{name}
{
  name_ = name.size() ? name : "TriMesh";
}


// ======================================================================
// *** Homework: Implement unimplemented TriMesh functions here
// ======================================================================
// ***** START OF YOUR CODE (DO NOT DELETE/MODIFY THIS LINE) *****
bool TriMesh::Hit(const Ray &ray, Real tmin, Real tmax,HitRecord &hit_record){
  bool had_hit = false;
  if(bvh_ == nullptr) {
    if(!GetBoundingBox().Hit(ray, tmin, tmax)) {
            return false;
    }
    for (auto fit = this->faces_begin(); fit != this->faces_end(); ++fit) {
      if(RayFaceHit(*fit, ray, tmin, tmax, hit_record)) {
        tmax = (hit_record.GetRayT() < tmax)?hit_record.GetRayT():tmax;
        had_hit = true;
      }
    }
  }
  else {
    if(bvh_->Hit(ray, tmin, tmax, hit_record)) {
      had_hit = true;
    }
  }
  return had_hit;
}
bool TriMesh::RayFaceHit(TriMesh::FaceHandle fh, const Ray &ray, Real tmin, Real tmax, HitRecord &hit_record){
  std::vector<TriMesh::VertexHandle> points;

  for(auto fvit = this->fv_iter(fh); fvit.is_valid(); ++fvit) {
    points.push_back(*fvit);
  }

  Real ray_t{0};
  Vec2r uv;
  if (!Triangle::RayTriangleHit(this->point(points[0]), this->point(points[1]), this->point(points[2]), ray,
                      tmin, tmax, ray_t, uv))
    return false;

  // fill hit_record
  const Vec3r &hit_point = ray.At(ray_t);
  hit_record.SetRayT(ray_t);
  hit_record.SetPoint(hit_point);

  // compute normal at the hitting point
  Real barycentric[3];
  barycentric[0] = 1-uv[0]-uv[1];
  barycentric[1] = uv[0];
  barycentric[2] = uv[1];
  Vec3r normal{0, 0, 0};
  for(ulong i=0; i<3; i++) {
    normal = normal + barycentric[i] * this->normal(points[i]);
  }
  hit_record.SetNormal(ray, normal.normalized());
  hit_record.SetSurface(GetPtr());

  FaceGeoUV face_geo_uv;
  face_geo_uv.SetFaceId(fh.idx());
  face_geo_uv.SetUV(uv);

  if(this->has_vertex_texcoords2D()) {
    Vec2r texture_coordinates{0, 0};
    for(ulong i=0; i<3; i++) {
      texture_coordinates[0] += barycentric[i]*this->texcoord2D(points[i])[0];
      texture_coordinates[1] += barycentric[i]*this->texcoord2D(points[i])[1];
    }
    face_geo_uv.SetGlobalUV(texture_coordinates);
  }
  else {
    face_geo_uv.SetGlobalUV(Vec2r{-1, -1});
  }

  hit_record.SetFaceGeoUV(face_geo_uv);
  return true;
}

bool TriMesh::Load(const boost::filesystem::path &filepath) {
  this->request_face_normals();
  this->request_vertex_normals();
  this->request_vertex_texcoords2D();

  OpenMesh::IO::Options opts{OpenMesh::IO::Options::FaceNormal | OpenMesh::IO::Options::VertexNormal | OpenMesh::IO::Options::VertexTexCoord};
  
  if (!OpenMesh::IO::read_mesh(*this, filepath.string(), opts)) {
    spdlog::error("could not load mesh from {}", filepath.string());
    return false; 
  }

  bool status = true;
  if (!opts.check(OpenMesh::IO::Options::FaceNormal))
    status = this->ComputeFaceNormals();
  if (!opts.check(OpenMesh::IO::Options::VertexNormal))
    status = this->ComputeVertexNormals();

  // delete vertex texcoord2d attribute if file did not have them
  if (opts.check(OpenMesh::IO::Options::VertexTexCoord))
    spdlog::info("mesh has texture coordinates");
  else
    release_vertex_texcoords2D();

  // build BVH tree
  BuildBVH();

  return status;
}

bool TriMesh::Save(const boost::filesystem::path &filepath, OpenMesh::IO::Options opts){
  if (!OpenMesh::IO::write_mesh(*this, filepath.string(), opts)) 
  {
    spdlog::error("could not write mesh to {}", filepath.string());
    return false;
  }
  return true;
}
AABB TriMesh::GetBoundingBox(bool force_recompute){
  if (!force_recompute && !IsBoundDirty())
    return bbox_;
  bbox_.Reset();
  for (auto vit = vertices_begin(); vit != vertices_end(); ++vit) {
    bbox_.ExpandBy(this->point(*vit));
  }
  bound_dirty_ = false;
  return bbox_;  
}


Vec3r TriMesh::FaceNormal(TriMesh::FaceHandle fh, bool is_normalize) {
  std::vector<Vec3r> points;

  for(auto fvit = this->fv_iter(fh); fvit.is_valid(); ++fvit) {
    points.push_back(this->point(*fvit));
  }

  Vec3r normal = (points[1] - points[0]).cross(points[2] - points[0]);
  if(is_normalize) {
    normal.normalize();
  }

  return normal;
}

bool TriMesh::ComputeFaceNormals() {
  this->request_face_normals();
  if (!this->has_face_normals())
  {
    spdlog::error("ERROR: Standard face property 'Normals' not available!\n");
    return false;
  }
  for(auto fit = this->faces_begin(); fit != this->faces_end(); ++fit) {
    const Vec3r face_normal = FaceNormal(*fit);
    this->set_normal(*fit, face_normal);
  }
  return true;
}

Vec3r TriMesh::VertexNormal(TriMesh::VertexHandle vh, bool is_normalize) {
  int cnt = 0;
  Vec3r normal{0, 0, 0};

  for(auto vfit = this->vf_iter(vh); vfit.is_valid(); ++vfit) {
    normal+=this->normal(*vfit);
    cnt++;
  }

  normal/=cnt;
  if(is_normalize) {
    normal.normalize();
  }

  return normal;
}

bool TriMesh::ComputeVertexNormals() {
  this->request_vertex_normals();
  if (!this->has_vertex_normals())
  {
    spdlog::error("ERROR: Standard normal property 'Normals' not available!\n");
    return false;
  }
  for(auto vit = this->vertices_begin(); vit != this->vertices_end(); ++vit) {
    const Vec3r vertex_normal = VertexNormal(*vit);
    this->set_normal(*vit, vertex_normal);
  }
  return true;
}

void TriMesh::BuildBVH() {
  std::vector<Surface::Ptr> mesh_faces;
  TriMesh::Ptr mesh = this->GetPtr();
  for(auto fit = this->faces_begin(); fit != this->faces_end(); ++fit) {
    BVHTriMeshFace::Ptr mesh_face = make_shared<BVHTriMeshFace>(mesh, *fit);
    mesh_faces.push_back(mesh_face);
  }
  bvh_ = BVHNode::BuildBVH(mesh_faces, string{"Triangle Mesh"});
}
// ***** END OF YOUR CODE (DO NOT DELETE/MODIFY THIS LINE) *****

}  // namespace core
}  // namespace olio