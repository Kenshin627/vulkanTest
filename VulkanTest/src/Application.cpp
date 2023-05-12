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
	vkDestroySemaphore(m_LogicDevice, m_ImageAvailableSemaphore, nullptr);
	vkDestroySemaphore(m_LogicDevice, m_RenderFinishedSemaphore, nullptr);
	vkDestroyFence(m_LogicDevice, m_InFlightFence, nullptr);
	vkDestroyCommandPool(m_LogicDevice, m_CommandPool, nullptr);
	for (auto& fb : m_FrameBuffers)
	{
		vkDestroyFramebuffer(m_LogicDevice, fb, nullptr);
	}
	vkDestroyPipeline(m_LogicDevice, m_Pipeline, nullptr);
	vkDestroyPipelineLayout(m_LogicDevice, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_LogicDevice, m_Renderpass, nullptr);
	for (auto& imageView : m_ImageViews)
	{
		vkDestroyImageView(m_LogicDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(m_LogicDevice, m_SwapChain, nullptr);
	vkDestroyBuffer(m_LogicDevice, m_VertexBuffer, nullptr);
	vkFreeMemory(m_LogicDevice, m_BufferMemory, nullptr);
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
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFrameBuffer();
	CreateCommandPool();
	CreateVertexBuffer();
	CreateCommandBuffer();
	CreateSyncObjects();
}

void Application::CreateInstance()
{
	vk::ApplicationInfo appInfo{};
	appInfo.sType = vk::StructureType::eApplicationInfo;
	appInfo.pApplicationName = "vulkanTest";

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	vk::InstanceCreateInfo createInfo{};
	createInfo.sType = vk::StructureType::eInstanceCreateInfo;
	createInfo.setPApplicationInfo(&appInfo)
		.setEnabledExtensionCount(glfwExtensionCount)
		.setPpEnabledExtensionNames(glfwExtensions)
		.setEnabledLayerCount(0);
	
	m_Vkinstance = vk::createInstance(createInfo);
	if (!m_Vkinstance)
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
	if (m_PhyiscalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("can not found stuitable device!");
	}
}

bool Application::IsDeviceSuitable(const vk::PhysicalDevice& device)
{
	bool isDeviceExtensionSupport = false;

	vk::PhysicalDeviceProperties properts = device.getProperties();
	vk::PhysicalDeviceFeatures features = device.getFeatures();
	
	QueueFamilyIndices indices = FindQueueFamilies(device);
	
	isDeviceExtensionSupport = IsDeviceExtensionSupport(device);
	SwapChainSupportDetail swapChainsupport = QuerySwapChainSupport(device);


	return  indices.IsComplete() && 
			isDeviceExtensionSupport &&
			swapChainsupport &&
		    properts.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
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
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(uniqueQueueFamilies.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtesions.size());
	createInfo.ppEnabledExtensionNames = m_DeviceExtesions.data();
	//validation layers
	createInfo.enabledLayerCount = 0;

	if (vkCreateDevice(m_PhyiscalDevice, &createInfo, nullptr, &m_LogicDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}
	vkGetDeviceQueue(m_LogicDevice, indices.GraphicFamily.value(), 0, &m_GraphicQueue);
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

bool Application::IsDeviceExtensionSupport(const vk::PhysicalDevice& device)
{
	uint32_t extensionCount = 0;
	std::set<std::string> requiredExtensions(m_DeviceExtesions.begin(), m_DeviceExtesions.end());

	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtesions;
	availableExtesions.resize(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtesions.data());

	for (auto& extension : availableExtesions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

Application::SwapChainSupportDetail Application::QuerySwapChainSupport(const VkPhysicalDevice& device)
{
	uint32_t formatCount = 0;
	uint32_t presentCount = 0;
	SwapChainSupportDetail swapChainDetails;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &swapChainDetails.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
	swapChainDetails.formats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, swapChainDetails.formats.data());
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentCount, nullptr);
	swapChainDetails.presentModes.resize(presentCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentCount, swapChainDetails.presentModes.data());
	return swapChainDetails;
}

void Application::CreateSwapChain()
{
	SwapChainSupportDetail detail = QuerySwapChainSupport(m_PhyiscalDevice);
	VkSurfaceFormatKHR format = ChooseSwapSurfaceFormat(detail.formats);
	VkPresentModeKHR mode = ChooseSwapSurfacePresentMode(detail.presentModes);
	VkExtent2D extent = ChooseSwapExtent(detail.capabilities);

	uint32_t imageCount = detail.capabilities.minImageCount + 1;
	if (detail.capabilities.maxImageCount > 0 && imageCount > detail.capabilities.maxImageCount)
	{
		imageCount = detail.capabilities.maxImageCount;
	}


	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.surface = m_Surface;
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.presentMode = mode;
	createInfo.minImageCount = imageCount;
	createInfo.imageExtent = extent;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.imageArrayLayers = 1;

	QueueFamilyIndices indices = FindQueueFamilies(m_PhyiscalDevice);
	uint32_t queueIndices[] = {indices.GraphicFamily.value(), indices.PresentFamily.value()};
	if (indices.GraphicFamily != indices.PresentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueIndices;
	}
	else 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//createInfo.queueFamilyIndexCount = 0;
		//createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = detail.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_LogicDevice, &createInfo, nullptr, &m_SwapChain) !=VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	uint32_t swapchainImageCount = 0;
	vkGetSwapchainImagesKHR(m_LogicDevice, m_SwapChain, &swapchainImageCount, nullptr);
	m_SwapChainImages.resize(swapchainImageCount);
	vkGetSwapchainImagesKHR(m_LogicDevice, m_SwapChain, &swapchainImageCount, m_SwapChainImages.data());
	m_SwapChainFormat = format.format;
	m_SwapChainExtent = extent;
}

VkSurfaceFormatKHR Application::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	VkSurfaceFormatKHR res = formats[0];
	for (const auto& format : formats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			res = format;
			break;
		}
	}
	return res;
}

VkPresentModeKHR Application::ChooseSwapSurfacePresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
	VkPresentModeKHR res = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			res = mode;
			break;
		}
	}
	return res;
}

