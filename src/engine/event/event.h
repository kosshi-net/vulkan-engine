#pragma once

#include "common.h"

enum Event {
	EVENT_NONE,

	EVENT_WIN_RESIZE,

	/* These can be used to recreate pipelines if no dynamic state is used.
	 * But just use dynamic pipeline state. */
	EVENT_VK_SWAPCHAIN_DESTROY,
	EVENT_VK_SWAPCHAIN_CREATE,

	/* Called right before the vulkan framework is destroyed */
	EVENT_RENDERERS_DESTROY, 

	EVENT_LOG,

	EVENT_COUNT,
};

void event_bind  (enum Event e, void(*callback)(Handle,void*), Handle);
void event_unbind(enum Event e, void(*callback)(Handle,void*), Handle);
void event_fire  (enum Event e, void*args);

