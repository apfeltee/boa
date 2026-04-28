

#include <sys/time.h>
#include "lit.h"


void lit_eventsystem_init(LitState* state, LitEventSystem* event_system)
{
    event_system->events = NULL;
    event_system->last_event = NULL;
}

void lit_eventsystem_destroy(LitEventSystem* event_system)
{
    event_system->events = NULL;
    event_system->last_event = NULL;
}

uint64_t lit_eventsystem_millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (((uint64_t)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

void lit_eventsystem_registerevent(LitState* state, LitValue callback, uint64_t time)
{
    LitEventSystem* event_system = state->event_system;
    LitEvent* event = lit_reallocate(state, NULL, 0, sizeof(LitEvent));

    event->expire_time = lit_eventsystem_millis() + time;
    event->callback = callback;
    event->next = NULL;
    event->previous = event_system->last_event;

    if(event_system->last_event == NULL)
    {
        event_system->events = event;
    }
    else
    {
        event_system->last_event->next = event;
    }

    event_system->last_event = event;
}

void lit_eventsystem_loop(LitState* state)
{
    LitEventSystem* event_system = state->event_system;

    while(event_system->events != NULL)
    {
        LitEvent* event = event_system->events;

        while(event != NULL)
        {
            if(lit_eventsystem_millis() >= event->expire_time)
            {
                LitEvent* nextevent = event->next;

                if(event->previous != NULL)
                {
                    event->previous->next = nextevent;
                }

                if(event == event_system->events)
                {
                    event_system->events = nextevent;
                }

                if(event == event_system->last_event)
                {
                    event_system->last_event = NULL;
                }

                lit_state_callvalue(state, event->callback, NULL, 0);
                lit_reallocate(state, event, sizeof(LitEvent), 0);

                event = nextevent;
            }
            else
            {
                event = event->next;
            }
        }
    }
}