VkExtent2D Application::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) 
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(m_Window, &width, &height);
		VkExtent2D actualExtent =
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
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_SwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_SwapChainFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		if (vkCreateImageView(m_LogicDevice, &createInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void Application::CreateGraphicsPipeline()
{
	auto vertexShaderCode = ReadFile("resource/shaders/vert.spv");
	auto fragmentShaderCode = ReadFile("resource/shaders/frag.spv");

	auto vertexShaderModule = CreateShaderModule(vertexShaderCode);
	auto fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.module = vertexShaderModule;
	vertexShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

	VkPipelineVertexInputStateCreateInfo vertexinputInfo{};
	vertexinputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	auto vertexBinding = Vertex::GetBindingDescription();
	auto vertexAttributes = Vertex::GetAttribuDescription();
	vertexinputInfo.vertexBindingDescriptionCount = 1;
	vertexinputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.size());
	vertexinputInfo.pVertexBindingDescriptions = &vertexBinding;
	vertexinputInfo.pVertexAttributeDescriptions = vertexAttributes.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = dynamicStates.size();
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayouyInfo{};
	pipelineLayouyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayouyInfo.setLayoutCount = 0;
	pipelineLayouyInfo.pushConstantRangeCount = 0;
	if (vkCreatePipelineLayout(m_LogicDevice, &pipelineLayouyInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexinputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;

	
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = m_Renderpass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_LogicDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(m_LogicDevice, vertexShaderModule, nullptr);
	vkDestroyShaderModule(m_LogicDevice, fragmentShaderModule, nullptr);
}

VkShaderModule Application::CreateShaderModule(const std::vector<char>& sourceCode)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.codeSize = sourceCode.size();
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(sourceCode.data());
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(m_LogicDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}

void Application::CreateRenderPass()
{
	//
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_SwapChainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	
	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(m_LogicDevice, &renderPassInfo, nullptr, &m_Renderpass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void Application::CreateFrameBuffer()
{
	m_FrameBuffers.resize(m_ImageViews.size());
	for (uint32_t i = 0; i < m_ImageViews.size(); i++)
	{
		VkImageView attachments[] = {
			m_ImageViews[i]
		};

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = m_Renderpass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = m_SwapChainExtent.width;
		createInfo.height = m_SwapChainExtent.height;
		createInfo.layers = 1;
		if (vkCreateFramebuffer(m_LogicDevice, &createInfo, nullptr, &m_FrameBuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}	
}

void Application::CreateCommandPool()
{
	VkCommandPoolCreateInfo createInfo{};
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhyiscalDevice);
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = queueFamilyIndices.GraphicFamily.value();
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_LogicDevice, &createInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void Application::CreateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(m_LogicDevice, &allocInfo, &m_CommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void Application::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_Renderpass;
		renderPassInfo.framebuffer = m_FrameBuffers[imageIndex];
		renderPassInfo.renderArea.extent = m_SwapChainExtent;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.clearValueCount = 1;
		VkClearValue clearColor = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
			VkViewport viewport{};
			viewport.height = (float)m_SwapChainExtent.height;
			viewport.width  = (float)m_SwapChainExtent.width;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = m_SwapChainExtent;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			VkBuffer vertexBuffers[] = { m_VertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBuffer);
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Application::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	if (vkCreateSemaphore(m_LogicDevice, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS || 
		vkCreateSemaphore(m_LogicDevice, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(m_LogicDevice, &fenceInfo, nullptr, &m_InFlightFence)                   != VK_SUCCESS
		)
	{
		throw std::runtime_error("failed to create semaphores!");
	}
}

void Application::DrawFrame()
{
	vkWaitForFences(m_LogicDevice, 1, &m_InFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_LogicDevice, 1, &m_InFlightFence);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_LogicDevice, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(m_CommandBuffer, 0);
	RecordCommandBuffer(m_CommandBuffer, imageIndex);

	VkSubmitInfo submitInfo{};
	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
	VkSemaphore emittSemaphores[] = { m_RenderFinishedSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = emittSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	if (vkQueueSubmit(m_GraphicQueue, 1, &submitInfo, m_InFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}	

	VkSwapchainKHR swapChains[] = { m_SwapChain };
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pSwapchains = swapChains;
	presentInfo.swapchainCount = 1;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = emittSemaphores;
	presentInfo.pResults = nullptr;
	vkQueuePresentKHR(m_PresentQueue, &presentInfo);
}

void Application::CreateVertexBuffer()
{
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	createInfo.size = sizeof(m_Vertices[0]) * m_Vertices.size();
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(m_LogicDevice, &createInfo, nullptr, &m_VertexBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create vertex buffer!");
	}
	VkMemoryRequirements requirments;
	vkGetBufferMemoryRequirements(m_LogicDevice, m_VertexBuffer, &requirments);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = requirments.size;
	allocateInfo.memoryTypeIndex = FindMemoryType(requirments.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(m_LogicDevice, &allocateInfo, nullptr, &m_BufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(m_LogicDevice, m_VertexBuffer, m_BufferMemory, 0);
	void* data;
	vkMapMemory(m_LogicDevice, m_BufferMemory, 0, createInfo.size, 0, &data);
	memcpy(data, m_Vertices.data(), (size_t)createInfo.size);
	vkUnmapMemory(m_LogicDevice, m_BufferMemory);
}

uint32_t Application::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(m_PhyiscalDevice, &properties);
	for (size_t i = 0; i < properties.memoryTypeCount; i++)
	{
		if ((typeFilter &  (1 << i)) && (properties.memoryTypes[i].propertyFlags & flags) == flags)
		{
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

