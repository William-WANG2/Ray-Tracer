#include "raytra_parser.h"
#include <fstream>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <spdlog/spdlog.h>
#include "core/geometry/surface.h"
#include "core/geometry/sphere.h"
#include "core/camera/camera.h"
#include "core/geometry/triangle.h"
#include "core/geometry/surface_list.h"
#include "core/light/light.h"
#include "core/material/phong_material.h"
#include "core/material/phong_dielectric.h"
#include "core/geometry/trimesh.h"
#include "map"
#include "core/texture/image_texture.h"

namespace olio {
namespace core {

using namespace std;
using boost::algorithm::trim;
namespace fs = boost::filesystem;

bool RaytraParser::ParseFile(const std::string &filename, Surface::Ptr &scene,
                             std::vector<Light::Ptr> &lights,
                             Camera::Ptr &camera, Vec2i &image_size)
{
  // get absoulte file path
  fs::path filepath(filename);
  if (!filepath.is_absolute())
    filepath = boost::filesystem::absolute(filepath,
      boost::filesystem::current_path());
  fs::path path_prefix(filepath.parent_path());

  // check if file exists
  if (!fs::exists(filepath)) {
    spdlog::error("RaytraParser::ParseFile: file {} does not exist", filename);
    return false;
  }

  // open file
  ifstream in(filename);
  if (!in) {
    spdlog::error("RaytraParser::ParseFile: could not open file {} "
                  "for reading", filename);
    return false;
  }
  int camera_count = 0;
  int ambient_count = 0;
  int light_count = 0;
  int material_count = 0;
  vector<Surface::Ptr> surfaces;
  map<int, Texture::Ptr> img_textures;

  // current material that's applied to the next read surface
  PhongMaterial::Ptr current_material;

  // parse file
  for (string line; getline(in, line);) {
    // skip comments and empty lines
    trim(line);
    if (line.size() && line.back() == '\n')
      line.pop_back();
    if (!line.size() || line[0] == '/')
      continue;

    // parse file
    char cmd {'\0'};
    istringstream iss(line);
    iss >> cmd;
    switch(cmd) {
    case '/': continue;
    case 's':
      {
        // sphere
        Real x, y, z, r;
        iss >> x >> y >> z >> r;
        auto sphere = Sphere::Create(Vec3r{x, y, z}, r);

        // set material
        if (!current_material) {
          spdlog::error("Invalid scene file: cannot find matching material "
                        "for surface: {}", line);
          return false;
        }
        sphere->SetMaterial(current_material);
        surfaces.push_back(sphere);
        break;
      }
    case 'c':
      {
        // camera
        Real x, y, z, vx, vy, vz, focal_length, viewport_width, viewport_height,
          pixels_width, pixels_height;
        iss >> x >> y >> z >> vx >> vy >> vz >> focal_length >> viewport_width
            >> viewport_height >> pixels_width >> pixels_height;
        Vec3r eye{x, y, z};
        Vec3r view_vec{vx, vy, vz};
        view_vec.normalize();
        Vec3r target = eye + view_vec;
        Vec3r up_vec{0, 1, 0};
        if (view_vec.isApprox(up_vec))
          up_vec = Vec3r{0, 0, 1};
        Real fovy = 2 * atan2(viewport_height * 0.5f, focal_length) * kRADtoDEG;

        // check viewport/image aspect ratios
        Real viewport_aspect = viewport_width / viewport_height;
        if (!isfinite(viewport_aspect) || viewport_aspect <= 0) {
          spdlog::error("Camera has bad viewport_aspect ratio: {}",
                        viewport_aspect);
          return false;
        }
        if (viewport_aspect > 20000) {
          spdlog::warn("Camera has very large viewport_aspect ratio: {}",
                       viewport_aspect);
        }
        Real image_aspect = pixels_width / pixels_height;
        if (fabs(viewport_aspect - image_aspect) > kEpsilon) {
          spdlog::warn("Camera viewport has a different aspect ratio than "
                       "output image (viewport_aspect: {} vs "
                       "image_aspect: {})", viewport_aspect, image_aspect);
          spdlog::warn("Output image width will be adjusted to match the "
                       "viewport aspect ratio");
        }

        // create camera
        camera = Camera::Create(eye, target, up_vec, fovy,viewport_aspect);
        image_size = Vec2i{static_cast<int>(pixels_width),
                           static_cast<int>(pixels_height)};
        ++camera_count;
        break;
      }
    case 'm':
      {
        // phong material
        Real dr, dg, db, sr, sg, sb, shininess, ir, ig, ib;
        iss >> dr >> dg >> db >> sr >> sg >> sb >> shininess >> ir >> ig >> ib;
        Vec3r ambient{fmax(0.01, dr), fmax(0.01, dg), fmax(0.01, db)};
        Vec3r diffuse{dr, dg, db};
        Vec3r specular{sr, sg, sb};
        Vec3r mirror{ir, ig, ib};
        current_material = PhongMaterial::Create(ambient, diffuse, specular,
                                                 shininess, mirror);
        ++material_count;
        break;
      }
      case 'w':
      {
        // triangle mesh
        std::string str_path;
        iss >> str_path;
        // get absoulte file path
        fs::path filepath(str_path);
        if (!filepath.is_absolute())
          filepath = path_prefix / filepath;

        auto tri_mesh = TriMesh::Create();
        if(!tri_mesh->Load(filepath)) {
          spdlog::error("Invalid mesh file: cannot load the mesh file: {}", str_path);
          return false;
        }
        tri_mesh->SetMaterial(current_material);
        surfaces.push_back(tri_mesh);
        break;
      }
    case 'd':
      {
        // phong dielectric material
        Real ior, dr, dg, db;
        iss >> ior >> dr >> dg >> db;
        Vec3r attenuation{dr, dg, db};
        current_material = PhongDielectric::Create(ior, attenuation);
        ++material_count;
        break;
      }
    case 't':
      {
        // triangle
        Real ax, ay, az, bx, by, bz, cx, cy, cz;
        iss >> ax >> ay >> az >> bx >> by >> bz >> cx >> cy >> cz;
        vector<Vec3r> points;
        points.reserve(3);
        points.push_back(Vec3r{ax, ay, az});
        points.push_back(Vec3r{bx, by, bz});
        points.push_back(Vec3r{cx, cy, cz});
        auto triangle = Triangle::Create(points);

        // set material
        if (!current_material) {
          spdlog::error("Invalid scene file: cannot find matching material "
                        "for surface: {}", line);
          return false;
        }
        triangle->SetMaterial(current_material);
        surfaces.push_back(triangle);
        break;
      }
    case 'l':
      {
        char light_type;
        iss >> light_type;

        if (light_type == 'p') {
          // point light
          Real x, y, z, r, g, b;
          iss >> x >> y >> z >> r >> g >> b;
          Vec3r position{x, y, z};
          Vec3r intensity{r, g, b};
          auto light = PointLight::Create(position, intensity);
          lights.push_back(light);
          ++light_count;
        } else if (light_type == 'a') {
          // ambient light
          Real r, g, b;
          iss >> r >> g >> b;
          auto ambient_light = AmbientLight::Create(Vec3r{r, g, b});
          lights.push_back(ambient_light);
          ++ambient_count;
        } else if (light_type == 's') {
          Real x, y, z, nx, ny, nz, ux, uy, uz, len, r, g, b;
          iss >> x >> y >> z >> nx >> ny >> nz >> ux >> uy >> uz >> len >> r >> g >> b;
          auto area_light = AreaLight::Create(Vec3r{x,y,z}, Vec3r{nx, ny, nz}, Vec3r{ux, uy, uz}, len, Vec3r{r, g, b});
          lights.push_back(area_light);
          ++light_count;
        }
        break;
      }
      case 'n':
        {
          // textured Phong material
          int ti;
          Real dr, dg, db, sr, sg, sb, shininess, ir, ig, ib;
          iss >> ti >> dr >> dg >> db >> sr >> sg >> sb >> shininess >> ir >> ig >> ib;
          Vec3r ambient{fmax(0.01, dr), fmax(0.01, dg), fmax(0.01, db)};
          if(img_textures.find(ti) == img_textures.end()) {
            spdlog::error("Invalid image texture id {} which is not defined!", ti);
            return false;
          }
          Texture::Ptr diffuse = img_textures[ti];
          Vec3r specular{sr, sg, sb};
          Vec3r mirror{ir, ig, ib};
          current_material = PhongMaterial::Create(ambient, diffuse, specular,
                                                  shininess, mirror);
          ++material_count;
        break;
        }
      case 'i':
       {
         // image texture with id “ti” and path “path”.
         int ti, flipx, flipy;
         string path_str;
         iss >> ti >> flipx >> flipy >> path_str;
         fs::path filepath(path_str);
         if (!filepath.is_absolute())
          filepath = path_prefix / filepath;
         Texture::Ptr img_texture = ImageTexture::Create(filepath, flipx, flipy); 
         if(img_textures.find(ti)!=img_textures.end()) {
            spdlog::error("image texture id {} which is already defined!", ti);
            return false;
         }
         img_textures[ti] = img_texture;
         break;
       }
    default:
      continue;
    }
  }

  // close input file
  in.close();

  if (camera_count != 1) {
    spdlog::error("Parse error: scene file should contain only one camera");
    return false;
  }

  if (ambient_count > 1) {
    spdlog::error("Parse error: scene file should contain only one ambient "
                  "light");
    return false;
  }

  if (surfaces.size() < 1)
    spdlog::warn("Scene file does not contain any surfaces");

  scene = SurfaceList::Create(surfaces);
  spdlog::info("Read {} surface(s), {} material(s), & {} point light(s) ",
               surfaces.size(), material_count, light_count);
  return true;
}

}  // namespace core
}  // namespace raytra