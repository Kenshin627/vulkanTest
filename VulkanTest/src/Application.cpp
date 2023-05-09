#include <vector>
#include <set>
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
	vkDestroyDevice(m_LogicDevice, nullptr);
	vkDestroySurfaceKHR(m_Vkinstance, m_Surface, nullptr);
	vkDestroyInstance(m_Vkinstance, nullptr);
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Application::InitVulkan()
{
	CreateInstance();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicDevice();
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
			m_PhyiscalDevice = device;
		}
	}
	if (m_PhyiscalDevice == VK_NULL_HANDLE)
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

	QueueFamilyIndices indices = FindQueueFamilies(device);
	return  indices.IsComplete() && properts.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader;
}

Application::QueueFamilyIndices Application::FindQueueFamilies(const VkPhysicalDevice& device)
{
	QueueFamilyIndices indices;
	uint32_t queueCount = 0;	
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
	std::vector<VkQueueFamilyProperties> properties(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, properties.data());
	int i = 0;
	VkBool32 presentSupport = false;
	for (auto& property : properties)
	{
		if (indices.IsComplete())
		{
			break;
		}
		if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.GraphicFamily = i;
		}
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
		if (presentSupport)
		{
			indices.PresentFamily = i;
		}
		i++;
	}
	return indices;
}

void Application::CreateLogicDevice()
{
	float queuePriority = 1.0f;
	QueueFamilyIndices indices = FindQueueFamilies(m_PhyiscalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicFamily.value(), indices.PresentFamily.value() };
	for (auto& queueIndex : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = uniqueQueueFamilies.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	//validation layers
	createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_PhyiscalDevice, &createInfo, nullptr, &m_LogicDevice))
	{
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(m_LogicDevice, indices.PresentFamily.value(), 0, &m_PresentQueue);
}

void Application::CreateSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = glfwGetWin32Window(m_Window);
	createInfo.hinstance = GetModuleHandle(nullptr);
	if (vkCreateWin32SurfaceKHR(m_Vkinstance, &createInfo, nullptr, &m_Surface))
	{
		throw std::runtime_error("failed to create window surface!");
	}
}