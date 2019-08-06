#include <vulkan/vulkan.hpp>

namespace Sample
{
    struct Device
    {
        struct QueueFamilyIndices
        {
            uint32_t                            m_GraphicsQueueFamilyIndex;
            uint32_t                            m_PresentQueueFamilyIndex;
        };

        QueueFamilyIndices                      m_QueueFamilyIndices;
        vk::PhysicalDevice                      m_PhysicalDevice;
        vk::PhysicalDeviceProperties            m_PhysicalDeviceProperties;
        vk::Device                              m_Device;
        vk::Queue                               m_GraphicsQueue;
        vk::Queue                               m_PresentQueue;

        Device( vk::Instance instance,
            vk::SurfaceKHR surface,
            const std::vector<const char*>& layers,
            const std::vector<const char*>& extensions );

        ~Device();

    private:
        float getPhysicalDeviceSuitability(
            vk::PhysicalDevice device,
            vk::SurfaceKHR surface,
            const std::vector<const char*>& layers,
            const std::vector<const char*>& extensions );

        QueueFamilyIndices getPhysicalDeviceQueueFamilyIndices(
            vk::PhysicalDevice device,
            vk::SurfaceKHR surface );

        std::vector<vk::DeviceQueueCreateInfo> getQueueCreateInfos(
            QueueFamilyIndices indices );
    };
}
