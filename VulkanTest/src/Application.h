#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <optional>
#include <vector>

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

	struct SwapChainSupportDetail
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
		operator bool()
		{
			return !formats.empty() && !presentModes.empty();
		}
	};
private:
	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	bool IsDeviceSuitable(const VkPhysicalDevice& device);
	QueueFamilyIndices FindQueueFamilies(const VkPhysicalDevice& device);
	void CreateLogicDevice();
	bool IsDeviceExtensionSupport(const VkPhysicalDevice& device);
	SwapChainSupportDetail QuerySwapChainSupport(const VkPhysicalDevice& device);
	void CreateSwapChain();
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseSwapSurfacePresentMode(const std::vector<VkPresentModeKHR>& presentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	void CreateImageViews();
	void CreateGraphicsPipeline();
	VkShaderModule CreateShaderModule(const std::vector<char>& sourceCode);
	void CreateRenderPass();
	void CreateFrameBuffer();
	void CreateCommandPool();
	void CreateCommandBuffer();

private:
	GLFWwindow* m_Window;
	VkInstance m_Vkinstance;
	VkPhysicalDevice m_PhyiscalDevice = VK_NULL_HANDLE;
	VkDevice m_LogicDevice = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface;
	VkQueue m_PresentQueue;
	VkSwapchainKHR m_SwapChain;
	std::vector<VkImage> m_SwapChainImages;
	VkFormat m_SwapChainFormat;
	VkExtent2D m_SwapChainExtent;
	std::vector<VkImageView> m_ImageViews;
	std::vector<const char*> m_DeviceExtesions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkRenderPass m_Renderpass;
	VkPipelineLayout m_PipelineLayout;
	VkPipeline m_Pipeline;
	std::vector<VkFramebuffer> m_FrameBuffers;
	VkCommandPool m_CommandPool;
	VkCommandBuffer m_CommandBuffer;
};