#pragma once

#include "re.h"
#include <array>
#include <vector>
#include <map>

namespace sb
{

	struct WfFace
	{
		std::vector<re::Vector3> Vertices;
		std::vector<re::Vector3> Normals;
		std::vector<re::Vector2> TexCoords;
	};


	using WfGroup = std::vector<WfFace>;
	using WfData = std::map<std::string, WfGroup>;

	WfData LoadWavefront(const std::string fileName);

}

