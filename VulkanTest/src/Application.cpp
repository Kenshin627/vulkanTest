#include <vector>
#include <set>
#include <iostream>
#include <limits>
#include <algorithm>
#include <cstdint>

#include "Application.h"
#include "../utils/readFile.h"

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
	m_LogicDevice.destroySemaphore(m_ImageAvailableSemaphore);
	m_LogicDevice.destroySemaphore(m_RenderFinishedSemaphore);

	m_LogicDevice.destroyFence(m_InFlightFence);
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
	CreateGraphicsPipeline();
	CreateFrameBuffer();
	CreateCommandPool();
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateCommandBuffer(m_CommandBuffer);
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

	vk::InstanceCreateInfo instanceInfo{};
	instanceInfo.sType = vk::StructureType::eInstanceCreateInfo;
	instanceInfo.setPApplicationInfo(&appInfo)
			    .setEnabledExtensionCount(glfwExtensionCount)
			    .setPpEnabledExtensionNames(glfwExtensions)
			    .setEnabledLayerCount(0);
	if (vk::createInstance(&instanceInfo, nullptr, &m_Vkinstance) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to create instance!");
	}
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
		vk::ImageSubresourceRange subresource;
		subresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
				   .setBaseMipLevel(0)
				   .setLevelCount(1)
				   .setBaseArrayLayer(0)
				   .setLayerCount(1);

		vk::ImageViewCreateInfo imageViewInfo{};
		imageViewInfo.sType = vk::StructureType::eImageViewCreateInfo;
		imageViewInfo.setImage(m_SwapChainImages[i])
					 .setViewType(vk::ImageViewType::e2D)
					 .setFormat(m_SwapChainFormat)
					 .setComponents(vk::ComponentMapping())
					 .setSubresourceRange(subresource);

		if (m_LogicDevice.createImageView(&imageViewInfo, nullptr, &m_ImageViews[i]) != vk::Result::eSuccess)
		{
			throw std::runtime_error("failed to create image views!");
		}
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
			  .setFrontFace(vk::FrontFace::eClockwise)
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
	layoutInfo.setSetLayoutCount(0)
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

void Application::CreateCommandBuffer(vk::CommandBuffer& buffer)
{
	vk::CommandBufferAllocateInfo commandBufferAllocInfo{};
	commandBufferAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
	commandBufferAllocInfo.setCommandPool(m_CommandPool)
						  .setCommandBufferCount(1)
						  .setLevel(vk::CommandBufferLevel::ePrimary);
	if (m_LogicDevice.allocateCommandBuffers(&commandBufferAllocInfo, &buffer) != vk::Result::eSuccess)
	{
		throw std::runtime_error("failed to allocate command buffers!");
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
		
	
	if (m_LogicDevice.createSemaphore(&semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != vk::Result::eSuccess ||
		m_LogicDevice.createSemaphore(&semaphoreInfo, nullptr, &m_RenderFinishedSemaphore) != vk::Result::eSuccess ||
		m_LogicDevice.createFence(&fenceInfo, nullptr, &m_InFlightFence)  != vk::Result::eSuccess
		)
	{
		throw std::runtime_error("failed to create semaphores!");
	}
}

void Application::DrawFrame()
{
	m_LogicDevice.waitForFences(1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
	m_LogicDevice.resetFences(1, &m_InFlightFence);

	uint32_t imageIndex;
	m_LogicDevice.acquireNextImageKHR(m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	m_CommandBuffer.reset();
	RecordCommandBuffer(m_CommandBuffer, imageIndex);

	vk::SubmitInfo submitInfo{};
	vk::Semaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
	vk::Semaphore signadSemaphores[] = { m_RenderFinishedSemaphore };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.sType = vk::StructureType::eSubmitInfo;
	submitInfo.setCommandBufferCount(1)
			  .setPCommandBuffers(&m_CommandBuffer)
			  .setWaitSemaphoreCount(1)
			  .setPWaitSemaphores(waitSemaphores)
			  .setSignalSemaphoreCount(1)
			  .setPSignalSemaphores(signadSemaphores)
			  .setPWaitDstStageMask(waitStages);
	
	if (m_GraphicQueue.submit(1, &submitInfo, m_InFlightFence) != vk::Result::eSuccess)
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
	CreateCommandBuffer(commandBuffer);

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

