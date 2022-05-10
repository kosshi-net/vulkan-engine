#include "event/event.h"
#include "common.h"

#define MAX_BOUND_CALLBACKS 64
static void (*callbacks[EVENT_COUNT][MAX_BOUND_CALLBACKS])(void*) = {{ 0 }};

void event_bind(enum Event e, void(*callback)(void*))
{
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i] == NULL ) {
			callbacks[e][i] = callback;
			return;
		}
	}
}

void event_unbind(enum Event e, void(*callback)(void*))
{
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i] == callback )
			callbacks[e][i] = NULL;
	}
}

void event_fire(enum Event e, void*args)
{
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i] != NULL ) 
			callbacks[e][i](args);
	}
}


