// ======================================================================
// Olio: Simple renderer
// Copyright (C) 2022 by Hadi Fadaifard
//
// Author: Hadi Fadaifard, 2022
// ======================================================================

//! \file       main.cc
//! \brief      rtbasic cli main.cc file
//! \author     Hadi Fadaifard, 2022

#include <vector>
#include <iostream>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>

#include "core/types.h"
#include "core/node.h"
#include "core/camera/camera.h"
#include "core/geometry/surface.h"
#include "core/parser/raytra_parser.h"
#include "core/renderer/raytracer.h"
#include "core/utils/segfault_handler.h"
#include "core/light/light.h"
#include "core/geometry/surface_list.h"
#include "core/geometry/bvh_node.h"

using namespace olio::core;
using namespace std;
namespace po = boost::program_options;

bool ParseArguments(int argc, char **argv, std::string *input_scene_name,
                    std::string *output_name, uint *samples_per_pixel, uint * shadow_samples) {
  po::options_description desc("options");
  try {
    desc.add_options()
      ("help,h", "print usage")
      ("input_scene,s",
       po::value             (input_scene_name)->required(),
       "Input scene file")
      ("output,o",
       po::value             (output_name)->required(),
       "Output name")
       ("samples_per_pixel,a",
       po::value              (samples_per_pixel)->default_value(1),
       "Samples per pixel")
       ("shadow_samples,d",
       po::value             (shadow_samples)->required(),
       "Shadow Per Samples");

    // parse arguments
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    if (vm.count("help")) {
      cout << desc << endl;
      return false;
    }
    po::notify(vm);
  } catch(std::exception &e) {
    cout << desc << endl;
    spdlog::error("{}", e.what());
    return false;
  } catch(...) {
    cout << desc << endl;
    spdlog::error("Invalid arguments");
    return false;
  }
  return true;
}


int
main(int argc, char **argv)
{
  utils::InstallSegfaultHandler();
  srand(123543);

  // parse command line arguments
  string input_scene_name, output_name;
  uint samples_per_pixel;
  uint shadow_samples;
  if (!ParseArguments(argc, argv, &input_scene_name, &output_name, &samples_per_pixel, &shadow_samples))
    return -1;

  // parse and render raytra scene
  Vec2i image_size;
  Surface::Ptr scene;
  vector<Light::Ptr> lights;
  Camera::Ptr camera;
  if (!RaytraParser::ParseFile(input_scene_name, scene, lights, camera,
      image_size) || !scene || !camera || image_size[0] <= 0 ||
      image_size[1] <= 0) {
    spdlog::error("Failed to parse scene file.");
    return -1;
  }

  auto scenelist_ptr = dynamic_pointer_cast<SurfaceList>(scene);
  if(!scenelist_ptr) {
    spdlog::error("Failed to convert to SurfaceList class.");
    return -1;
  }

  for(auto & light: lights) {
    AreaLight::Ptr area_light = dynamic_pointer_cast<AreaLight>(light);
    if(area_light) {
      area_light->SetShadowSamples(shadow_samples);
    }
  }
  
  auto bvh_tree = BVHNode::BuildBVH(scenelist_ptr->GetSurfaces(), string{"Scene Objects"});

  // render scene
  RayTracer rt;
  rt.SetNumSamplesPerPixel(samples_per_pixel);
  rt.SetImageHeight(static_cast<uint>(image_size[1]));
  rt.Render(bvh_tree, lights, camera);

  // save rendered image to file
  rt.WriteImage(output_name, 2);
  return 0;
}
