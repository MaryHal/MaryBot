#ifndef EVENTHISTORY_H
#define EVENTHISTORY_H

#include <time.h>
#include <jansson.h>

// Keep track of the last n events in a JSON object.
typedef struct EventHistory_t
{
        json_t* root;
        unsigned int maxSize;
} EventHistory_t;

EventHistory_t* createEventHistory(unsigned int historySize);
void destroyEventHistory(EventHistory_t* history);
void pushEventToHistory(EventHistory_t* history,
                        time_t time,
                        const char* type, const char* origin,
                        const char** params, unsigned int paramCount);
void popEvent(EventHistory_t* history);
const json_t* frontEvent(EventHistory_t* history);

void exportHistory(EventHistory_t* history, const char* filename);

#endif /* EVENTHISTORY_H */
