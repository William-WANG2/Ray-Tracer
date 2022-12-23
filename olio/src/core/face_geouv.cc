#include "core/types.h"
#include "core/face_geouv.h"

namespace olio {
namespace core {

    FaceGeoUV::FaceGeoUV(int face_id, const Vec2r & uv, const Vec2r & global_uv) {
        face_id_ = face_id;
        uv_ = uv;
        global_uv_ = global_uv;
    }

}  // namespace core
}  // namespace olio