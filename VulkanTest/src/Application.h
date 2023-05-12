#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <optional>
#include <vector>
#include <glm.hpp>

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

	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;
		static VkVertexInputBindingDescription GetBindingDescription()
		{
			VkVertexInputBindingDescription desc;
			desc.binding = 0;
			desc.stride = sizeof(Vertex);
			desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return desc;
		}

		static std::vector<VkVertexInputAttributeDescription> GetAttribuDescription()
		{
			std::vector<VkVertexInputAttributeDescription> attributes(2);
			attributes[0].binding = 0;
			attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributes[0].location = 0;
			attributes[0].offset = offsetof(Vertex, pos);

			attributes[1].binding = 0;
			attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributes[1].location = 1;
			attributes[1].offset = offsetof(Vertex, color);
			return attributes;
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
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void CreateSyncObjects();
	void DrawFrame();
	void CreateVertexBuffer();
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags);

private:
	GLFWwindow* m_Window;
	VkInstance m_Vkinstance;
	VkPhysicalDevice m_PhyiscalDevice = VK_NULL_HANDLE;
	VkDevice m_LogicDevice = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface;
	VkQueue m_PresentQueue;
	VkQueue m_GraphicQueue;
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
	VkSemaphore m_ImageAvailableSemaphore;
	VkSemaphore m_RenderFinishedSemaphore;
	VkFence m_InFlightFence;
	VkBuffer m_VertexBuffer;
	VkDeviceMemory m_BufferMemory;
	std::vector<Vertex> m_Vertices = {
	{{0.0f, -0.5f}, {0.8f, 0.6f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.6f, 0.7f}},
	{{-0.5f, 0.5f}, {0.8f, 0.0f, 1.0f}}
	};
};