#include <vector>
#include <set>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <stb_image.h>

#include "Application.h"
#include "../utils/readFile.h"

static const uint32_t MAX_FRAME_IN_FLIGHT = 2;
const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

void Application::InitWindow()
{
	if (!glfwInit())
	{
		std::cout << "init glfw failed!" << std::endl;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	m_Window = glfwCreateWindow(1024, 768, "Vulkan", nullptr, nullptr);
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
		DrawFrame();
	}
}

void Application::Cleanup()
{
	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		m_LogicDevice.destroySemaphore(m_ImageAvailableSemaphores[i]);
		m_LogicDevice.destroySemaphore(m_RenderFinishedSemaphores[i]);
		m_LogicDevice.destroyFence(m_InFlightFences[i]);
	}
	m_LogicDevice.destroyCommandPool(m_CommandPool);
	
	for (auto& fb : m_FrameBuffers)
	{
		m_LogicDevice.destroyFramebuffer(fb);
	}
	m_LogicDevice.destroyPipeline(m_Pipeline);
	m_LogicDevice.destroyPipelineLayout(m_PipelineLayout);
	m_LogicDevice.destroyRenderPass(m_Renderpass);

	for (auto& imageView : m_ImageViews)
	{
		m_LogicDevice.destroyImageView(imageView);
	}
	m_LogicDevice.destroySwapchainKHR(m_SwapChain);

	m_LogicDevice.destroyImage(m_Image);
	m_LogicDevice.freeMemory(m_Memory);

	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		m_LogicDevice.destroyBuffer(m_UniformBuffers[i]);
		m_LogicDevice.freeMemory(m_UniformBufferMemory[i]);
	}

	m_LogicDevice.destroyDescriptorSetLayout(m_DescriptorSetLayout);
	m_LogicDevice.destroyBuffer(m_IndexBuffer);
	m_LogicDevice.freeMemory(m_IndexBufferMemory);
	m_LogicDevice.destroyBuffer(m_VertexBuffer);
	m_LogicDevice.freeMemory(m_VertexBufferMemory);	

	m_LogicDevice.destroy();
	m_Vkinstance.destroySurfaceKHR(m_Surface);
	m_Vkinstance.destroy();
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Application::InitVulkan()
{
	CreateInstance();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicDevice();
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	createDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateFrameBuffer();
	CreateCommandPool();
	CreateImageTexture();
	CreateSampler();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSets();
	CreateCommandBuffer();
	CreateSyncObjects();
}

