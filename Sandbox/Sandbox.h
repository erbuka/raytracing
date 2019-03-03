#pragma once

#include <vector>
#include <memory>
#include <map>
#include <array>
#include <future>
#include <tuple>
#include <RealityEngine.h>


#include <imgui.h>
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace sb
{
	class Sandbox
	{
	public:
		
		~Sandbox();

		int Start(unsigned int width, unsigned int height);

		void Resize(unsigned int width, unsigned int height);
		void KeyPressed(int keycode);
		void MouseMoved(float x, float y);
		void MousePressed(int button);
		void MouseReleased(int button);

	private:

		void InitGL();
		void InitImGUI();
		void InitMaterials();
		void InitScene();

		void Update(float dt);
		void Render(float dt);

		void StartRaytracer();

		std::pair<re::real, re::real> GetCursorPos();
		std::pair<re::real, re::real> GetWindowSize();

		std::shared_ptr<re::Material> sb::Sandbox::CreateMarble(std::shared_ptr<re::Material> dark, std::shared_ptr<re::Material> bright);
		std::shared_ptr<re::Material> sb::Sandbox::CreatePerlin(std::shared_ptr<re::Material> first, std::shared_ptr<re::Material> second);
		std::shared_ptr<re::Material> sb::Sandbox::CreateWorley(std::shared_ptr<re::Material> base, std::shared_ptr<re::Material> feats);


	private:
		GLFWwindow* m_Window;

		unsigned int m_Width, m_Height;

		bool m_SceneDirty = true;

		struct {
			re::Raytracer::AAMode Antialiasing = re::Raytracer::AAMode::None;
			int MaxRecursion = 1;
			int GroundMaterial = 0;
			int Sky = 0;
			bool LampsSwitch = false;
		} Settings;

		std::future<re::Renderer::RenderStatus> m_RaytracerFuture;

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

		GLuint m_FastTexture, m_Texture;

		re::Plane * m_Ground;

		std::pair<re::real, re::real> m_PrevDragPos, m_CurrDragPos;

		struct {
			float Alpha = re::PI / 2;
			float Beta = 0;
		} m_CameraDir;

		static constexpr float CMovementSpeed = 10.0f;
		static constexpr float CSphereDistance = 4.0f;

		static constexpr float MoveSpeed = 8.0f;

	};
}