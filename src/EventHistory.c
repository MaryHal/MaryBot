#include "EventHistory.h"

#include <stdlib.h>

EventHistory_t* createEventHistory(unsigned int historySize)
{
    EventHistory_t* history = (EventHistory_t*)malloc(sizeof(EventHistory_t));

    history->root = json_array();
    history->maxSize = historySize;

    return history;
}

void destroyEventHistory(EventHistory_t* history)
{
    free(history);
}

void pushEventToHistory(EventHistory_t* history,
                        time_t time,
                        const char* type, const char* origin,
                        const char** params, unsigned int paramCount)
{
    // If our event history is currently too large, get rid of the oldest
    // element.
    if (json_array_size(history->root) >= history->maxSize)
    {
        popEvent(history);
    }

    json_t* eventObj = json_object();
    json_object_set_new(eventObj, "time",  json_integer(time));
    json_object_set_new(eventObj, "type",  json_string(type));
    json_object_set_new(eventObj, "origin", json_string(origin));
    json_object_set_new(eventObj, "params", json_array());

    json_t* jsonParamArray = json_object_get(eventObj, "params");

    // Fill our json parameter array.
    for (unsigned int i = 0; i < paramCount; i++)
    {
        json_array_append(jsonParamArray, json_string(params[i]));
    }

    json_array_append(history->root, eventObj);

    exportHistory(history, "messages.json");
}

void popEvent(EventHistory_t* history)
{
    json_array_remove(history->root, 0);
}

const json_t* frontEvent(EventHistory_t* history)
{
    return json_array_get(history->root, json_array_size(history->root) - 1);
}

void exportHistory(EventHistory_t* history, const char* filename)
{
    json_dump_file(history->root, filename, JSON_COMPACT);
}
