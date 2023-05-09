#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <optional>

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
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> GraphicFamily;
		std::optional<uint32_t> PresentFamily;
		bool IsComplete() { return GraphicFamily.has_value() && PresentFamily.has_value(); }
	};
private:
	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	bool IsDeviceSuitable(const VkPhysicalDevice& device);
	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device);
	void CreateLogicDevice();

private:
	GLFWwindow* m_Window;
	VkInstance m_Vkinstance;
	VkPhysicalDevice m_PhyiscalDevice = VK_NULL_HANDLE;
	VkDevice m_LogicDevice = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface;
	VkQueue m_PresentQueue;
};