#pragma once

#include "color.h"

struct Material {
  Color diffuse;
  float albedo;
  float specularAlbedo;
  float specularCoefficient;
  float reflectivity; // The reflectivity of the material
  float transparency; // The transparency of the material
  float refractionIndex;
};
