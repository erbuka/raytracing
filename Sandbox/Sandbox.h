#pragma once

#include <vector>
#include <memory>
#include <map>
#include <array>
#include <future>
#include <tuple>
#include <map>
#include <re.h>


#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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
		void KeyPressed(int keycode, bool repeat);
		void MouseMoved(float x, float y);
		void MousePressed(int button);
		void MouseReleased(int button);

	private:

		void LoadSceneCodes();
		void SaveSceneCodes();

		void InitGL();
		void InitImGUI();

		void UpdateScene();

		void Update(float dt);
		void Render(float dt);

		void StartRaytracer();


		std::pair<re::real, re::real> GetCursorPos();
		std::pair<re::real, re::real> GetWindowSize();


	private:
		GLFWwindow* m_Window;

		unsigned int m_Width, m_Height;

		bool m_SceneDirty = true, m_ValidRender = false, m_ShowImGui = true;

		struct {
			re::Raytracer::AAMode Antialiasing = re::Raytracer::AAMode::None;
			int MaxRecursion = 3;
		} Settings;


		std::future<re::Renderer::RenderStatus> m_RaytracerFuture;

		std::shared_ptr<re::Scene> m_Scene;
		std::shared_ptr<re::Raytracer> m_Raytracer;
		std::shared_ptr<re::DebugRaycaster> m_Raycaster;
		std::vector<std::shared_ptr<re::Material>> m_Materials;
		std::vector<std::shared_ptr<re::Noise>> m_Noises;


		
		std::string m_CurrentSceneName;
		char m_CurrentSceneCode[1024 * 10] = "";
		char m_SaveSceneInputName[128] = "";
		std::map<std::string, std::string> m_SceneCodes;
		std::string m_LuaError;

		// std::array<re::Color, 4> m_LampColors = { 0x00ff00, 0xff0000, 0x0000ff, 0x00ffff };

		GLuint m_FastTexture, m_Texture;

		std::pair<re::real, re::real> m_PrevDragPos, m_CurrDragPos;

		struct {
			float Alpha = -re::PI / 2;
			float Beta = 0;
		} m_CameraDir;

		static constexpr float MoveSpeed = 8.0f;

	};
}