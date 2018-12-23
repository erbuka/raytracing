#include "SFML/Graphics.hpp"
#include "SFML/Window.hpp"

#include "Raytracer.h"
#include "Scene.h"
#include "Material.h"
#include "Marble.h"
#include "Perlin.h"
#include "Worley.h"

#include <chrono>
#include <memory>
#include <sstream>
#include <array>

template<typename T>
constexpr T CWidth() { return (T)1280; }

template<typename T>
constexpr T CHeight() { return (T)768; }

template<typename T>
constexpr T CFastWidth() { return CWidth<T>() / 4; }

template<typename T>
constexpr T CFastHeight() { return CHeight<T>() / 4; }


class Test1 {
public:

	Test1() {}
	~Test1() {

	}

	void Start() {
		m_Window.create(sf::VideoMode(CWidth<int>(), CHeight<int>()), "Test");

		m_Raytracer = std::shared_ptr<re::Raytracer>(new re::Raytracer(CWidth<int>(), CHeight<int>()));
		m_Raycaster = std::shared_ptr<re::DebugRaycaster>(new re::DebugRaycaster(CFastWidth<int>(), CFastHeight<int>()));
		
		CreateMaterials();
		m_Scene = std::shared_ptr<re::Scene>(CreateScene());

		m_Texture.create(CWidth<int>(), CHeight<int>());
		m_FastTexture.create(CFastWidth<int>(), CFastHeight<int>());

		m_RenderRect.setSize({ CWidth<float>(), CHeight<float>() });

		if (!m_Font.loadFromFile("res/arial.ttf"))
		{
			return;
		}

		while (m_Window.isOpen())
		{
			sf::Event event;
			while (m_Window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed) 
				{
					m_Window.close();
				}
				else if (event.type == sf::Event::KeyReleased)
				{
					switch (event.key.code)
					{
					case sf::Keyboard::R:
						m_MaxRecursion = (m_MaxRecursion + 1) % 4;
						break;
					case sf::Keyboard::T:
						m_SuperSampling = !m_SuperSampling;
						break;
					case sf::Keyboard::H:
						m_HideInfo = !m_HideInfo;
						break;
					case sf::Keyboard::G:
						m_GroundMaterial = (m_GroundMaterial + 1) % m_GroundMaterials.size();
						break;
					case sf::Keyboard::Y:
						m_SkyBox = (m_SkyBox + 1) % m_SkyBoxes.size();
						break;
					case sf::Keyboard::L:
						m_LampsSwitch = !m_LampsSwitch;
						break;
					case sf::Keyboard::Return:
						m_Render = !m_Render;
						break;
					}
				}
			}

			m_Window.clear();
				
			Update();
			Render();

			m_Window.display();
		}
	}

