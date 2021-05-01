#pragma once

#include "DeviceLVE.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <memory>


class SwapChain
{
public:
    struct SwapChainDetails
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;    // Surface properties, e.g. image size/extent
        std::vector<VkSurfaceFormatKHR> formats;         // Surface image formats, e.g. RGBA and size of each color
        std::vector<VkPresentModeKHR> presentationModes; // How images should be presented to screen
    };

public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SwapChain(std::shared_ptr<DeviceLVE> device, VkExtent2D extent, std::shared_ptr<SwapChain> previous, std::shared_ptr<WindowLVE> window);
    ~SwapChain();

    void init();

    VkExtent2D getSwapChainExtent() { return m_SwapChainExtent; }
    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
    VkSwapchainKHR& getSwapChainKHR() { return m_SwapchainKHR; }
    std::vector<VkImage>& getSwapChainImages() { return m_SwapChainImages; }
    std::vector<VkImageView>& getSwapChainImageViews() { return m_SwapChainImageViews; }
    VkFormat getSwapChainImageFormat() { return m_SwapChainImageFormat; }
    SwapChainDetails getSwapChainDetails();
    VkRenderPass& getRenderPass() { return m_RenderPass; }
    VkFramebuffer& getFrameBuffer(int index) { return m_SwapChainFramebuffers[index]; }
    std::vector<VkFramebuffer>& getFrameBuffers() { return m_SwapChainFramebuffers; }
    VkFormat findDepthFormat();
    VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
    std::vector<VkImageView>& getColorBufferImageViews() { return m_ColorBufferImageViews; }
    std::vector<VkImageView>& getDepthBufferImageViews() { return m_DepthBufferImageViews; }

private:
    VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    size_t imageCount() { return m_SwapChainImages.size(); }
    VkImage createImage(uint32_t width, uint32_t height, VkFormat format,
        VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);
    void createRenderPass();
    void createColorBufferImage();
    void createDepthResources();
    void createFramebuffers();
    void createSyncObjects();

private:
    std::shared_ptr<DeviceLVE> m_Device;
    VkExtent2D m_WindowExtent;
    std::shared_ptr<SwapChain> m_SwapChainOld;
    std::shared_ptr<WindowLVE> m_Window; // lveWindow

    VkSwapchainKHR m_SwapchainKHR;
    VkFormat m_SwapChainImageFormat;
    VkFormat m_SwapChainDepthFormat;
    VkExtent2D m_SwapChainExtent;

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    VkRenderPass m_RenderPass;

    SwapChainDetails m_SwapChainDetails;

    size_t currentFrame = 0;

    uint32_t m_SwapChainImageCount;

    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;

    std::vector<VkImage> m_ColorBufferImages;
    std::vector<VkDeviceMemory> m_ColorBufferImageMemory;
    std::vector<VkImageView> m_ColorBufferImageViews;

    // framebuffers / depth resources
    std::vector<VkImage> m_DepthBufferImages;
    std::vector<VkDeviceMemory> m_DepthBufferImageMemorys;
    std::vector<VkImageView> m_DepthBufferImageViews;

    // synchronization
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;

};
