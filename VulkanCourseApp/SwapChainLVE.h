#pragma once

#include "DeviceLVE.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <memory>


class SwapChainLVE {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SwapChainLVE(std::shared_ptr<DeviceLVE> device, VkExtent2D windowExtent);
    SwapChainLVE(std::shared_ptr<DeviceLVE> device, VkExtent2D windowExtent, std::shared_ptr<SwapChainLVE> previous);
    ~SwapChainLVE();

    SwapChainLVE(const SwapChainLVE&) = delete;
    SwapChainLVE& operator=(const SwapChainLVE&) = delete;

    VkFramebuffer getFrameBuffer(int index) { return swapChainFramebuffers[index]; }
    VkRenderPass getRenderPass() { return renderPass; }
    VkImageView getImageView(int index) { return swapChainImageViews[index]; }
    size_t imageCount() { return swapChainImages.size(); }
    VkFormat getSwapChainImageFormat() { return swapChainImageFormat; }
    VkExtent2D getSwapChainExtent() { return swapChainExtent; }
    uint32_t width() { return swapChainExtent.width; }
    uint32_t height() { return swapChainExtent.height; }

    float extentAspectRatio() {
        return static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height);
    }
    VkFormat findDepthFormat();

    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

private:
    void init();
    void createSwapChain();
    void createImageViews();
    void createDepthResources();
    void createRenderPass();
    void createFramebuffers();
    void createSyncObjects();

    // Helper functions
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

public:
    struct SwapChainDetails
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;    // Surface properties, e.g. image size/extent
        std::vector<VkSurfaceFormatKHR> formats;         // Surface image formats, e.g. RGBA and size of each color
        std::vector<VkPresentModeKHR> presentationModes; // How images should be presented to screen
    };

    struct SwapchainImage
    {
        VkImage image;
        VkImageView imageView;
    };

private:

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VkDeviceMemory> depthImageMemorys;
    std::vector<VkImageView> depthImageViews;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;

    std::shared_ptr<DeviceLVE> m_Device;
    VkExtent2D m_WindowExtent;

    VkSwapchainKHR swapChain;
    std::shared_ptr<SwapChainLVE> m_SwapChainOld;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame = 0;
};
