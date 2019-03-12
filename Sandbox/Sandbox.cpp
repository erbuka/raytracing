#include "Sandbox.h"

#include "WavefrontLoader.h"

#include <chrono>


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
	auto sandbox = GetSandbox(window);
	switch (action)
	{
	case GLFW_PRESS:
	case GLFW_REPEAT:
		sandbox->KeyPressed(keycode, action == GLFW_REPEAT);
		break;
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
	InitMaterials();
	InitScene();

	m_Raytracer = std::shared_ptr<re::Raytracer>(new re::Raytracer(m_Width, m_Height));
	m_Raytracer->NumThreads = 8;

	m_Raycaster = std::shared_ptr<re::DebugRaycaster>(new re::DebugRaycaster(m_Width / 8, m_Height / 8));
	m_Raycaster->Mode = re::DebugRaycaster::Modes::Color;
	m_Raycaster->NumThreads = 8;

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


		m_Ground->Material = m_GroundMaterials[Settings.GroundMaterial].get();

		m_Scene->Lights.clear();

		m_Scene->Lights.push_back(m_AmbientLights[Settings.Sky].get());
		m_Scene->Lights.push_back(m_DirectionalLights[Settings.Sky].get());

		if (Settings.LampsSwitch)
		{
			for (auto &l : m_Lamps)
			{
				m_Scene->Lights.push_back(l.get());
			}
		}

		m_Scene->Background = m_SkyBoxes[Settings.Sky].get();
	}

	auto right = re::Cross(m_Scene->Camera.Direction, re::Vector3::Up);

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
				ImGui::Combo("Ground Material", &Settings.GroundMaterial, "Checkerboard\0Marble\0Worley");
				ImGui::Combo("Sky", &Settings.Sky, "Day\0Night");
				ImGui::Checkbox("Lights on", &Settings.LampsSwitch);
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

void sb::Sandbox::InitMaterials()
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
	m_Materials.insert({ "solid_black", std::shared_ptr<re::Material>(new re::UniformMaterial(0, 1.0f)) });
	m_Materials.insert({ "marble", std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Noise>(new re::Marble(5, 32)), m_Materials["solid_black"], m_Materials["mirror"])) });

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

void sb::Sandbox::InitScene()
{
	m_Scene = std::shared_ptr<re::Scene>(new re::Scene());

	/*
	m_Scene->CameraPosition = { -2.88, 7.86, -4.26 };
	m_CameraDir.Alpha = 0.9;
	m_CameraDir.Beta = -0.8;
	*/


	m_Scene->Camera.Position = { 0, 3, 20 };
	m_CameraDir.Alpha = -re::PI/2;
	m_CameraDir.Beta = 0;

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

		m_AmbientLights.push_back(std::shared_ptr<re::Light>(makeAmbientLight(0)));
		m_AmbientLights.push_back(std::shared_ptr<re::Light>(makeAmbientLight(0)));

		m_DirectionalLights.push_back(std::shared_ptr<re::Light>(makeDirectionalLight(0xffddee, { 1, 1, -1 })));
		m_DirectionalLights.push_back(std::shared_ptr<re::Light>(makeDirectionalLight(0x353532, { 1, 1, -1 })));

		//m_SkyBoxes.push_back(std::shared_ptr<re::Background>(new re::SkyBox(0x009CE5FF, 0x00C3FAFF, m_DirectionalLights[0].get())));
		m_SkyBoxes.push_back(std::shared_ptr<re::Background>(new re::SkyBox(0x4444ff, 0xffffff, m_DirectionalLights[0].get())));
		m_SkyBoxes.push_back(std::shared_ptr<re::Background>(new re::SkyBox(0x000033, 0x444444, m_DirectionalLights[1].get())));

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
		m_Ground->Material = nullptr;
	}

	{

		auto wfData = LoadWavefront("res/bunny.obj_");

		auto meshNode = m_Scene->GetRoot()->AddChild();
		auto mesh = meshNode->AddComponent<re::Mesh>();

		meshNode->GetComponentOfType<re::Transform>()->Position = { CSphereDistance, -1, -2 * CSphereDistance };
		meshNode->GetComponentOfType<re::Transform>()->Scale = { 5, 5, 5 };

		for (auto f : wfData["bunny"])
		{
			auto& t = mesh->AddTriangle();
			t.Vertices = { f.Vertices[0], f.Vertices[1], f.Vertices[2] };
			t.Normals = { f.Normals[0], f.Normals[1], f.Normals[2] };
		}
		
		mesh->NormalMode = re::NormalModes::Vertex;
		mesh->Material = m_Materials["marble"].get();
		
	
	}

}


std::shared_ptr<re::Material> sb::Sandbox::CreateMarble(std::shared_ptr<re::Material> dark, std::shared_ptr<re::Material> bright)
{
	return std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Marble>(new re::Marble(2.0f, 4.0f, 4.0f)), dark, bright));
}

std::shared_ptr<re::Material> sb::Sandbox::CreatePerlin(std::shared_ptr<re::Material> first, std::shared_ptr<re::Material> second)
{
	return std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Perlin>(new re::Perlin(1.0f)), first, second));
}

std::shared_ptr<re::Material> sb::Sandbox::CreateWorley(std::shared_ptr<re::Material> base, std::shared_ptr<re::Material> feats)
{
	return std::shared_ptr<re::Material>(new re::InterpolatedMaterial(std::shared_ptr<re::Worley>(new re::Worley(2.0f, 10.0f)), base, feats));
}


