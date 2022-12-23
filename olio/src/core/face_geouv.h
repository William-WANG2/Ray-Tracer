#pragma once

#include "core/types.h"

namespace olio {
namespace core {

class FaceGeoUV {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  FaceGeoUV() = default;
  FaceGeoUV(int face_id, const Vec2r & uv, const Vec2r & global_uv);

  inline int GetFaceId() {
    return face_id_;
  }
  inline Vec2r GetUV() {
    return uv_;
  }
  inline Vec2r GetGlobalUV() {
    return global_uv_;
  }
  inline void SetFaceId(int face_id) {
    face_id_ = face_id;
  }
  inline void SetUV(const Vec2r & uv) {
    uv_ = uv;
  }
  inline void SetGlobalUV(const Vec2r & global_uv) {
    global_uv_ = global_uv;
  }
protected:
  int face_id_;
  Vec2r uv_;
  Vec2r global_uv_;
};

}  // namespace core
}  // namespace olio