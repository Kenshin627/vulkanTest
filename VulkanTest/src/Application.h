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
		glm::vec2 coord;
		static vk::VertexInputBindingDescription GetBindingDescription()
		{
			vk::VertexInputBindingDescription desc;
			return desc.setBinding(0)
					   .setStride(sizeof(Vertex))
					   .setInputRate(vk::VertexInputRate::eVertex);
		}

		static std::vector<vk::VertexInputAttributeDescription> GetAttribuDescription()
		{
			std::vector<vk::VertexInputAttributeDescription> attributes(3);
			attributes[0].setBinding(0)
						 .setFormat(vk::Format::eR32G32Sfloat)
						 .setLocation(0)
						 .setOffset(offsetof(Vertex, pos));

			attributes[1].setBinding(0)
					     .setFormat(vk::Format::eR32G32B32Sfloat)
					     .setLocation(1)
					     .setOffset(offsetof(Vertex, color));
			
			attributes[2].setBinding(0)
						 .setFormat(vk::Format::eR32G32Sfloat)
						 .setLocation(2)
						 .setOffset(offsetof(Vertex, coord));

			return attributes;
		}
	};

	struct UniformBufferObject
	{
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 projection;
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
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);
	uint32_t FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags flags);
	void CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void createDescriptorSetLayout();
	void UploadUniformBuffer(uint32_t currentImage);
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateImage(uint32_t width, uint32_t height, vk::Format format);
	void CreateImageTexture();
	vk::CommandBuffer BeginOneTimeCommand();
	void EndCommand(vk::CommandBuffer commandBuffer);
	void TransiationImageLayout(const vk::Image& image, vk::AccessFlags distFlags, vk::AccessFlags srcFlags, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage);
	void CopyBufferToImage(vk::Buffer buffer, vk::Image image, vk::ImageLayout dstImagelayout, uint32_t width, uint32_t height);
	void CreateImageView(vk::Image image, vk::ImageView& view, vk::Format format, vk::ImageViewType viewType);
	void CreateSampler();

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
	vk::DescriptorSetLayout m_DescriptorSetLayout;
	vk::PipelineLayout m_PipelineLayout;
	vk::Pipeline m_Pipeline;
	std::vector<vk::Framebuffer> m_FrameBuffers;
	vk::CommandPool m_CommandPool;
	std::vector<vk::CommandBuffer> m_CommandBuffers;
	std::vector<vk::Semaphore> m_ImageAvailableSemaphores;
	std::vector<vk::Semaphore> m_RenderFinishedSemaphores;
	std::vector<vk::Fence> m_InFlightFences;
	vk::Buffer m_VertexBuffer;
	vk::DeviceMemory m_VertexBufferMemory;
	vk::Buffer m_IndexBuffer;
	vk::DeviceMemory m_IndexBufferMemory;
	std::vector<vk::Buffer> m_UniformBuffers;
	std::vector<vk::DeviceMemory> m_UniformBufferMemory;
	std::vector<void*> m_UniformBufferMapped;

	std::vector<Vertex> m_Vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, { 0.0f, 1.0f }},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, { 1.0f, 1.0f }},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, { 1.0f, 0.0f }},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, { 0.0f, 0.0f }}
	};
	std::vector<uint16_t> m_Indices = { 0, 1, 2, 2, 3, 0 };
	uint32_t m_CurrentFrame = 0;
	vk::DescriptorPool m_DescriptorPool;
	std::vector<vk::DescriptorSet> m_DescriptorSets;

	vk::Image m_Image;
	vk::DeviceMemory  m_Memory;
	vk::ImageView m_View;
	vk::Sampler m_Sampler;
};