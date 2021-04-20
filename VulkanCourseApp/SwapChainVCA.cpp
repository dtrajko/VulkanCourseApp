#include "SwapChainVCA.h"

#include "Utilities.h"

// std
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>


SwapChainVCA::SwapChainVCA(std::shared_ptr<DeviceLVE> device, VkExtent2D extent, std::shared_ptr<SwapChainVCA> previous, std::shared_ptr<WindowLVE> window)
    : m_Device{ device }, m_WindowExtent{ extent }, m_SwapChainOld{ previous }, m_Window{ window }
{
    init();

    // clean up old swap chain since it's no longer needed
    m_SwapChainOld = nullptr;
}

void SwapChainVCA::init()
{
    // Get SwapChain details so we can pick best settings
    SwapChainDetails swapChainDetails = getSwapChainDetails();

    // Find optimal surface values for our swap chain
    // 1. CHOOSE BEST SURFACE FORMAT
    VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
    // 2. CHOOSE BEST PRESENTATION MODE
    VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
    // 3. CHOOSE SWAP CHAIN RESOLUTION
    VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

    // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
    m_SwapChainImageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    // If image count higher than max, then clamp down to max
    // If 0, then limitless
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
        swapChainDetails.surfaceCapabilities.maxImageCount < m_SwapChainImageCount)
    {
        m_SwapChainImageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
    }

    // Creation information for swap chain
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_Device->surface();                                        // Swapchain surface
    swapChainCreateInfo.imageFormat = surfaceFormat.format;                                   // Swapchain format
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;                           // Swapchain color space
    swapChainCreateInfo.presentMode = presentMode;                                            // Swapchain presentation mode
    swapChainCreateInfo.imageExtent = extent;                                                 // Swapchain image extents
    swapChainCreateInfo.minImageCount = m_SwapChainImageCount;                                // Minimum images in swapchain
    swapChainCreateInfo.imageArrayLayers = 1;                                                 // Number of layers for each image in chain
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                     // What attachment images will be used as
    swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform; // Transform to perform to swap chain images
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle blending images with external graphics (e.g. other windows)
    swapChainCreateInfo.clipped = VK_TRUE; // Whether to clip parts of image not in view (e.g. behind another window, off screen)

    // Get Queue Family Indices
    QueueFamilyIndices indices = m_Device->findPhysicalQueueFamilies();

    // If Graphics and Presentation families are different, then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        uint32_t queueFamilyIndices[] =
        {
            (uint32_t)indices.graphicsFamily,
            (uint32_t)indices.presentationFamily,
        };

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image share handling
        swapChainCreateInfo.queueFamilyIndexCount = 2;                     // Number of queues to share images between
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;      // Array of queues to share between
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    // If old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create Swapchain
    VkResult result = vkCreateSwapchainKHR(m_Device->device(), &swapChainCreateInfo, nullptr, &m_SwapchainKHR);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Swapchain!");
    }

    printf("Vulkan Swapchain successfully created.\n");

    // Store for later reference
    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent = extent;

    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(m_Device->device(), m_SwapchainKHR, &swapChainImageCount, nullptr);

    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(m_Device->device(), m_SwapchainKHR, &swapChainImageCount, images.data());

    for (VkImage image : images)
    {
        // Store image handle
        VkImageView imageView = createImageView(image, m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        // Add to swapchain image list
        m_SwapChainImages.push_back(image);
        m_SwapChainImageViews.push_back(imageView);
    }

    createRenderPass();
    createColorBufferImage();
    createDepthResources();
    createFramebuffers();
    createSyncObjects();
}

