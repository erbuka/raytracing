#include "Sandbox.h"

#include "WavefrontLoader.h"

#include <chrono>
#include <fstream>

#include <lua.hpp>
#include <LuaState.h>

#include <tinyxml2.h>


static std::string SAVED_SCENES_FILE = "res/scenes.xml";

template<typename T, typename ...Args>
void CheckSize(T& container, size_t index, const char * fmt, Args... args)
{
	if (index >= container.size())
	{
		static char buffer[256];
		sprintf_s(buffer, fmt, args...);
		throw std::exception(buffer);
	}
}

template<typename ...Args>
std::string TsPrintf(const char * fmt, Args... args)
{
	static char buffer[256];
	sprintf_s(buffer, fmt, args...);
	return std::string(buffer);
}


static inline sb::Sandbox* GetSandbox(GLFWwindow* window)
{
	return static_cast<sb::Sandbox*>(glfwGetWindowUserPointer(window));
}

static void GLFW_WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	GetSandbox(window)->Resize(width, height);
}


static void GLFW_KeyCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods)
{

	auto& io = ImGui::GetIO();
	
 	if (!io.WantCaptureKeyboard)
	{
		auto sandbox = GetSandbox(window);
		switch (action)
		{
		case GLFW_PRESS:
		case GLFW_REPEAT:
			sandbox->KeyPressed(keycode, action == GLFW_REPEAT);
			break;
		}
	}
}

static void GLFW_CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	auto& io = ImGui::GetIO();

	if (!io.WantCaptureMouse)
	{
		GetSandbox(window)->MouseMoved(static_cast<float>(xpos), static_cast<float>(ypos));
	}
}

static void GLFW_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{	
	auto& io = ImGui::GetIO();

	if (!io.WantCaptureMouse)
	{

		switch (action)
		{
		case GLFW_PRESS:
			GetSandbox(window)->MousePressed(button);
			break;
		case GLFW_RELEASE:
			GetSandbox(window)->MouseReleased(button);
			break;
		}
	}
}

sb::Sandbox::~Sandbox()
{
}

int sb::Sandbox::Start(unsigned int width, unsigned int height)
{

	m_Width = width;
	m_Height = height;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */


	m_Window = glfwCreateWindow(m_Width, m_Height, "Raytracing Sandbox", NULL, NULL);
	if (!m_Window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(m_Window);


	/* Init GLAD */
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


	/* Map window funcs */
	glfwSetWindowSizeCallback(m_Window, &GLFW_WindowSizeCallback);
	glfwSetKeyCallback(m_Window, &GLFW_KeyCallback);
	glfwSetCursorPosCallback(m_Window, &GLFW_CursorPosCallback);
	glfwSetMouseButtonCallback(m_Window, &GLFW_MouseButtonCallback);


	InitGL();
	InitImGUI();
	LoadSceneCodes();


	m_Raytracer = std::shared_ptr<re::Raytracer>(new re::Raytracer(m_Width, m_Height));
	m_Raytracer->NumThreads = 8;

	m_Raycaster = std::shared_ptr<re::DebugRaycaster>(new re::DebugRaycaster(m_Width / 8, m_Height / 8));
	m_Raycaster->Mode = re::DebugRaycaster::Modes::Color;
	m_Raycaster->NumThreads = 8;

	// Init scene
	UpdateScene();

	auto currTime = std::chrono::high_resolution_clock::now();
	auto prevTime = std::chrono::high_resolution_clock::now();

	glfwSetWindowUserPointer(m_Window, this);


	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(m_Window))
	{
		currTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> delta = currTime - prevTime;
		prevTime = currTime;

		/* Render here */
		Update(delta.count());
		Render(delta.count());

		/* Swap front and back buffers */
		glfwSwapBuffers(m_Window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void sb::Sandbox::Resize(unsigned int width, unsigned int height)
{
	glViewport(0, 0, width, height);
}

void sb::Sandbox::KeyPressed(int keycode, bool repeat)
{
	if (keycode == GLFW_KEY_SPACE && !repeat)
	{
		m_ShowImGui = !m_ShowImGui;
	}
}

void sb::Sandbox::MouseMoved(float x, float y)
{
	if (glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		m_CurrDragPos = GetCursorPos();
		
		re::real w, h;
		std::tie(w, h) = GetWindowSize();
		
		float dx = -(m_CurrDragPos.first - m_PrevDragPos.first) / w;
		float dy = (m_CurrDragPos.second - m_PrevDragPos.second) / h;

		m_CameraDir.Alpha += dx * re::PI;
		m_CameraDir.Beta += dy * re::PI;

		if (dx != 0 || dy != 0)
		{
			m_SceneDirty = true;
			m_Raytracer->Interrupt();
		}

		m_PrevDragPos = m_CurrDragPos;
	}
}

void sb::Sandbox::MousePressed(int button)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		m_CurrDragPos = m_PrevDragPos = GetCursorPos();
	}
}

void sb::Sandbox::MouseReleased(int button)
{
}

void sb::Sandbox::InitImGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}

