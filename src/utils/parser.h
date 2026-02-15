#pragma once
#define GLM_FORCE_RADIANS

#include "object.h"
#include "physics_engine.h"
#include "json.hpp"
#include <glm/glm.hpp>

nlohmann::json openjson(std::string path);
glm::vec3 toVec3 (nlohmann::json data);
Eigen::Vector3d toVector3d(nlohmann::json data);
Transform toTransform(nlohmann::json data);
std::shared_ptr<Material> toMaterial(nlohmann::json data);

std::vector<std::shared_ptr<Object>> parseObjects(std::string resourcePath, std::string jsonPath);
SimParameters parseParameters(std::string jsonPath);