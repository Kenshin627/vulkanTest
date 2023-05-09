#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Application
{
public:
	void Run() 
	{ 
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	};
	void InitWindow();
	void InitVulkan();
	void MainLoop();
	void Cleanup();
private:
	void CreateInstance();
	void PickPhysicalDevice();
	bool IsDeviceSuitable(const VkPhysicalDevice& device);
private:
	GLFWwindow* m_Window;
	VkInstance m_Vkinstance;
	VkPhysicalDevice m_Device = VK_NULL_HANDLE;
};