VkResult SwapChainVCA::acquireNextImage(uint32_t* imageIndex) {
    vkWaitForFences(
        m_Device->device(),
        1,
        &m_InFlightFences[currentFrame],
        VK_TRUE,
        std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(
        m_Device->device(),
        m_SwapchainKHR,
        std::numeric_limits<uint64_t>::max(),
        m_ImageAvailableSemaphores[currentFrame],  // must be a not signaled semaphore
        VK_NULL_HANDLE,
        imageIndex);

    return result;
}

VkResult SwapChainVCA::submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex)
{
    if (m_ImagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_Device->device(), 1, &m_ImagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
    }

    m_ImagesInFlight[*imageIndex] = m_InFlightFences[currentFrame];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = buffers;

    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_Device->device(), 1, &m_InFlightFences[currentFrame]);

    if (vkQueueSubmit(m_Device->graphicsQueue(), 1, &submitInfo, m_InFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_SwapchainKHR };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = imageIndex;

    auto result = vkQueuePresentKHR(m_Device->presentationQueue(), &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

SwapChainVCA::SwapChainDetails SwapChainVCA::getSwapChainDetails()
{
    SwapChainDetails swapChainDetails;

    // -- SURFACE CAPABILITIES --
    // Get the surface capabilities for the given surface on the given physical device

    auto phyDevice = m_Device->getPhysicalDevice();
    auto surface = m_Device->surface();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &swapChainDetails.surfaceCapabilities);

    // -- FORMATS --
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &formatCount, nullptr);

    // If formats return, get list of formats
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &formatCount, swapChainDetails.formats.data());
    }

    // -- PRESENTATION MODES --
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &presentationCount, nullptr);

    // If presentation modes returned, get list of presentation modes
    if (presentationCount != 0)
    {
        swapChainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device->getPhysicalDevice(), m_Device->surface(), &presentationCount, swapChainDetails.presentationModes.data());
    }

    return swapChainDetails;
}

// Best format is subjective, but ours will be:
// Format:     VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
// colorSpace: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR SwapChainVCA::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If restricted, search for optimal format
    for (const auto& format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // If can't find optimal format, then just return first format
    return formats[0];
}

void SwapChainVCA::createColorBufferImage()
{
    // Resize supported format for color attachment
    m_ColorBufferImages.resize(m_SwapChainImages.size());
    m_ColorBufferImageMemory.resize(m_SwapChainImages.size());
    m_ColorBufferImageViews.resize(m_SwapChainImages.size());

    // Get supported format for color attachment
    VkFormat colorBufferImageFormat = chooseSupportedFormat(
        { VK_FORMAT_B8G8R8A8_UNORM }, // VK_FORMAT_R8G8B8A8_UNORM
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    for (size_t i = 0; i < m_SwapChainImages.size(); i++)
    {
        // Create Color Buffer Image
        m_ColorBufferImages[i] = createImage(m_SwapChainExtent.width, m_SwapChainExtent.height, colorBufferImageFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &m_ColorBufferImageMemory[i]);

        // Create Color Buffer Image View
        m_ColorBufferImageViews[i] = createImageView(m_ColorBufferImages[i], colorBufferImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void SwapChainVCA::createDepthResources()
{
    m_DepthBufferImages.resize(m_SwapChainImages.size());
    m_DepthBufferImageMemorys.resize(m_SwapChainImages.size());
    m_DepthBufferImageViews.resize(m_SwapChainImages.size());

    // Get supported format for depth buffer
    VkFormat depthBufferImageFormat = chooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    for (size_t i = 0; i < m_SwapChainImages.size(); i++)
    {
        // Create Depth Buffer Image
        m_DepthBufferImages[i] = createImage(m_SwapChainExtent.width, m_SwapChainExtent.height, depthBufferImageFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            &m_DepthBufferImageMemorys[i]);

        // Create Depth Buffer Image View
        m_DepthBufferImageViews[i] = createImageView(m_DepthBufferImages[i], depthBufferImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    }
}

void SwapChainVCA::createFramebuffers()
{
    // Resize framebuffer count to equal swapchain image count
    m_SwapChainFramebuffers.resize(m_SwapChainImages.size());

    // Create a framebuffer for each swapchain image
    for (size_t i = 0; i < m_SwapChainImages.size(); i++)
    {
        std::array<VkImageView, 3> attachments =
        {
            m_SwapChainImageViews[i],
            m_ColorBufferImageViews[i],
            m_DepthBufferImageViews[i],
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_RenderPass;         // Render Pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data(); // List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width = m_SwapChainExtent.width;     // Framebuffer width
        framebufferCreateInfo.height = m_SwapChainExtent.height;   // Framebuffer height
        framebufferCreateInfo.layers = 1;                        // Framebuffer layers

        VkResult result = vkCreateFramebuffer(m_Device->device(), &framebufferCreateInfo, nullptr, &m_SwapChainFramebuffers[i]);

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Framebuffer!");
        }

        printf("Vulkan Framebuffer successfully created.\n");
    }
}

VkPresentModeKHR SwapChainVCA::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
    // Look for VK_PRESENT_MODE_MAILBOX_KHR
    for (const auto& presentationMode : presentationModes)
    {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentationMode;
        }
    }

    // If can't find, use FIFO as Vulkan spec says it must be present
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkFormat SwapChainVCA::findDepthFormat()
{
    return m_Device->findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkExtent2D SwapChainVCA::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
    // If current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        // If value can very, need to set manually

        // Get window size
        int width, height;
        glfwGetFramebufferSize(m_Window->getHandle(), &width, &height);

        // Create new extent using window size
        VkExtent2D newExtent = {};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        // Surface also defines max and min, so make sure within boundaries by clamping value
        newExtent.width = std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width);
        newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, newExtent.width);
        newExtent.height = std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height);
        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, newExtent.height);

        return newExtent;
    }
}

