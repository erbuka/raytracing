#include "WavefrontLoader.h"

#include <fstream>
#include <string>
#include <sstream>

namespace sb
{


	WfData LoadWavefront(const std::string fileName)
	{
		static auto parseFaceVertex = [](const std::string& str) -> std::array<int, 3> 
		{
			std::array<int, 3> result;

			size_t pos0 = std::string::npos;
			size_t pos1 = std::string::npos;

			pos0 = str.find("/", 0);

			if (pos0 != std::string::npos)
				pos1 = str.find("/", pos0 + 1);

			if (pos0 == std::string::npos && pos1 == std::string::npos)
				result = { std::atoi(str.c_str()), 0, 0 };
			else if (pos1 == std::string::npos)
				result = { std::atoi(str.substr(0, pos0).c_str()), std::atoi(str.substr(pos0 + 1).c_str()), 0 };
			else if(pos0 + 1 == pos1)
				result = { std::atoi(str.substr(0, pos0).c_str()), 0, std::atoi(str.substr(pos1 + 1).c_str()) };
			else
				result = { std::atoi(str.substr(0, pos0).c_str()), std::atoi(str.substr(pos0 + 1, pos1).c_str()), std::atoi(str.substr(pos1 + 1).c_str()) };

			for (auto &i : result)
				i--;

			return result;

		};

		WfData result;
		std::string currentGroup = "default";

		std::vector<re::Vector3> vertices, normals;
		std::vector<re::Vector2> texCoords;

		std::ifstream is;
		is.open(fileName);

		std::string line;

		while (is.good())
		{
			re::real a, b, c;
			std::getline(is, line);
			std::stringstream ss(line);
			std::string token;

			ss >> token;
			
			if (token == "v")
			{
				ss >> a >> b >> c;
				vertices.push_back({ a, b, c });
			}
			else if (token == "vt")
			{
				ss >> a >> b;
				texCoords.push_back({ a, b });
			}
			else if (token == "vn")
			{
				ss >> a >> b >> c;
				normals.push_back({ a, b, c });
			}
			else if (token == "g")
			{
				ss >> currentGroup;
			}
			else if (token == "f")
			{
				WfFace face;
				while (ss.good())
				{
					std::string faceStr;
					ss >> faceStr;

					auto [v, vt, vn] = parseFaceVertex(faceStr);

					face.Vertices.push_back(vertices[v]);
					face.TexCoords.push_back(vt != -1 ? texCoords[vt] : re::Vector2::Zero);
					face.Normals.push_back(vn != -1 ? normals[vn] : re::Vector3::Zero);
				}

				result[currentGroup].push_back(face);

			}

		}
	
		return result;
	}
}