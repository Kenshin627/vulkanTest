#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
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
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> presentModes;
		operator bool()
		{
			return !formats.empty() && !presentModes.empty();
		}
	};

	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;
		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			vk::VertexInputBindingDescription desc;
			return desc.setBinding(0)
					   .setStride(sizeof(Vertex))
					   .setInputRate(vk::VertexInputRate::eVertex);
		}

		static std::vector<vk::VertexInputAttributeDescription> GetAttribuDescription()
		{
			std::vector<vk::VertexInputAttributeDescription> attributes(2);
			attributes[0].setBinding(0)
						 .setFormat(vk::Format::eR32G32Sfloat)
						 .setLocation(0)
						 .setOffset(offsetof(Vertex, pos));

			attributes[1].setBinding(0)
					     .setFormat(vk::Format::eR32G32B32Sfloat)
					     .setLocation(1)
					     .setOffset(offsetof(Vertex, color));

			return attributes;
		}
	};

private:
	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	bool IsDeviceSuitable(const vk::PhysicalDevice& device);
	QueueFamilyIndices FindQueueFamilies(const vk::PhysicalDevice& device);
	void CreateLogicDevice();
	bool IsDeviceExtensionSupport(const vk::PhysicalDevice& device);
	SwapChainSupportDetail QuerySwapChainSupport(const vk::PhysicalDevice& device);
	void CreateSwapChain();
	vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
	vk::PresentModeKHR ChooseSwapSurfacePresentMode(const std::vector<vk::PresentModeKHR>& presentModes);
	vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
	void CreateImageViews();
	void CreateGraphicsPipeline();
	vk::ShaderModule CreateShaderModule(const std::vector<char>& sourceCode);
	void CreateRenderPass();
	void CreateFrameBuffer();
	void CreateCommandPool();
	void CreateCommandBuffer();
	void RecordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex);
	void CreateSyncObjects();
	void DrawFrame();
	void CreateVertexBuffer();
	uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags flags);

private:
	GLFWwindow* m_Window;
	vk::Instance m_Vkinstance;
	vk::PhysicalDevice m_PhyiscalDevice = VK_NULL_HANDLE;
	vk::Device m_LogicDevice = VK_NULL_HANDLE;
	vk::SurfaceKHR m_Surface;
	vk::Queue m_PresentQueue;
	vk::Queue m_GraphicQueue;
	vk::SwapchainKHR m_SwapChain;
	std::vector<vk::Image> m_SwapChainImages;
	vk::Format m_SwapChainFormat;
	vk::Extent2D m_SwapChainExtent;
	std::vector<vk::ImageView> m_ImageViews;
	std::vector<const char*> m_DeviceExtesions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::RenderPass m_Renderpass;
	vk::PipelineLayout m_PipelineLayout;
	vk::Pipeline m_Pipeline;
	std::vector<vk::Framebuffer> m_FrameBuffers;
	vk::CommandPool m_CommandPool;
	vk::CommandBuffer m_CommandBuffer;
	vk::Semaphore m_ImageAvailableSemaphore;
	vk::Semaphore m_RenderFinishedSemaphore;
	vk::Fence m_InFlightFence;
	vk::Buffer m_VertexBuffer;
	vk::DeviceMemory m_BufferMemory;
	std::vector<Vertex> m_Vertices = {
		{{0.0f, -0.5f}, {0.8f, 0.6f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.6f, 0.7f}},
		{{-0.5f, 0.5f}, {0.8f, 0.0f, 1.0f}}
	};
};