#pragma once

enum Event {
	EVENT_NONE,

	EVENT_WIN_RESIZE,
	EVENT_VK_SWAPCHAIN_DESTROY,
	EVENT_VK_SWAPCHAIN_CREATE,

	EVENT_COUNT,
};

void event_bind  (enum Event e, void(*callback)(void*));
void event_unbind(enum Event e, void(*callback)(void*));
void event_fire  (enum Event e, void*args);
