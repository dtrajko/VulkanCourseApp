#pragma once

#include "DeviceLVE.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <memory>


class SwapChainVCA
{
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

public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    SwapChainVCA(std::shared_ptr<DeviceLVE> device, VkExtent2D extent, std::shared_ptr<SwapChainVCA> previous, std::shared_ptr<WindowLVE> window);
    ~SwapChainVCA();

    void init();

    VkExtent2D getSwapChainExtent() { return m_SwapChainExtent; }
    VkResult acquireNextImage(uint32_t* imageIndex);
    VkResult submitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);
    VkSwapchainKHR& getSwapChainKHR() { return m_SwapchainKHR; }
    std::vector<SwapchainImage>& getSwapChainImages() { return m_SwapChainImages; }
    VkFormat getSwapChainImageFormat() { return m_SwapChainImageFormat; }
    SwapChainDetails getSwapChainDetails();
    VkRenderPass getRenderPass() { return m_RenderPass; }
    VkFramebuffer getFrameBuffer(int index) { return m_SwapChainFramebuffers[index]; }
    VkFormat findDepthFormat();

private:
    VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    void createRenderPass();

private:
    std::shared_ptr<DeviceLVE> m_Device;
    VkExtent2D m_WindowExtent;
    std::shared_ptr<SwapChainVCA> m_SwapChainOld;
    std::shared_ptr<WindowLVE> m_Window; // lveWindow

    VkSwapchainKHR m_SwapchainKHR;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    VkRenderPass m_RenderPass;

    std::vector<SwapchainImage> m_SwapChainImages;

    SwapChainDetails m_SwapChainDetails;

    size_t currentFrame = 0;

    // synchronization
    std::vector<VkSemaphore> m_ImageAvailableSemaphores;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<VkFence> m_InFlightFences;
    std::vector<VkFence> m_ImagesInFlight;

};