private:

	std::shared_ptr<re::Material> CreateMarble(std::shared_ptr<re::Material> dark, std::shared_ptr<re::Material> bright)
	{
		return std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Marble>(new re::Marble(2.0f, 4.0f, 4.0f)), dark, bright));
	}

	std::shared_ptr<re::Material> CreatePerlin(std::shared_ptr<re::Material> first, std::shared_ptr<re::Material> second)
	{
		return std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Perlin>(new re::Perlin(1.0f)), first, second));
	}

	std::shared_ptr<re::Material> CreateWorley(std::shared_ptr<re::Material> base, std::shared_ptr<re::Material> feats)
	{
		return std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Worley>(new re::Worley(2.0f, 10.0f)), base, feats));
	}

	void CreateMaterials()
	{
		m_Materials.insert({ "red", std::shared_ptr<re::Material>(new re::UniformMaterial(0xf44336, 0.6f)) });
		m_Materials.insert({ "green", std::shared_ptr<re::Material>(new re::UniformMaterial(0x4CAF50, 0.6f)) });
		m_Materials.insert({ "blue", std::shared_ptr<re::Material>(new re::UniformMaterial(0x2196F3, 0.6f)) });
		m_Materials.insert({ "dark_red", std::shared_ptr<re::Material>(new re::UniformMaterial(0x8d1007, 0.9f)) });
		m_Materials.insert({ "dark_green", std::shared_ptr<re::Material>(new re::UniformMaterial(0x265728, 0.9f)) });
		m_Materials.insert({ "dark_blue", std::shared_ptr<re::Material>(new re::UniformMaterial(0x074b83, 0.9f)) });
		m_Materials.insert({ "glass_black", std::shared_ptr<re::Material>(new re::UniformMaterial(0x000000, 0.8f)) });
		m_Materials.insert({ "glass_white", std::shared_ptr<re::Material>(new re::UniformMaterial(0xffffff, 0.4f)) });
		m_Materials.insert({ "mirror", std::shared_ptr<re::Material>(new re::UniformMaterial(0xffffff, 0.0f)) });


		m_GroundMaterials.push_back(std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Noise>(new re::CheckerBoard(16.0f)), m_Materials["glass_black"], m_Materials["glass_white"])));
		m_GroundMaterials.push_back(std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Noise>(new re::Marble(64, 32)), m_Materials["glass_black"], m_Materials["glass_white"])));
		m_GroundMaterials.push_back(std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Noise>(new re::Worley(64, 32)), m_Materials["glass_black"], m_Materials["glass_white"])));

		m_SpheresMaterials.push_back(m_Materials["red"]);
		m_SpheresMaterials.push_back(m_Materials["green"]);
		m_SpheresMaterials.push_back(m_Materials["blue"]);
		m_SpheresMaterials.push_back(CreateMarble(m_Materials["dark_red"], m_Materials["red"]));
		m_SpheresMaterials.push_back(CreateMarble(m_Materials["dark_green"], m_Materials["green"]));
		m_SpheresMaterials.push_back(CreateMarble(m_Materials["dark_blue"], m_Materials["blue"]));
		m_SpheresMaterials.push_back(CreateWorley(m_Materials["red"], m_Materials["mirror"]));
		m_SpheresMaterials.push_back(CreateWorley(m_Materials["green"], m_Materials["mirror"]));
		m_SpheresMaterials.push_back(CreateWorley(m_Materials["blue"], m_Materials["mirror"]));
	}

	void Update()
	{
		static auto now = std::chrono::high_resolution_clock::now();
		static auto before = std::chrono::high_resolution_clock::now();
		bool fast = false;

		now = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> delta = now - before;
		before = now;

		float dt = delta.count();

		// Camera control
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		{
			m_CameraAngle.Y += 1.0f * dt;
			fast = true;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
		{
			m_CameraAngle.Y -= 1.0f * dt;
			fast = true;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
		{
			m_CameraAngle.X += 1.0f * dt;
			fast = true;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
		{
			m_CameraAngle.X -= 1.0f * dt;
			fast = true;
		}

		m_Scene->LookDirection =
			re::Vector3(std::cosf(m_CameraAngle.Y), -std::sinf(m_CameraAngle.X), std::sinf(m_CameraAngle.Y)).Normalized();

		// Arrow keys movement

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
		{
			m_Scene->CameraPosition = m_Scene->CameraPosition + m_Scene->LookDirection * CMovementSpeed * dt;
			fast = true;
		}
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
		{
			m_Scene->CameraPosition = m_Scene->CameraPosition - m_Scene->LookDirection * CMovementSpeed * dt;
			fast = true;
		}

		m_Ground->Material = m_GroundMaterials[m_GroundMaterial].get();
		m_Scene->Background = m_SkyBoxes[m_SkyBox].get();

		m_Scene->Lights.clear();

		m_Scene->Lights.push_back(m_AmbientLights[m_SkyBox].get());
		m_Scene->Lights.push_back(m_DirectionalLights[m_SkyBox].get());

		if (m_LampsSwitch)
		{
			for (auto &lamp : m_Lamps)
			{
				m_Scene->Lights.push_back(lamp.get());
			}
		}

	}

	void RenderText(int line, const std::string str)
	{
		static constexpr int FontSize = 16;
		sf::Text text;
		text.setString(str);
		text.setFont(m_Font);
		text.setFillColor(sf::Color::White);
		text.setCharacterSize(FontSize);
		text.setPosition({ (float)FontSize, line * FontSize * 1.1f });
		m_Window.draw(text);
	}

	void Render()
	{
		std::stringstream ss;
		unsigned int * pixels;
		// Draw Scene
		m_Raytracer->MaxRecursion = m_MaxRecursion;

		m_Raycaster->SuperSampling = m_Raytracer->SuperSampling = m_SuperSampling;
		
		if (!m_Render)
		{
			pixels = m_Raycaster->Render(m_Scene.get());
			m_FastTexture.update((const sf::Uint8*)pixels);
			m_RenderRect.setTexture(&m_FastTexture, true);
		}
		else
		{
			pixels = m_Raytracer->Render(m_Scene.get());
			m_Texture.update((const sf::Uint8*)pixels);
			m_RenderRect.setTexture(&m_Texture, true);
		}

		m_Window.draw(m_RenderRect);

		// Draw info
		if (!m_HideInfo)
		{
			ss.str(""), ss << "(WASD) Move camera - (UP/DOWN) Move forward/backward - (ENTER) Render Scene";
			RenderText(1, ss.str());

			ss.str(""), ss << "(T) SuperSampling: " << (m_SuperSampling ? "Yes" : "No");
			RenderText(2, ss.str());

			ss.str(""), ss << "(R) Max Recursion: " << m_MaxRecursion;
			RenderText(3, ss.str());

			ss.str(""), ss << "(G) Ground material: " << m_GroundMaterial;
			RenderText(4, ss.str());

			ss.str(""), ss << "(Y) Sky: " << m_SkyBox;
			RenderText(5, ss.str());

			ss.str(""), ss << "(L) Lamps: " << (m_LampsSwitch ? "On" : "Off");
			RenderText(6, ss.str());

			ss.str(""), ss << "(H) Hide info";
			RenderText(10, ss.str());
		}

	}

	re::Scene * CreateScene()
	{
		re::Scene * m_Scene = new re::Scene();

		m_Scene->CameraPosition = { 0, 1, 0 };

		static auto makeDirectionalLight = [](re::Color color, re::Vector3 direction)
		{
			re::Light * directional = new re::Light();
			directional->Color = color;
			directional->Direction = direction.Normalized();
			directional->Type = re::LightType::Directional;
			return directional;
		};


		static auto makeAmbientLight = [](re::Color color)
		{
			re::Light * ambient = new re::Light();
			ambient->Color = color;
			ambient->Type = re::LightType::Ambient;
			return ambient;
		};


		{
			m_SkyBoxes.push_back(std::shared_ptr<re::Background>(new re::SkyBox(0x009CE5FF, 0x00C3FAFF)));
			m_SkyBoxes.push_back(std::shared_ptr<re::Background>(new re::SkyBox(0x0005061c, 0x000c0f47)));
			
			m_AmbientLights.push_back(std::shared_ptr<re::Light>(makeAmbientLight(0x222222)));
			m_AmbientLights.push_back(std::shared_ptr<re::Light>(makeAmbientLight(0)));

			m_DirectionalLights.push_back(std::shared_ptr<re::Light>(makeDirectionalLight(0x00fff4d6, { 1, 1, -1 })));
			m_DirectionalLights.push_back(std::shared_ptr<re::Light>(makeDirectionalLight(0x00353532, { 1, 1, -1 } )));

		}

		{
			int sx = 3;
			int sz = m_SpheresMaterials.size() / 3;

			for (int x = 0; x < sx; x++)
			{
				for (int z = 0; z < sz; z++)
				{
					auto sphereNode = m_Scene->GetRoot()->AddChild();
					sphereNode->GetComponentOfType<re::Transform>()->Position = re::Vector3(x, 0, z) * CSphereDistance;
					auto sphere = sphereNode->AddComponent<re::Sphere>();
					sphere->Material = m_SpheresMaterials[z * 3 + x].get();
				}
			}

			for (int x = 0; x < sx - 1; x++)
			{
				for (int z = 0; z < sz - 1; z++)
				{
					auto light = std::shared_ptr<re::Light>(new re::Light());
					light->Type = re::LightType::Point;
					light->Position = re::Vector3(x, 0, z) * CSphereDistance + re::Vector3(CSphereDistance / 2, CSphereDistance, CSphereDistance / 2);
					light->Attenuation = CSphereDistance * 2;
					light->Color = m_LampColors[(z * (sx - 1) + x) % m_LampColors.size()];
					m_Lamps.push_back(light);
				}
			}
		}

		{
			auto groundNode = m_Scene->GetRoot()->AddChild();
			groundNode->GetComponentOfType<re::Transform>()->Position = { 0, -1, 0 };
			m_Ground = groundNode->AddComponent<re::Plane>();
			m_Ground->Normal = { 0, 1, 0 };
			m_Ground->Material = m_Materials["bw_checker"].get();
		}

		return m_Scene;
	}

	sf::RenderWindow m_Window;
	sf::Texture m_Texture, m_FastTexture;
	sf::RectangleShape m_RenderRect;
	sf::Font m_Font;

	std::vector<std::shared_ptr<re::Background>> m_SkyBoxes;
	std::vector<std::shared_ptr<re::Light>> m_AmbientLights;
	std::vector<std::shared_ptr<re::Light>> m_DirectionalLights;

	std::shared_ptr<re::Scene> m_Scene;
	std::shared_ptr<re::Raytracer> m_Raytracer;
	std::shared_ptr<re::DebugRaycaster> m_Raycaster;
	std::map<std::string, std::shared_ptr<re::Material>> m_Materials;
	std::vector<std::shared_ptr<re::Material>> m_SpheresMaterials;
	std::vector<std::shared_ptr<re::Material>> m_GroundMaterials;
	std::vector<std::shared_ptr<re::Light>> m_Lamps;

	std::array<re::Color, 4> m_LampColors = { 0xc7ffbc, 0xffc1bc, 0xbcc3ff, 0xfff8bc };
	
	re::Plane * m_Ground;
	re::Vector2 m_CameraAngle;

	bool m_Render = false, m_SuperSampling = false, m_HideInfo = false, m_LampsSwitch = false;
	int m_GroundMaterial = 0, m_SkyBox = 0;
	int m_MaxRecursion = 3;

	static constexpr float CMovementSpeed = 10.0f;
	static constexpr float CSphereDistance = 4.0f;
};


int main(int argc, char** argv)
{
	Test1 test1;
	test1.Start();
}