VkImageView SwapChainVCA::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {};

    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;                                // Image create view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;             // Type if image (1D, 2D, 3D, Cube etc)
    viewCreateInfo.format = format;                              // Format of image data
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Allows remapping of RGBA components to other RGBA values
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources - allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;    // Which aspect of image to view (e.g. COLOR_BIT for viewing color)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;            // Start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;              // Number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;          // Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;              // Number of array levels to view

    // Create image view and return it
    VkImageView imageView;
    VkResult result = vkCreateImageView(m_Device->device(), &viewCreateInfo, nullptr, &imageView);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an ImageView!");
    }

    printf("Vulkan ImageView successfully created.\n");

    return imageView;
}

void SwapChainVCA::createRenderPass()
{
    // Array of our subpasses
    std::array<VkSubpassDescription, 2> subpasses = {};

    // ATTACHMENTS

    // -- SUBPASS 1 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

    // -- -- Color Attachment (Input Attachment)
    VkFormat colorBufferImageFormat = chooseSupportedFormat(
        { VK_FORMAT_B8G8R8A8_UNORM }, // VK_FORMAT_R8G8B8A8_UNORM
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = colorBufferImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // -- -- Depth Attachment (Input Attachment)
    VkFormat depthBufferImageFormat = chooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = depthBufferImageFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // -- -- Color Attachment (Input Attachment) Reference
    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 1; // ID of attachments array in createFramebuffers
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // -- -- Depth Attachment (Input Attachment) Reference
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 2; // ID of attachments array in createFramebuffers
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Set up Subpass 1
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = &colorAttachmentReference;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentReference;

    // -- SUBPASS 2 ATTACHMENTS + REFERENCES (INPUT ATTACHMENTS)

    // -- -- Swapchain Color Attachment
    VkAttachmentDescription swapchainColorAttachment = {};
    swapchainColorAttachment.format = m_SwapChainImageFormat;                     // Format to use for attachment
    swapchainColorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                   // Number of samples to write for multisampling
    swapchainColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;              // Describes what to do with attachment before rendering
    swapchainColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // Describes what to do with attachment after rendering
    swapchainColorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Describes what to do with stencil before rendering
    swapchainColorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    swapchainColorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;         // Image data layout before Render Pass starts
    swapchainColorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;     // Image data layout after Render Pass (to change to)

    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference swapchainColorAttachmentReference = {};
    swapchainColorAttachmentReference.attachment = 0; // First one from the render pass attachments list
    swapchainColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // References to attachments that subpass will take input from
    std::array<VkAttachmentReference, 2> inputReferences;
    inputReferences[0].attachment = 1;
    inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    inputReferences[1].attachment = 2;
    inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // Set up Subpass 2
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &swapchainColorAttachmentReference;
    subpasses[1].inputAttachmentCount = static_cast<uint32_t>(inputReferences.size());
    subpasses[1].pInputAttachments = inputReferences.data();

    // SUBPASS DEPENDENCIES

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 3> subpassDependencies;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED (colorAttachment.initialLayout) to 
    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL (colorAttachmentReference.layout)
    // Transition must happen after:
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of Render Pass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;           // Stage access mask (memory access)
    // But must happen before:
    subpassDependencies[0].dstSubpass = 0; // First subpass (?)
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;

    // Subpass 1 layout (color/depth) to Subpass 2 layout
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[1].dstSubpass = 1;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL (colorAttachmentReference.layout) to 
    // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR (colorAttachment.finalLayout)
    // Transition must happen after:
    subpassDependencies[2].srcSubpass = 0; // Subpass index
    subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // But must happen before:
    subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[2].dependencyFlags = 0;

    std::array<VkAttachmentDescription, 3> renderPassAttachments = { swapchainColorAttachment, colorAttachment, depthAttachment };

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    renderPassCreateInfo.pSubpasses = subpasses.data();
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(m_Device->device(), &renderPassCreateInfo, nullptr, &m_RenderPass);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Render Pass!");
    }

    printf("Vulkan Render Pass successfully created.\n");
}

