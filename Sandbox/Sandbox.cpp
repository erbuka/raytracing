#include "Sandbox.h"

#include <chrono>


static void GLFW_WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	sb::Sandbox* sandbox =  static_cast<sb::Sandbox*>(glfwGetWindowUserPointer(window));
	sandbox->Resize(width, height);
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

	InitGL();
	InitImGUI();
	InitMaterials();
	InitScene();

	m_Raytracer = std::shared_ptr<re::Raytracer>(new re::Raytracer(m_Width, m_Height));
	m_Raycaster = std::shared_ptr<re::DebugRaycaster>(new re::DebugRaycaster(m_Width / 4, m_Height / 4));

	auto currTime = std::chrono::high_resolution_clock::now();
	auto prevTime = std::chrono::high_resolution_clock::now();

	/* Map window funcs */
	glfwSetWindowSizeCallback(m_Window, &GLFW_WindowSizeCallback);

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
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenTextures(1, &m_Texture);
	glBindTexture(GL_TEXTURE_2D, m_Texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

}

void sb::Sandbox::Update(float dt)
{

	static auto isKeyDown = [this](int key) -> bool {
		int status = glfwGetKey(m_Window, key);
		return status == GLFW_PRESS || status == GLFW_REPEAT;
	};

	auto pixels = m_Raycaster->Render(m_Scene.get());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width / 4, m_Height / 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	auto right = re::Cross(m_Scene->LookDirection, re::Vector3::Up);

	if (isKeyDown(GLFW_KEY_W))
	{
		m_Scene->CameraPosition = m_Scene->CameraPosition + m_Scene->LookDirection * MoveSpeed * dt;
	}
	else if (isKeyDown(GLFW_KEY_S))
	{
		m_Scene->CameraPosition = m_Scene->CameraPosition - m_Scene->LookDirection * MoveSpeed * dt;
	}

	if (isKeyDown(GLFW_KEY_D))
	{
		m_Scene->CameraPosition = m_Scene->CameraPosition + right * MoveSpeed * dt;
	}
	else if (isKeyDown(GLFW_KEY_A))
	{
		m_Scene->CameraPosition = m_Scene->CameraPosition - right * MoveSpeed * dt;
	}


}

void sb::Sandbox::Render(float dt)
{
	
	/* Scene */
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_QUADS);
	{
		glColor3f(1, 1, 1); glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
		glColor3f(0, 1, 1); glTexCoord2f(1, 1); glVertex3f(1, 0, 0);
		glColor3f(1, 0, 1); glTexCoord2f(1, 0);	glVertex3f(1, 1, 0);
		glColor3f(1, 1, 0); glTexCoord2f(0, 0);	glVertex3f(0, 1, 0);
	}
	glEnd();

	/* ImGUI */

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	{
		
		ImGui::Begin("Raytracing");                         
		ImGui::Checkbox("Supersampling", &Settings.Supersampling);
		ImGui::SliderInt("Max Recursion", &Settings.MaxRecursion, 0, 2);
		ImGui::Combo("Ground Material", &Settings.GroundMaterial, "Checkerboard\0Marble\0Worley");
		ImGui::Combo("Sky", &Settings.Sky, "Day\0Night");
		ImGui::Checkbox("Lamps switch", &Settings.LampsSwitch);
		ImGui::End();
		
	}
	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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
		m_DirectionalLights.push_back(std::shared_ptr<re::Light>(makeDirectionalLight(0x00353532, { 1, 1, -1 })));

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