void Application::CreateInstance()
{
	vk::ApplicationInfo appInfo{};
	appInfo.sType = vk::StructureType::eApplicationInfo;
	appInfo.setPApplicationName("vulkanTest")
		   .setApiVersion(VK_API_VERSION_1_3);

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	vk::InstanceCreateInfo instanceInfo{};
	instanceInfo.sType = vk::StructureType::eInstanceCreateInfo;
	instanceInfo.setPApplicationInfo(&appInfo)
			    .setEnabledExtensionCount(extensions.size())
			    .setPpEnabledExtensionNames(extensions.data());

	if (CheckValidationLayerSupport())
	{
		//default enalbe validationLayer
		instanceInfo.setEnabledLayerCount(validationLayers.size())
					.setPpEnabledLayerNames(validationLayers.data());
	}
	if (vk::createInstance(&instanceInfo, nullptr, &m_Vkinstance) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

bool Application::CheckValidationLayerSupport()
{
	auto availableLayers = vk::enumerateInstanceLayerProperties();
	for (auto& validationLayer : validationLayers)
	{
		bool layFound = false;
		for (auto& layer : availableLayers)
		{
			if (strcmp(validationLayer, layer.layerName) == 0)
			{
				layFound = true;
				break;
			}
		}
		if (!layFound)
		{
			return false;
		}
	}
	return true;
}

void Application::PickPhysicalDevice()
{
	uint32_t phisicalDeviceCount = 0;
	auto devices = m_Vkinstance.enumeratePhysicalDevices();
	
	if (devices.size() == 0)
	{
		throw std::runtime_error("can not found device!");
	}


	for (auto& device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			m_PhyiscalDevice = device;
		}
	}
	if (!m_PhyiscalDevice)
	{
		throw std::runtime_error("can not found stuitable device!");
	}
}

bool Application::IsDeviceSuitable(const vk::PhysicalDevice& device)
{
	bool isDeviceExtensionSupport = false;

	vk::PhysicalDeviceProperties properties = device.getProperties();
	vk::PhysicalDeviceFeatures features = device.getFeatures();
	
	QueueFamilyIndices indices = FindQueueFamilies(device);
	
	isDeviceExtensionSupport = IsDeviceExtensionSupport(device);
	SwapChainSupportDetail swapChainsupport = QuerySwapChainSupport(device);


	return  indices.IsComplete() && 
			isDeviceExtensionSupport &&
			swapChainsupport &&
			properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
		    features.geometryShader;
}

Application::QueueFamilyIndices Application::FindQueueFamilies(const vk::PhysicalDevice& device)
{
	QueueFamilyIndices indices;
	auto properties = device.getQueueFamilyProperties();
	int i = 0;
	
	for (auto& property : properties)
	{		
		if (property.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.GraphicFamily = i;
		}

		VkBool32 presentSupport = false;
		device.getSurfaceSupportKHR(i, m_Surface, &presentSupport);
		
		if (presentSupport)
		{
			indices.PresentFamily = i;
		}

		if (indices.IsComplete())
		{
			break;
		}
		i++;
	}
	return indices;
}

void Application::CreateLogicDevice()
{
	float queuePriority = 1.0f;
	QueueFamilyIndices indices = FindQueueFamilies(m_PhyiscalDevice);
	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicFamily.value(), indices.PresentFamily.value() };
	for (auto& queueIndex : uniqueQueueFamilies)
	{
		vk::DeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
		queueCreateInfo.setQueueFamilyIndex(queueIndex)
				       .setQueueCount(1)
				       .setPQueuePriorities(&queuePriority);
		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures{};

	vk::DeviceCreateInfo createInfo{};
	createInfo.sType = vk::StructureType::eDeviceCreateInfo;
	createInfo.setQueueCreateInfoCount(static_cast<uint32_t>(uniqueQueueFamilies.size()))
			  .setPQueueCreateInfos(queueCreateInfos.data())
			  .setPEnabledFeatures(&deviceFeatures)
			  .setEnabledExtensionCount(static_cast<uint32_t>(m_DeviceExtesions.size()))
			  .setPpEnabledExtensionNames(m_DeviceExtesions.data())
			  .setEnabledLayerCount(0);

	m_LogicDevice = m_PhyiscalDevice.createDevice(createInfo);
	if (!m_LogicDevice)
	{
		throw std::runtime_error("failed to create logical device!");
	}
	m_GraphicQueue = m_LogicDevice.getQueue(indices.GraphicFamily.value(), 0);
	m_PresentQueue = m_LogicDevice.getQueue(indices.PresentFamily.value(), 0);
}

void Application::CreateSurface()
{
	vk::Win32SurfaceCreateInfoKHR surfaceInfo{};
	surfaceInfo.sType = vk::StructureType::eWin32SurfaceCreateInfoKHR;
	surfaceInfo.setHwnd(glfwGetWin32Window(m_Window))
		.setHinstance(GetModuleHandle(nullptr));
	if (m_Vkinstance.createWin32SurfaceKHR(&surfaceInfo, nullptr, &m_Surface) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

bool Application::IsDeviceExtensionSupport(const vk::PhysicalDevice& device)
{
	std::set<std::string> requiredExtensions(m_DeviceExtesions.begin(), m_DeviceExtesions.end());
	auto availableExtesions = device.enumerateDeviceExtensionProperties();
	for (auto& extension : availableExtesions)
	{
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

Application::SwapChainSupportDetail Application::QuerySwapChainSupport(const vk::PhysicalDevice& device)
{
	SwapChainSupportDetail swapChainDetail;
	swapChainDetail.capabilities = device.getSurfaceCapabilitiesKHR(m_Surface);
	swapChainDetail.formats = device.getSurfaceFormatsKHR(m_Surface);
	swapChainDetail.presentModes = device.getSurfacePresentModesKHR(m_Surface);
	return swapChainDetail;
}

void Application::CreateSwapChain()
{
	SwapChainSupportDetail detail = QuerySwapChainSupport(m_PhyiscalDevice);
	vk::SurfaceFormatKHR format = ChooseSwapSurfaceFormat(detail.formats);
	vk::PresentModeKHR mode = ChooseSwapSurfacePresentMode(detail.presentModes);
	vk::Extent2D extent = ChooseSwapExtent(detail.capabilities);

	uint32_t imageCount = detail.capabilities.minImageCount + 1;
	if (detail.capabilities.maxImageCount > 0 && imageCount > detail.capabilities.maxImageCount)
	{
		imageCount = detail.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapChainInfo{};
	swapChainInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
	swapChainInfo.setSurface(m_Surface)
				 .setPresentMode(mode)
				 .setMinImageCount(imageCount)
				 .setImageExtent(extent)
				 .setImageFormat(format.format)
				 .setImageColorSpace(format.colorSpace)
				 .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
				 .setImageArrayLayers(1)
				 .setPreTransform(detail.capabilities.currentTransform)
				 .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
				 .setClipped(VK_TRUE)
				 .setOldSwapchain(VK_NULL_HANDLE);

	QueueFamilyIndices indices = FindQueueFamilies(m_PhyiscalDevice);
	uint32_t queueIndices[] = {indices.GraphicFamily.value(), indices.PresentFamily.value()};
	if (indices.GraphicFamily != indices.PresentFamily)
	{
		swapChainInfo.setImageSharingMode(vk::SharingMode::eConcurrent)
			.setPQueueFamilyIndices(queueIndices)
			.setQueueFamilyIndexCount(2);
	}
	else 
	{
		swapChainInfo.setImageSharingMode(vk::SharingMode::eExclusive);
	}
	
	if (m_LogicDevice.createSwapchainKHR(&swapChainInfo, nullptr, &m_SwapChain) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	m_SwapChainImages = m_LogicDevice.getSwapchainImagesKHR(m_SwapChain);
	m_SwapChainFormat = format.format;
	m_SwapChainExtent = extent;
}

vk::SurfaceFormatKHR Application::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& formats)
{
	vk::SurfaceFormatKHR res = formats[0];
	for (const auto& format : formats)
	{
		if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			res = format;
			break;
		}
	}
	return res;
}

vk::PresentModeKHR Application::ChooseSwapSurfacePresentMode(const std::vector<vk::PresentModeKHR>& presentModes)
{
	vk::PresentModeKHR res = vk::PresentModeKHR::eFifo;
	for (const auto& mode : presentModes)
	{
		if (mode == vk::PresentModeKHR::eMailbox)
		{
			res = mode;
			break;
		}
	}
	return res;
}

vk::Extent2D Application::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) 
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		vk::Extent2D actualExtent =
		{
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actualExtent;
	}
}

void Application::CreateImageViews()
{
	m_ImageViews.resize(m_SwapChainImages.size());
	for (uint32_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		CreateImageView(m_SwapChainImages[i], m_ImageViews[i], m_SwapChainFormat, vk::ImageViewType::e2D);
		
	}
}

void Application::CreateGraphicsPipeline()
{
	#pragma region shader
	std::vector<char> vertexShaderCode    = ReadFile("resource/shaders/vert.spv");
	std::vector<char> fragmentShaderCode  = ReadFile("resource/shaders/frag.spv");
	vk::ShaderModule vertexShaderModule   = CreateShaderModule(vertexShaderCode);
	vk::ShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);
	vk::PipelineShaderStageCreateInfo vertexShaderInfo{};
	vertexShaderInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	vertexShaderInfo.setStage(vk::ShaderStageFlagBits::eVertex)
					.setModule(vertexShaderModule)
					.setPName("main");
	
	vk::PipelineShaderStageCreateInfo fragmentShaderInfo{};
	fragmentShaderInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
	fragmentShaderInfo.setStage(vk::ShaderStageFlagBits::eFragment)
					  .setModule(fragmentShaderModule)
					  .setPName("main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertexShaderInfo, fragmentShaderInfo };
	#pragma endregion

	#pragma region vertexInput
	vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
	vk::VertexInputBindingDescription vertexBinding = Vertex::GetBindingDescription();
	std::vector<vk::VertexInputAttributeDescription> vertexAttributes = Vertex::GetAttribuDescription();
	vertexInputInfo.setVertexBindingDescriptionCount(1)
				   .setPVertexBindingDescriptions(&vertexBinding)
				   .setVertexAttributeDescriptionCount(static_cast<uint32_t>(vertexAttributes.size()))
				   .setPVertexAttributeDescriptions(vertexAttributes.data());
	#pragma endregion

	#pragma region inputAssembly
	vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
	inputAssemblyInfo.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
	inputAssemblyInfo.setTopology(vk::PrimitiveTopology::eTriangleList)
			         .setPrimitiveRestartEnable(VK_FALSE);
	#pragma endregion

	#pragma region viewport
	vk::PipelineViewportStateCreateInfo viewportInfo{};
	viewportInfo.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
	viewportInfo.setViewportCount(1)
			    .setScissorCount(1);
	#pragma endregion

	#pragma region rasterizer
	vk::PipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
	rasterizer.setDepthClampEnable(VK_FALSE)
			  .setRasterizerDiscardEnable(VK_FALSE)
			  .setPolygonMode(vk::PolygonMode::eFill)
			  .setLineWidth(1.0f)
			  .setCullMode(vk::CullModeFlagBits::eBack)
			  .setFrontFace(vk::FrontFace::eCounterClockwise)
			  .setDepthBiasEnable(VK_FALSE);
	#pragma endregion

	#pragma region multisamples
	vk::PipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
	multisampling.setSampleShadingEnable(VK_FALSE)
				 .setRasterizationSamples(vk::SampleCountFlagBits::e1);
	#pragma endregion

	#pragma region blending
	vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
						.setBlendEnable(VK_FALSE);

	std::array<float, 4> blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f };
	vk::PipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
	colorBlending.setLogicOpEnable(VK_FALSE)
				 .setLogicOp(vk::LogicOp::eCopy)
				 .setAttachmentCount(1)
				 .setPAttachments(&colorBlendAttachment)
				 .setBlendConstants(blendConstants);
	#pragma endregion

	#pragma region dynamicState
	std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor
	};
	vk::PipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = vk::StructureType::ePipelineDynamicStateCreateInfo;
	dynamicState.setDynamicStateCount(dynamicStates.size())
		        .setPDynamicStates(dynamicStates.data());
	#pragma endregion

	#pragma region layout
	vk::PipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
	layoutInfo.setSetLayoutCount(1)
			  .setPSetLayouts(&m_DescriptorSetLayout)
		      .setPushConstantRangeCount(0);
	if (m_LogicDevice.createPipelineLayout(&layoutInfo, nullptr, &m_PipelineLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
	#pragma endregion	  

	#pragma region pipeline
	vk::GraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
	pipelineInfo.setStageCount(2)
				.setPStages(shaderStages)
				.setPVertexInputState(&vertexInputInfo)
				.setPInputAssemblyState(&inputAssemblyInfo)
				.setPViewportState(&viewportInfo)
				.setPRasterizationState(&rasterizer)
				.setPMultisampleState(&multisampling)
				.setPDepthStencilState(nullptr)
				.setPColorBlendState(&colorBlending)
				.setPDynamicState(&dynamicState)
				.setLayout(m_PipelineLayout)
				.setRenderPass(m_Renderpass)
				.setSubpass(0)
				.setBasePipelineHandle(VK_NULL_HANDLE)
				.setBasePipelineIndex(-1);
	
	if (m_LogicDevice.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &m_Pipeline) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	#pragma endregion

	m_LogicDevice.destroyShaderModule(vertexShaderModule, nullptr);
	m_LogicDevice.destroyShaderModule(fragmentShaderModule, nullptr);
}

vk::ShaderModule Application::CreateShaderModule(const std::vector<char>& sourceCode)
{
	vk::ShaderModuleCreateInfo shaderModuleInfo{};
	shaderModuleInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
	shaderModuleInfo.setPCode(reinterpret_cast<const uint32_t*>(sourceCode.data()))
			        .setCodeSize(sourceCode.size());

	vk::ShaderModule shaderModule;
	
	if (m_LogicDevice.createShaderModule(&shaderModuleInfo, nullptr, &shaderModule) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void Application::CreateRenderPass()
{
	vk::AttachmentDescription colorAttachment{};
	colorAttachment.setFormat(m_SwapChainFormat)
				   .setSamples(vk::SampleCountFlagBits::e1)
				   .setLoadOp(vk::AttachmentLoadOp::eClear)
				   .setStoreOp(vk::AttachmentStoreOp::eStore)
				   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				   .setInitialLayout(vk::ImageLayout::eUndefined)
				   .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
	
	vk::AttachmentReference colorAttachmentRef{};
	colorAttachmentRef.setAttachment(0)
			          .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpassInfo{};
	subpassInfo.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			   .setColorAttachmentCount(1)
			   .setPColorAttachments(&colorAttachmentRef);
	
	vk::SubpassDependency dependencyInfo{};
	
	dependencyInfo.setSrcSubpass(VK_SUBPASS_EXTERNAL)
				  .setDstSubpass(0)
				  .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				  .setSrcAccessMask(vk::AccessFlagBits::eNone)
				  .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				  .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

	vk::RenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = vk::StructureType::eRenderPassCreateInfo;
	renderPassInfo.setAttachmentCount(1)
			      .setPAttachments(&colorAttachment)
			      .setSubpassCount(1)
			      .setPSubpasses(&subpassInfo)
			      .setDependencyCount(1)
			      .setPDependencies(&dependencyInfo);
	
	if (m_LogicDevice.createRenderPass(&renderPassInfo, nullptr, &m_Renderpass) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void Application::CreateFrameBuffer()
{
	m_FrameBuffers.resize(m_ImageViews.size());
	for (uint32_t i = 0; i < m_ImageViews.size(); i++)
	{
		vk::ImageView attachments[] = {
			m_ImageViews[i]
		};
		vk::FramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = vk::StructureType::eFramebufferCreateInfo;
		framebufferInfo.setRenderPass(m_Renderpass)
					   .setAttachmentCount(1)
					   .setPAttachments(attachments)
					   .setWidth(m_SwapChainExtent.width)
					   .setHeight(m_SwapChainExtent.height)
					   .setLayers(1);		
		if (m_LogicDevice.createFramebuffer(&framebufferInfo, nullptr, &m_FrameBuffers[i]) != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}	
}

void Application::CreateCommandPool()
{
	vk::CommandPoolCreateInfo commandpoolInfo{};
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhyiscalDevice);
	commandpoolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
	commandpoolInfo.setQueueFamilyIndex(queueFamilyIndices.GraphicFamily.value())
			       .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	if (m_LogicDevice.createCommandPool(&commandpoolInfo, nullptr, &m_CommandPool) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void Application::CreateCommandBuffer()
{
	vk::CommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	commandBufferAllocInfo.setCommandPool(m_CommandPool)
						  .setCommandBufferCount(1)
						  .setLevel(vk::CommandBufferLevel::ePrimary);
	m_CommandBuffers.resize(MAX_FRAME_IN_FLIGHT);
	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		if (m_LogicDevice.allocateCommandBuffers(&commandBufferAllocInfo, &m_CommandBuffers[i]) != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}
}

void Application::RecordCommandBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex)
{
	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;	
	if (commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}
		vk::Rect2D renderArea;
		renderArea.setExtent(m_SwapChainExtent)
			      .setOffset(vk::Offset2D(0, 0));

		vk::ClearValue clearColor;
		
		vk::RenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType = vk::StructureType::eRenderPassBeginInfo;
		renderPassBeginInfo.setRenderPass(m_Renderpass)
						   .setFramebuffer(m_FrameBuffers[imageIndex])
						   .setRenderArea(renderArea)
						   .setClearValueCount(1)
						   .setPClearValues(&clearColor);

		commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);
			commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
			vk::Viewport viewport{};
			viewport.setX(0.0f)
					.setY(0.0f)
					.setWidth((float)m_SwapChainExtent.width)
					.setHeight((float)m_SwapChainExtent.height)
					.setMinDepth(0.0f)
					.setMaxDepth(1.0f);
			
			commandBuffer.setViewport(0, 1, &viewport);

			vk::Rect2D scissor{};
			scissor.setOffset(vk::Offset2D(0, 0))
				   .setExtent(m_SwapChainExtent);

			commandBuffer.setScissor(0, 1, &scissor);

			vk::Buffer vertexBuffers[] = { m_VertexBuffer };
			vk::DeviceSize offsets[] = { 0 };
			commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
			commandBuffer.bindIndexBuffer(m_IndexBuffer, 0, vk::IndexType::eUint16);
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineLayout, 0, 1, &m_DescriptorSets[m_CurrentFrame], 0, nullptr);
			commandBuffer.drawIndexed(static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
			//commandBuffer.draw(m_Vertices.size(), 1, 0, 0);
		
		commandBuffer.endRenderPass();

	commandBuffer.end();
}

void Application::CreateSyncObjects()
{
	vk::SemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = vk::StructureType::eSemaphoreCreateInfo;
	vk::FenceCreateInfo fenceInfo{};
	fenceInfo.sType = vk::StructureType::eFenceCreateInfo;
	fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);

	m_CommandBuffers.resize(MAX_FRAME_IN_FLIGHT);
	m_ImageAvailableSemaphores.resize(MAX_FRAME_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAME_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAME_IN_FLIGHT);
		
	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		if (m_LogicDevice.createSemaphore(&semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != vk::Result::eSuccess ||
			m_LogicDevice.createSemaphore(&semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != vk::Result::eSuccess ||
			m_LogicDevice.createFence(&fenceInfo, nullptr, &m_InFlightFences[i]) != vk::Result::eSuccess
			)
		{
			throw std::runtime_error("failed to create semaphores!");
		}
	}
}

void Application::DrawFrame()
{
	m_LogicDevice.waitForFences(1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
	uint32_t imageIndex;
	m_LogicDevice.acquireNextImageKHR(m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
	UploadUniformBuffer(m_CurrentFrame);
	m_LogicDevice.resetFences(1, &m_InFlightFences[m_CurrentFrame]);
	m_CommandBuffers[m_CurrentFrame].reset();
	RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);

	vk::SubmitInfo submitInfo{};
	vk::Semaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame]};
	vk::Semaphore signadSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame]};
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&m_CommandBuffers[m_CurrentFrame])
			  .setWaitSemaphoreCount(1)
			  .setPWaitSemaphores(waitSemaphores)
			  .setSignalSemaphoreCount(1)
			  .setPSignalSemaphores(signadSemaphores)
			  .setPWaitDstStageMask(waitStages);
	
	if (m_GraphicQueue.submit(1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}	

	vk::SwapchainKHR swapChains[] = { m_SwapChain };
	vk::PresentInfoKHR presentInfo{};
	presentInfo.sType = vk::StructureType::ePresentInfoKHR;
	presentInfo.setPImageIndices(&imageIndex)
			   .setSwapchainCount(1)
			   .setPSwapchains(swapChains)
			   .setWaitSemaphoreCount(1)
			   .setPWaitSemaphores(signadSemaphores)
			   .setPResults(nullptr);
	
	if (m_PresentQueue.presentKHR(&presentInfo) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to present Image!");
	}
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAME_IN_FLIGHT;
}

void Application::CreateVertexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
	void* data;
	m_LogicDevice.mapMemory(stagingBufferMemory, 0, bufferSize, {}, &data);
	memcpy(data, m_Vertices.data(), (size_t)bufferSize);
	m_LogicDevice.unmapMemory(stagingBufferMemory);

	CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_VertexBuffer, m_VertexBufferMemory);
	
	CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);
	m_LogicDevice.destroyBuffer(stagingBuffer, nullptr);
	m_LogicDevice.freeMemory(stagingBufferMemory, nullptr);
}

void Application::CreateIndexBuffer()
{
	VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;
	CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingMemory);
	void* data;
	m_LogicDevice.mapMemory(stagingMemory, 0, bufferSize, {}, &data);
	memcpy(data, m_Indices.data(), (size_t)bufferSize);
	m_LogicDevice.unmapMemory(stagingMemory);

	CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_IndexBuffer, m_IndexBufferMemory);
	CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);
	m_LogicDevice.destroyBuffer(stagingBuffer, nullptr);
	m_LogicDevice.freeMemory(stagingMemory, nullptr);
}