void sb::Sandbox::LoadSceneCodes()
{
	using namespace tinyxml2;

	XMLDocument doc;
	if (doc.LoadFile(SAVED_SCENES_FILE.c_str()) == XML_SUCCESS)
	{

		auto root = doc.RootElement();

		for (auto sceneEl = root->FirstChildElement("scene"); sceneEl != nullptr; sceneEl = sceneEl->NextSiblingElement("scene"))
		{
			auto text = sceneEl->GetText();
			m_SceneCodes[sceneEl->Attribute("name")] = text != nullptr ? text : "";
		}
	}

	if (m_SceneCodes.size() > 0)
	{
		auto scene = m_SceneCodes.begin();

		m_CurrentSceneName = scene->first;
		strcpy(m_CurrentSceneCode, scene->second.c_str());
	}
}

void sb::Sandbox::SaveSceneCodes()
{
	using namespace tinyxml2;
	
	XMLPrinter printer;

	printer.OpenElement("scenes");
	for (auto& s : m_SceneCodes)
	{
		printer.OpenElement("scene");
		printer.PushAttribute("name", s.first.c_str());
		printer.PushText(s.second.c_str());
		printer.CloseElement();
	}
	printer.CloseElement();

	XMLDocument doc;
	doc.Parse(printer.CStr());
	doc.SaveFile(SAVED_SCENES_FILE.c_str());
}

