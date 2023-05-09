#include <vector>
#include <iostream>

#include "Application.h"

void Application::InitWindow()
{
	if (!glfwInit())
	{
		std::cout << "init glfw failed!" << std::endl;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	m_Window = glfwCreateWindow(800, 600, "Vulkan TEST", nullptr, nullptr);
	if (!m_Window)
	{
		std::cout << "create Window failed!" << std::endl;
	}
}

void Application::MainLoop()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();
	}
}

void Application::Cleanup()
{
	vkDestroyInstance(m_Vkinstance, nullptr);
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Application::InitVulkan()
{
	CreateInstance();
}

void Application::CreateInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "vulkanTest";

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	//validation layers
	createInfo.enabledLayerCount = 0;

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Vkinstance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void Application::PickPhysicalDevice()
{
	uint32_t phisicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_Vkinstance, &phisicalDeviceCount, nullptr);
	if (phisicalDeviceCount == 0)
	{
		throw std::runtime_error("can not found device!");
	}

	std::vector<VkPhysicalDevice> phsicalDevices(phisicalDeviceCount);
	vkEnumeratePhysicalDevices(m_Vkinstance, &phisicalDeviceCount, phsicalDevices.data());

	for (auto& device : phsicalDevices)
	{
		if (IsDeviceSuitable(device))
		{
			m_Device = device;
		}
	}
	if (m_Device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("can not found stuitable device!");
	}
}

bool Application::IsDeviceSuitable(const VkPhysicalDevice& device)
{
	VkPhysicalDeviceProperties properts{};
	VkPhysicalDeviceFeatures features{};
	vkGetPhysicalDeviceProperties(device, &properts);
	vkGetPhysicalDeviceFeatures(device, &features);
	return properts.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader;
}