void Application::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::Buffer& buffer, vk::DeviceMemory& bufferMemory)
{
	vk::BufferCreateInfo bufferInfo{};
	bufferInfo.sType = vk::StructureType::eBufferCreateInfo;
	bufferInfo.setUsage(usage)
			  .setSize(size)
			  .setSharingMode(vk::SharingMode::eExclusive);
	if (m_LogicDevice.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create vertex buffer!");
	}
	vk::MemoryRequirements requirment;
	m_LogicDevice.getBufferMemoryRequirements(buffer, &requirment);

	vk::MemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	allocateInfo.setAllocationSize(requirment.size)
			    .setMemoryTypeIndex(FindMemoryType(requirment.memoryTypeBits, properties));
	if (m_LogicDevice.allocateMemory(&allocateInfo, nullptr, &bufferMemory) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}
	m_LogicDevice.bindBufferMemory(buffer, bufferMemory, 0);
}

uint32_t Application::FindMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags flags)
{
	 vk::PhysicalDeviceMemoryProperties properties;
	 m_PhyiscalDevice.getMemoryProperties(&properties);
	for (size_t i = 0; i < properties.memoryTypeCount; i++)
	{
		if ((typeFilter &  (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

void Application::CopyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
	vk::CommandBuffer commandBuffer;
	
	vk::CommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	commandBufferAllocInfo.setCommandPool(m_CommandPool)
					      .setCommandBufferCount(1)
					      .setLevel(vk::CommandBufferLevel::ePrimary);
	
	if (m_LogicDevice.allocateCommandBuffers(&commandBufferAllocInfo, &commandBuffer) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}


	vk::CommandBufferBeginInfo commandBeginInfo{};
	commandBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
	commandBeginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	if (commandBuffer.begin(&commandBeginInfo) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to begin command buffer!");
	}
		vk::BufferCopy copyRegion{};
		copyRegion.setDstOffset(0)
				  .setSrcOffset(0)
				  .setSize(size);
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);
	commandBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&commandBuffer);
	m_GraphicQueue.submit(1, &submitInfo, VK_NULL_HANDLE);
	m_GraphicQueue.waitIdle();

	m_LogicDevice.freeCommandBuffers(m_CommandPool, 1, &commandBuffer);
}

void Application::createDescriptorSetLayout()
{
	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	//uniform
	vk::DescriptorSetLayoutBinding uniformBinding{};
	uniformBinding.setBinding(0)
		   .setDescriptorCount(1)
		   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
		   .setStageFlags(vk::ShaderStageFlagBits::eVertex)
		   .setPImmutableSamplers(nullptr);
	bindings.push_back(uniformBinding);

	//sampler
	vk::DescriptorSetLayoutBinding samplerBinding{};
	samplerBinding.setBinding(1)
				  .setDescriptorCount(1)
				  .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				  .setStageFlags(vk::ShaderStageFlagBits::eFragment)
				  .setPImmutableSamplers(nullptr);
	bindings.push_back(samplerBinding);

	vk::DescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = vk::StructureType::eDescriptorSetLayoutCreateInfo;
	layoutInfo.setBindingCount(bindings.size())
		      .setPBindings(bindings.data());

	if (m_LogicDevice.createDescriptorSetLayout(&layoutInfo, nullptr, &m_DescriptorSetLayout) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Application::CreateUniformBuffers()
{
	vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
	m_UniformBuffers.resize(MAX_FRAME_IN_FLIGHT);
	m_UniformBufferMemory.resize(MAX_FRAME_IN_FLIGHT);
	m_UniformBufferMapped.resize(MAX_FRAME_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_UniformBuffers[i], m_UniformBufferMemory[i]);
		if (m_LogicDevice.mapMemory(m_UniformBufferMemory[i], 0, bufferSize, {}, &m_UniformBufferMapped[i]) != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to map uniformBuffer Memory!");
		}
	}
}

void Application::UploadUniformBuffer(uint32_t currentImage)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.0f);
	ubo.projection[1][1] *= -1;
	memcpy(m_UniformBufferMapped[currentImage], &ubo, sizeof(ubo));
}

void Application::CreateDescriptorPool()
{
	vk::DescriptorPoolSize poolSize{};
	poolSize.setType(vk::DescriptorType::eUniformBuffer)
			.setDescriptorCount(static_cast<uint32_t>(MAX_FRAME_IN_FLIGHT));

	vk::DescriptorPoolSize samplerPool{};
	samplerPool.setType(vk::DescriptorType::eCombinedImageSampler)
			  .setDescriptorCount(static_cast<uint32_t>(MAX_FRAME_IN_FLIGHT));
	std::vector<vk::DescriptorPoolSize> pool;
	pool.push_back(poolSize);
	pool.push_back(samplerPool);

	vk::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = vk::StructureType::eDescriptorPoolCreateInfo;
	poolInfo.setPoolSizeCount(pool.size())
			.setPPoolSizes(pool.data())
			.setMaxSets(static_cast<uint32_t>(MAX_FRAME_IN_FLIGHT));
	if (m_LogicDevice.createDescriptorPool(&poolInfo, nullptr, &m_DescriptorPool) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

void Application::CreateDescriptorSets()
{
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAME_IN_FLIGHT, m_DescriptorSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = vk::StructureType::eDescriptorSetAllocateInfo;
	allocInfo.setDescriptorPool(m_DescriptorPool)
			 .setDescriptorSetCount(MAX_FRAME_IN_FLIGHT)
			 .setPSetLayouts(layouts.data());

	m_DescriptorSets.resize(MAX_FRAME_IN_FLIGHT);
	if (m_LogicDevice.allocateDescriptorSets(&allocInfo, m_DescriptorSets.data()) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAME_IN_FLIGHT; i++)
	{
		std::vector<vk::WriteDescriptorSet> writes;

		vk::DescriptorBufferInfo bufferInfo{};
		bufferInfo.setBuffer(m_UniformBuffers[i])
				  .setOffset(0)
				  .setRange(sizeof(UniformBufferObject));

		vk::WriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = vk::StructureType::eWriteDescriptorSet;
		descriptorWrite.setDstSet(m_DescriptorSets[i])
					   .setDstBinding(0)
					   .setDstArrayElement(0)
					   .setDescriptorType(vk::DescriptorType::eUniformBuffer)
					   .setDescriptorCount(1)
					   .setPBufferInfo(&bufferInfo)
					   .setPImageInfo(nullptr)
					   .setPTexelBufferView(nullptr);
		writes.push_back(descriptorWrite);

		vk::DescriptorImageInfo imageInfo{};
		imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				 .setImageView(m_View)
				 .setSampler(m_Sampler);
		vk::WriteDescriptorSet samplerDescriptorWrite{};
		samplerDescriptorWrite.sType = vk::StructureType::eWriteDescriptorSet;
		samplerDescriptorWrite.setDstSet(m_DescriptorSets[i])
							  .setDstBinding(1)
							  .setDstArrayElement(0)
							  .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
							  .setDescriptorCount(1)
							  .setPImageInfo(&imageInfo);
		writes.push_back(samplerDescriptorWrite);

		m_LogicDevice.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);
	}
}

void Application::CreateImageTexture()
{
	//load image
	int width, height, channels;
	stbi_uc* pixels = stbi_load("resource/textures/texture.jpg", &width, &height, &channels, STBI_rgb_alpha);
	if (!pixels)
	{
		throw std::runtime_error("load image failed!");
	}

	//localBuffer
	vk::DeviceSize bufferSize = width * height * 4;
	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingMemory;
	CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingMemory);
	void* data;
	if (m_LogicDevice.mapMemory(stagingMemory, 0, bufferSize, {}, &data) != vk::Result::eSuccess)
	{
		throw std::runtime_error("mapMemory failed!");
	}
	memcpy(data, pixels, bufferSize);
	m_LogicDevice.unmapMemory(stagingMemory);

	stbi_image_free(pixels);

	//GpuImage
	CreateImage(width, height, vk::Format::eR8G8B8A8Srgb);

	//transiation undefined -> transferSrc 
	TransiationImageLayout(m_Image, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eNone, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer);

	//copyBufferToImage
	CopyBufferToImage(stagingBuffer, m_Image, vk::ImageLayout::eTransferDstOptimal, width, height);

	//transiation transferSrc -> shder-readonly
	TransiationImageLayout(m_Image, vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader);

	m_LogicDevice.destroyBuffer(stagingBuffer);
	m_LogicDevice.freeMemory(stagingMemory);

	//imageView
	CreateImageView(m_Image, m_View, vk::Format::eR8G8B8A8Srgb, vk::ImageViewType::e2D);

}

void Application::CreateImage(uint32_t width, uint32_t height, vk::Format format)
{
	vk::Extent3D extentInfo{};
	extentInfo.setWidth(width)
			  .setHeight(height)
			  .setDepth(1);

	vk::ImageCreateInfo imageInfo{};
	imageInfo.sType = vk::StructureType::eImageCreateInfo;
	imageInfo.setArrayLayers(1)
			 .setExtent(extentInfo)
			 .setFormat(format)
			 .setImageType(vk::ImageType::e2D)
			 .setInitialLayout(vk::ImageLayout::eUndefined)
			 .setMipLevels(1)
			 .setSamples(vk::SampleCountFlagBits::e1)
			 .setSharingMode(vk::SharingMode::eExclusive)
			 .setTiling(vk::ImageTiling::eOptimal)
			 .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	if (m_LogicDevice.createImage(&imageInfo, nullptr, &m_Image) != vk::Result::eSuccess)
	{
		throw std::runtime_error("createImage failed!");
	}

	vk::MemoryRequirements requirments;
	m_LogicDevice.getImageMemoryRequirements(m_Image, &requirments);
	vk::MemoryAllocateInfo memoryInfo{};
	memoryInfo.sType = vk::StructureType::eMemoryAllocateInfo;
	memoryInfo.setAllocationSize(requirments.size)
			  .setMemoryTypeIndex(FindMemoryType(requirments.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
	if (m_LogicDevice.allocateMemory(&memoryInfo, nullptr, &m_Memory) != vk::Result::eSuccess)
	{
		throw std::runtime_error("allocate memory failed!");
	}
	m_LogicDevice.bindImageMemory(m_Image, m_Memory, 0);
}

vk::CommandBuffer Application::BeginOneTimeCommand()
{
	vk::CommandBuffer commandBuffer;
	vk::CommandBufferAllocateInfo bufferInfo{};
	bufferInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	bufferInfo.setCommandBufferCount(1)
			  .setLevel(vk::CommandBufferLevel::ePrimary)
			  .setCommandPool(m_CommandPool);
	if (m_LogicDevice.allocateCommandBuffers(&bufferInfo, &commandBuffer) != vk::Result::eSuccess)
	{
		throw std::runtime_error("allocate commandBufer failed!");
	}

	vk::CommandBufferBeginInfo beginInfo{};
	beginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;
	beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	if (commandBuffer.begin(&beginInfo) != vk::Result::eSuccess)
	{
		throw std::runtime_error("begin commandBufer failed!");
	}
	return commandBuffer;
}

void Application::EndCommand(vk::CommandBuffer commandBuffer)
{
	commandBuffer.end();

	vk::SubmitInfo submitInfo{};
	submitInfo.sType = vk::StructureType::eCommandBufferSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&commandBuffer);
	if (m_GraphicQueue.submit(1, &submitInfo, {}) != vk::Result::eSuccess)
	{
		throw std::runtime_error("submit commandBuffer failed!");
	}
	m_GraphicQueue.waitIdle();
	m_LogicDevice.freeCommandBuffers(m_CommandPool, 1, &commandBuffer);
}

void Application::TransiationImageLayout(const vk::Image& image, vk::AccessFlags distFlags, vk::AccessFlags srcFlags, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::PipelineStageFlags srcStage, vk::PipelineStageFlags dstStage)
{
	vk::CommandBuffer command = BeginOneTimeCommand();

	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
			        .setBaseArrayLayer(0)
			        .setBaseMipLevel(0)
			        .setLayerCount(1)
			        .setLevelCount(1);

	vk::ImageMemoryBarrier barrierInfo{};;
	barrierInfo.sType = vk::StructureType::eImageMemoryBarrier;
	
	barrierInfo.setImage(image)
			   .setDstAccessMask(distFlags)
			   .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			   .setNewLayout(newLayout)
			   .setSrcAccessMask(srcFlags)
			   .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			   .setOldLayout(oldLayout)
			   .setSubresourceRange(subresourceRange);
	command.pipelineBarrier(srcStage, dstStage, vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrierInfo);

	EndCommand(command);
}

void Application::CopyBufferToImage(vk::Buffer buffer, vk::Image image, vk::ImageLayout dstImagelayout, uint32_t width, uint32_t height)
{
	vk::CommandBuffer command = BeginOneTimeCommand();
	vk::ImageSubresourceLayers layer;
	layer.setAspectMask(vk::ImageAspectFlagBits::eColor)
	     .setMipLevel(0)
	     .setBaseArrayLayer(0)
	     .setLayerCount(1);
	vk::BufferImageCopy copyInfo{};
	copyInfo.setBufferImageHeight(0)
			.setBufferOffset(0)
			.setBufferRowLength(0)
			.setImageExtent(vk::Extent3D(width, height, 1))
			.setImageOffset(vk::Offset3D(0, 0, 0))
			.setImageSubresource(layer);
	command.copyBufferToImage(buffer, image, dstImagelayout, 1, &copyInfo);
	EndCommand(command);
}

void Application::CreateImageView(vk::Image image, vk::ImageView& view,  vk::Format format, vk::ImageViewType viewType)
{
	vk::ImageSubresourceRange region{};
	region.setAspectMask(vk::ImageAspectFlagBits::eColor)
		  .setBaseArrayLayer(0)
		  .setBaseMipLevel(0)
		  .setLayerCount(1)
		  .setLevelCount(1);

	vk::ImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = vk::StructureType::eImageViewCreateInfo;
	imageViewInfo.setComponents(vk::ComponentMapping())
				 .setFormat(format)
				 .setImage(image)
				 .setViewType(vk::ImageViewType::e2D)
				 .setSubresourceRange(region);
	if (m_LogicDevice.createImageView(&imageViewInfo, nullptr, &view) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create imageView failed!");
	}
}

void Application::CreateSampler()
{
	vk::SamplerCreateInfo samplerInfo{};
	samplerInfo.sType = vk::StructureType::eSamplerCreateInfo;
	samplerInfo.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			   .setAddressModeV(vk::SamplerAddressMode::eRepeat)
			   .setAddressModeW(vk::SamplerAddressMode::eRepeat)
			   .setAnisotropyEnable(false)
			   .setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
			   .setCompareEnable(false)
			   .setCompareOp(vk::CompareOp::eAlways)
			   .setMagFilter(vk::Filter::eLinear)
			   .setMinFilter(vk::Filter::eLinear)
			   .setMipmapMode(vk::SamplerMipmapMode::eLinear)
			   .setUnnormalizedCoordinates(false);
	if (m_LogicDevice.createSampler(&samplerInfo, nullptr, &m_Sampler) != vk::Result::eSuccess)
	{
		throw std::runtime_error("create Sampler failed!");
	}
}