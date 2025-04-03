#pragma once

#include "../../parallel-rdp-standalone/vulkan/wsi.hpp"
#include <cstdint>

class IOS_WSIPlatform : public Vulkan::WSIPlatform
{
public:
	VkSurfaceKHR create_surface(VkInstance instance, VkPhysicalDevice gpu) override;
    void destroy_surface(VkInstance instance, VkSurfaceKHR surface) override;
	std::vector<const char *> get_instance_extensions() override;
	uint32_t get_surface_width() override;
	uint32_t get_surface_height() override;
	bool alive(Vulkan::WSI &wsi) override;
	void poll_input() override;
	void poll_input_async(Granite::InputTrackerHandler *handler) override;

    void setLayer(void* layer);

	void setWidth(uint32_t width);
	void setHeight(uint32_t height);
private:
    void* metalLayer;
	uint32_t width = 0;
	uint32_t height = 0;
};
