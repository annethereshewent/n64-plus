#pragma once

#include "../parallel-rdp-standalone/vulkan/wsi.hpp"
#include "config.hpp"
#ifdef USING_SDL2
    #include <SDL2/SDL.h>
#else
    #include <SDL3/SDL.h>
#endif

class SDL_WSIPlatform : public Vulkan::WSIPlatform
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
	void set_window(SDL_Window *_window);
	void do_resize();

private:
	SDL_Window *window;
};
