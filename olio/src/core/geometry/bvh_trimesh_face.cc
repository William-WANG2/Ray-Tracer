#include "core/geometry/bvh_trimesh_face.h"
#include "core/geometry/triangle.h"
#include "core/ray.h"

namespace olio {
namespace core {
    BVHTriMeshFace::BVHTriMeshFace(TriMesh::Ptr mesh, TriMesh::FaceHandle fh) {
        mesh_ = mesh;
        fh_ = fh;
    }
    AABB BVHTriMeshFace::GetBoundingBox(bool force_recompute) {
        // if bound is clean, just return existing bbox_
        if (!force_recompute && !IsBoundDirty())
            return bbox_;
        // compute triangle mesh bbox
        bbox_.Reset();
        for(auto fvit = mesh_->fv_iter(fh_); fvit.is_valid(); ++fvit) {
            bbox_.ExpandBy(mesh_->point(*fvit));
        }
        bound_dirty_ = false;
        return bbox_;
    }

    bool BVHTriMeshFace::Hit(const Ray &ray, Real tmin, Real tmax, HitRecord &hit_record) {
        if(!bbox_.Hit(ray, tmin, tmax)) {
            return false;
        }
        std::vector<TriMesh::VertexHandle> points;
        for(auto fvit = mesh_->fv_iter(fh_); fvit.is_valid(); ++fvit) {
            points.push_back(*fvit);
        }

        Real ray_t{0};
        Vec2r uv;
        if (!Triangle::RayTriangleHit(mesh_->point(points[0]), mesh_->point(points[1]), mesh_->point(points[2]), ray, tmin, tmax, ray_t, uv))
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
            normal = normal + barycentric[i] * mesh_->normal(points[i]);
        }
        hit_record.SetNormal(ray, normal.normalized());
        hit_record.SetSurface(mesh_->GetPtr());

        FaceGeoUV face_geo_uv;
        face_geo_uv.SetFaceId(fh_.idx());
        face_geo_uv.SetUV(uv);

        if(mesh_->has_vertex_texcoords2D()) {
            Vec2r texture_coordinates{0, 0};
            for(ulong i=0; i<3; i++) {
            texture_coordinates[0] += barycentric[i]*mesh_->texcoord2D(points[i])[0];
            texture_coordinates[1] += barycentric[i]*mesh_->texcoord2D(points[i])[1];
            }
            face_geo_uv.SetGlobalUV(texture_coordinates);
        }
        else {
            face_geo_uv.SetGlobalUV(Vec2r{-1, -1});
        }

        hit_record.SetFaceGeoUV(face_geo_uv);
        return true;
    }
}  // namespace core
}  // namespace olio
