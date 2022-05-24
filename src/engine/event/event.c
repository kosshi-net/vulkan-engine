#include "event/event.h"
#include "common.h"

#define MAX_BOUND_CALLBACKS 64

struct {
	void (*func)(Handle, void*);
	size_t data;
} callbacks[EVENT_COUNT][MAX_BOUND_CALLBACKS];

void event_bind(enum Event e, void(*callback)(Handle, void*), Handle data)
{
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i].func == NULL ) {
			callbacks[e][i].func = callback;
			callbacks[e][i].data = data;
			return;
		}
	}
}

void event_unbind(enum Event e, void(*callback)(Handle, void*), Handle data)
{
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if (callbacks[e][i].func == callback 
		 && callbacks[e][i].data == data)
		{
			callbacks[e][i].func = NULL;
		}
	}
}

void event_fire(enum Event e, void*args)
{
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i].func != NULL ) 
			callbacks[e][i].func(callbacks[e][i].data, args);
	}
}