VkFormat SwapChainVCA::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    // Loop through options and find compatible one
    for (VkFormat format : formats)
    {
        // Get properties for given format on this device
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_Device->getPhysicalDevice(), format, &properties);

        // Depending on tiling choice, need to check for different bit flag
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a matching format!");
}

void SwapChainVCA::createSyncObjects() {
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_ImagesInFlight.resize(imageCount(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_Device->device(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateSemaphore(m_Device->device(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) !=
            VK_SUCCESS ||
            vkCreateFence(m_Device->device(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

VkImage SwapChainVCA::createImage(uint32_t width, uint32_t height, VkFormat format,
    VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
{
    // CREATE IMAGE
    // Image Creation Info
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;              // Type of image (1D, 2D or 3D)
    imageCreateInfo.extent.width = width;                      // Width of image extent
    imageCreateInfo.extent.height = height;                    // Height of image extent
    imageCreateInfo.extent.depth = 1;                          // Depth of image (just 1, no 3D aspect)
    imageCreateInfo.mipLevels = 1;                             // Number of mipmap levels
    imageCreateInfo.arrayLayers = 1;                           // Number of levels in image array
    imageCreateInfo.format = format;                           // Format type of image
    imageCreateInfo.tiling = tiling;                           // How image data should be "tiled" (arranged for optimal reading)
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout of image data on creation
    imageCreateInfo.usage = useFlags;                          // Bit flags defining what image will be used for
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;           // Number of samples for multi-sampling
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;   // Whether image can be shared between queues

    VkImage image;
    VkResult result = vkCreateImage(m_Device->device(), &imageCreateInfo, nullptr, &image);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image!");
    }

    printf("Vulkan Image successfully created.\n");

    // CREATE MEMORY FOR IMAGE

    // Get memory requirements for a type of image
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(m_Device->device(), image, &memoryRequirements);

    // Allocate memory using image requirements and user defined properties
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(m_Device->getPhysicalDevice(), memoryRequirements.memoryTypeBits, propFlags);

    result = vkAllocateMemory(m_Device->device(), &memoryAllocInfo, nullptr, imageMemory);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate memory for image!");
    }

    printf("Vulkan Image Memory successfully allocated.\n");

    // Connect memory to image
    vkBindImageMemory(m_Device->device(), image, *imageMemory, 0);

    return image;
}

SwapChainVCA::~SwapChainVCA()
{
    for (auto semaphore : m_ImageAvailableSemaphores) {
        vkDestroySemaphore(m_Device->device(), semaphore, nullptr);
    }

    for (auto semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device->device(), semaphore, nullptr);
    }

    for (auto fence : m_InFlightFences) {
        vkDestroyFence(m_Device->device(), fence, nullptr);
    }

    for (auto image : m_ColorBufferImages) {
        vkDestroyImage(m_Device->device(), image, nullptr);
    }

    for (auto image : m_DepthBufferImages) {
        vkDestroyImage(m_Device->device(), image, nullptr);
    }

    for (auto imageView : m_ColorBufferImageViews) {
        vkDestroyImageView(m_Device->device(), imageView, nullptr);
    }

    for (auto imageView : m_DepthBufferImageViews) {
        vkDestroyImageView(m_Device->device(), imageView, nullptr);
    }

    for (auto deviceMemory : m_ColorBufferImageMemory) {
        vkFreeMemory(m_Device->device(), deviceMemory, nullptr);
    }

    for (auto deviceMemory : m_DepthBufferImageMemorys) {
        vkFreeMemory(m_Device->device(), deviceMemory, nullptr);
    }

    for (auto framebuffer : m_SwapChainFramebuffers) {
        vkDestroyFramebuffer(m_Device->device(), framebuffer, nullptr);
    }
}