void sb::Sandbox::InitGL()
{
	glClearColor(0, 0, 0, 1);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	glGenTextures(1, &m_FastTexture);
	glBindTexture(GL_TEXTURE_2D, m_FastTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

}

void sb::Sandbox::Update(float dt)
{

	static auto isKeyDown = [this](int key) -> bool {
		int status = glfwGetKey(m_Window, key);
		return status == GLFW_PRESS || status == GLFW_REPEAT;
	};

	if (m_Raytracer->GetStatus().Finished)
	{
		m_Scene->Camera.Direction = re::Vector3{
			std::cos(m_CameraDir.Beta) * std::cos(m_CameraDir.Alpha),
			std::sin(m_CameraDir.Beta),
			std::cos(m_CameraDir.Beta) * std::sin(m_CameraDir.Alpha)
		};

		m_Raytracer->Antialiasing = Settings.Antialiasing;
		m_Raytracer->MaxRecursion = Settings.MaxRecursion;
	}

	auto right = re::Cross(m_Scene->Camera.Direction, re::Vector3::Up);
	
	auto& io = ImGui::GetIO();
	if (!io.WantTextInput)
	{
		if (isKeyDown(GLFW_KEY_W))
		{
			m_Scene->Camera.Position = m_Scene->Camera.Position + m_Scene->Camera.Direction * MoveSpeed * dt;
			m_SceneDirty = true;
		}
		else if (isKeyDown(GLFW_KEY_S))
		{
			m_Scene->Camera.Position = m_Scene->Camera.Position - m_Scene->Camera.Direction * MoveSpeed * dt;
			m_SceneDirty = true;
		}

		if (isKeyDown(GLFW_KEY_D))
		{
			m_Scene->Camera.Position = m_Scene->Camera.Position + right * MoveSpeed * dt;
			m_SceneDirty = true;
		}
		else if (isKeyDown(GLFW_KEY_A))
		{
			m_Scene->Camera.Position = m_Scene->Camera.Position - right * MoveSpeed * dt;
			m_SceneDirty = true;
		}
	}

	if (m_SceneDirty)
	{
		m_SceneDirty = false;
		m_ValidRender = false;
		m_Raytracer->Interrupt();

		unsigned int * pixels = m_Raycaster->RenderSync(m_Scene.get());
		glBindTexture(GL_TEXTURE_2D, m_FastTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Raycaster->GetViewWidth(), m_Raycaster->GetViewHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}
	else
	{
		auto status = m_Raytracer->GetStatus();

		if (!m_ValidRender && status.Finished && !status.Interruped)
		{
			glBindTexture(GL_TEXTURE_2D, m_Texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Raytracer->GetViewWidth(), m_Raytracer->GetViewHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, status.Pixels);
			m_ValidRender = true;
		}
	}

}

void sb::Sandbox::Render(float dt)
{
	auto& io = ImGui::GetIO();


	auto status = m_Raytracer->GetStatus();
	/* Scene */

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, m_ValidRender ? m_Texture : m_FastTexture);

	glBegin(GL_QUADS);
	{
		glColor3f(1, 1, 1);
		glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
		glTexCoord2f(1, 1); glVertex3f(1, 0, 0);
		glTexCoord2f(1, 0);	glVertex3f(1, 1, 0);
		glTexCoord2f(0, 0);	glVertex3f(0, 1, 0);
	}
	glEnd();

	/* ImGUI */
	if (m_ShowImGui)
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		{
			ImGui::Begin("Raytracing");

			if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
			{
				if (ImGui::BeginTabItem("Main"))
				{
					if (ImGui::CollapsingHeader("Instructions", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::BulletText("Press WASD to move");
						ImGui::BulletText("Use the mouse to rotate");
						ImGui::BulletText("Press \"Render\" button to render the scene");
						ImGui::BulletText("Press SPACE key to hide/show this window");
					}

					if (ImGui::CollapsingHeader("Info", ImGuiTreeNodeFlags_DefaultOpen))
					{
						float cameraPos[3] = { m_Scene->Camera.Position.X, m_Scene->Camera.Position.Y, m_Scene->Camera.Position.Z };
						float lookDir[3] = { m_Scene->Camera.Direction.X, m_Scene->Camera.Direction.Y, m_Scene->Camera.Direction.Z };

						ImGui::InputFloat3("Camera position", cameraPos, 3, ImGuiInputTextFlags_ReadOnly);
						ImGui::InputFloat3("Look direction", lookDir, 3, ImGuiInputTextFlags_ReadOnly);
					}

					if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Combo("Antialiasing", (int*)&Settings.Antialiasing, "None\0SSAA");
						ImGui::SliderInt("Max Recursion", &Settings.MaxRecursion, 0, 3);
						ImGui::Combo("Fast Raycaster Mode", (int*)(&m_Raycaster->Mode), "Normal\0Color");
					}

					auto status = m_Raytracer->GetStatus();

					if (status.Finished)
					{
						if (ImGui::Button("Render", { ImGui::GetContentRegionAvailWidth(), 0 })) {
							StartRaytracer();
						};
					}
					else
					{
						ImGui::ProgressBar(status.Percent);
					}

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Scene Editor"))
				{

					if (m_SceneCodes.size() > 0)
					{
						
						if (ImGui::BeginCombo("Scene", m_CurrentSceneName.c_str()))
						{
							for (auto& s : m_SceneCodes)
							{
								auto name = s.first;
								bool selected = name == m_CurrentSceneName;
								if (ImGui::Selectable(name.c_str(), selected))
								{
									m_CurrentSceneName = name;
									strcpy(m_CurrentSceneCode, s.second.c_str());
								}
							}
							ImGui::EndCombo();
						}
					}


					if (ImGui::Button("Save"))
					{
						if (m_CurrentSceneName != "")
						{
							m_SceneCodes[m_CurrentSceneName] = m_CurrentSceneCode;
							SaveSceneCodes();
						}
					}

					ImGui::SameLine();

					if (ImGui::Button("Save as..."))
					{
						ImGui::OpenPopup("SaveSceneModal");
					}

					ImGui::SameLine();

					if (ImGui::Button("Delete"))
					{
						if(m_CurrentSceneName != "")
						{
							m_SceneCodes.erase(m_CurrentSceneName);
							m_CurrentSceneName = "";
							SaveSceneCodes();
						}
					}

					if (ImGui::BeginPopupModal("SaveSceneModal", NULL, ImGuiWindowFlags_AlwaysAutoResize))
					{
						ImGui::InputText("Scene name", m_SaveSceneInputName, sizeof(m_SaveSceneInputName));
						if (ImGui::Button("Save"))
						{
							std::string name(m_SaveSceneInputName);
							if (name.size() > 0)
							{
								m_SceneCodes[name] = m_CurrentSceneCode;
								SaveSceneCodes();
								ImGui::CloseCurrentPopup();
							}
						}
						ImGui::SameLine();
						if (ImGui::Button("Close"))
						{
							ImGui::CloseCurrentPopup();
						}
						ImGui::EndPopup();
					}

					ImGui::InputTextMultiline("Code", m_CurrentSceneCode, sizeof(m_CurrentSceneCode), { -1, -ImGui::GetFrameHeightWithSpacing() });
					if (ImGui::Button("Run Code"))
					{
						UpdateScene();
					}

					if (!m_LuaError.empty())
					{
						ImGui::SameLine();
						ImGui::TextColored({ 1,0,0,1 }, "Error: %s", m_LuaError.c_str());
					}

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}

			ImGui::End();

		}


		ImGui::Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

}

void sb::Sandbox::StartRaytracer()
{
	std::promise<re::Renderer::RenderStatus> p;
	m_RaytracerFuture = p.get_future();
	m_Raytracer->Render(m_Scene.get(), std::move(p));
	m_ValidRender = false;
}

std::pair<re::real,re::real> sb::Sandbox::GetCursorPos()
{
	double x, y;
	glfwGetCursorPos(m_Window, &x, &y);
	return { static_cast<re::real>(x), static_cast<re::real>(y) };
}

std::pair<re::real, re::real> sb::Sandbox::GetWindowSize()
{
	int x, y;
	glfwGetWindowSize(m_Window, &x, &y);
	return { static_cast<re::real>(x), static_cast<re::real>(y) };
}

void sb::Sandbox::UpdateScene()
{
	lua::State state;

	try
	{
		m_Scene = std::make_shared<re::Scene>();

		m_CameraDir = CameraDir();

		m_Noises.clear();
		m_Materials.clear();

		re::Vector3 position, scale = { 1,1,1 };
		

		// Materials
		state.set("reUniformMaterial", [&](int color, re::real absorptance) -> int {
			m_Materials.push_back(std::shared_ptr<re::Material>(new re::UniformMaterial(color, absorptance)));
			return m_Materials.size() - 1;
		});

		state.set("reInterpolatedMaterial", [&](int noise, int material0, int material1) -> int {

			CheckSize(m_Materials, material0, "Invalid material: %d", material0);
			CheckSize(m_Materials, material1, "Invalid material: %d", material1);
			CheckSize(m_Noises, noise, "Invalid noise: %d", noise);

			auto noisePtr = m_Noises[noise];
			auto mat0Ptr = m_Materials[material0];
			auto mat1Ptr = m_Materials[material1];

			m_Materials.push_back(std::shared_ptr<re::Material>(new re::InterpolatedMaterial(noisePtr, mat0Ptr, mat1Ptr)));
			return m_Materials.size() - 1;
		});

		// Noises

		state.set("reCheckerBoard", [&](re::real domainSize) -> int {
			m_Noises.push_back(std::shared_ptr<re::Noise>(new re::CheckerBoard(domainSize)));
			return m_Noises.size() - 1;
		});
			
		state.set("rePerlin", [&](re::real domainSize) -> int {
			m_Noises.push_back(std::shared_ptr<re::Noise>(new re::Perlin(domainSize)));
			return m_Noises.size() - 1;
		});

		state.set("reWorley", [&](re::real domainSize, int divisions) -> int {
			m_Noises.push_back(std::shared_ptr<re::Noise>(new re::Worley(domainSize, divisions)));
			return m_Noises.size() - 1;
		});

		state.set("reMarble", [&](re::real domainSize, re::real frequency, re::real turbolence) -> int {
			m_Noises.push_back(std::shared_ptr<re::Noise>(new re::Marble(domainSize, frequency, turbolence)));
			return m_Noises.size() - 1;
		});

		// Shapes
			
		state.set("rePosition", [&](re::real x, re::real y, re::real z) -> void {
			position = { x,y,z };
		});

		state.set("reScale", [&](re::real x, re::real y, re::real z) -> void {
			scale = { x,y,z };
		});

		state.set("reSphere", [&](size_t material) -> void {

			CheckSize(m_Materials, material, "Invalid material: %d", material);

			auto sphereNode = m_Scene->GetRoot()->AddChild();
			sphereNode->GetComponentOfType<re::Transform>()->Position = position;
			sphereNode->GetComponentOfType<re::Transform>()->Scale = scale;
			auto sphere = sphereNode->AddComponent<re::Sphere>();
			sphere->Material = m_Materials[material].get();
		});

		state.set("rePlane", [&](int material, re::real nx, re::real ny, re::real nz) -> void {
			
			CheckSize(m_Materials, material, "Invalid material: %d", material);

			auto planeNode = m_Scene->GetRoot()->AddChild();
			planeNode->GetComponentOfType<re::Transform>()->Position = position;
			planeNode->GetComponentOfType<re::Transform>()->Scale = scale;
			auto plane = planeNode->AddComponent<re::Plane>();
			plane->Normal = { nx, ny, nz };
			plane->Material = m_Materials[material].get();
		});

		state.set("reObjMesh", [&](int material, std::string file, std::string group) -> void {

			CheckSize(m_Materials, material, "Invalid material: %d", material);

			auto wfData = LoadWavefront(file);

			auto meshNode = m_Scene->GetRoot()->AddChild();
			auto mesh = meshNode->AddComponent<re::Mesh>();

			meshNode->GetComponentOfType<re::Transform>()->Position = position;
			meshNode->GetComponentOfType<re::Transform>()->Scale = scale;

			if (wfData.find(group) == wfData.end())
				throw std::exception(TsPrintf("Invalid obj group: %s", group.c_str()).c_str());

			for (auto f : wfData[group])
			{
				auto& t = mesh->AddTriangle();
				t.Vertices = { f.Vertices[0], f.Vertices[1], f.Vertices[2] };
				t.Normals = { f.Normals[0], f.Normals[1], f.Normals[2] };
			}

			mesh->NormalMode = re::NormalModes::Vertex;
			mesh->Material = m_Materials[material].get();
		});


		// Lights
		state.set("reAmbientLight", [&](int color) -> int {
			auto light = std::make_shared<re::Light>();
			
			light->Type = re::LightType::Ambient;
			light->Color = color;

			m_Scene->Lights.push_back(light);
			m_Lights.push_back(light);
			
			return m_Lights.size() - 1;
		});

		state.set("reDirectionalLight", [&](int color, re::real nx, re::real ny, re::real nz) -> int {
			auto light = std::make_shared<re::Light>();
			
			light->Type = re::LightType::Directional;
			light->Color = color;
			light->Direction = re::Vector3(nx,ny,nz).Normalized();
			
			m_Scene->Lights.push_back(light);
			m_Lights.push_back(light);

			return m_Lights.size() - 1;
		});

		state.set("rePointLight", [&](int color, re::real attenuation) -> int {
			auto light = std::make_shared<re::Light>();

			light->Type = re::LightType::Point;
			light->Color = color;
			light->Position = position;
			light->Attenuation = attenuation;

			m_Scene->Lights.push_back(light);
			m_Lights.push_back(light);

			return m_Lights.size() - 1;
		});


		// Camera control
		state.set("reCameraPos", [&](re::real x, re::real y, re::real z) -> void {
			m_Scene->Camera.Position = { x,y,z };
		});

		// Background
		state.set("reSkyBox", [&](int color0, int color1, int light) -> void {

			CheckSize(m_Lights, light, "Invalid light: %d", light);


			m_Scene->Background = std::shared_ptr<re::Background>(new re::SkyBox(color0, color1, m_Lights[light]));

		});


		state.doString(m_CurrentSceneCode);
		m_LuaError = "";
		m_SceneDirty = true;
		m_Raytracer->Interrupt();
	}
	catch (std::exception err)
	{
		m_LuaError = err.what();
	}

}

