#ifndef RADGUI_Fifo_H
#define RADGUI_Fifo_H

#include "Event.h"
#include "Mutex.h"
#include "include/radgui.h"

/*
 * RADGUI Queue
 * ---------------------------------------------------------------------- */

/// @brief Check if the queue has no elements
/// @param queue        pointer to the queue
/// @return 0 if empty, > 0 if not
///
int RADGUI_QueueIsEmpty( RADGUI_Queue * queue );

///
/// @brief Return a reference to the front of the queue
/// @param queue        pointer to the queue
/// @return a reference to the front value
///
RADGUI_Event * RADGUI_QueueFront( RADGUI_Queue * queue );

///
/// @brief Pop the front element from the queue
/// @param queue        pointer to the queue
/// @return pointer to the new queue's head (NULL if empty)
///
RADGUI_Queue * RADGUI_QueuePop( RADGUI_Queue * queue );

///
/// @brief Enqueue a copy of the new element. The element will not be destroyed
/// @param queue        pointer to the queue
/// @param event        pointer to the new element
/// @return pointer to the new queue's head
///
RADGUI_Queue * RADGUI_QueuePush( RADGUI_Queue * queue, RADGUI_Event * event );

/*
 * RADGUI EventFifo
 * ---------------------------------------------------------------------- */

///
/// @brief Initialize a FIFO Queue of Events
/// @param eventFifo    pointer to EventFifo structure
///
void RADGUI_initEventFifo( RADGUI_EventFifo * eventFifo );

///
/// @brief Free resources stored in a FIFO Queue of Events
/// @param eventFifo    pointer to EventFifo structure
///
void RADGUI_destroyEventFifo( RADGUI_EventFifo * eventFifo );

///
/// @brief Check if a FIFO Queue of Events is empty
/// @param eventFifo    pointer to EventFifo structure
/// @return 0 is the queue is empty, -1 otherwise
///
int RADGUI_EventFifoIsEmpty( RADGUI_EventFifo * eventFifo );

///
/// @brief This function extracts an Event from the queue, or sleep if the
/// queue is empty, waiting for a new outgoing Event
/// @param eventFifo    pointer to EventFifo structure
/// @param event        pointer to the Event's structure used to store the extracted Event
///
void RADGUI_EventFifoPopOrSleepIfEmpty( RADGUI_EventFifo * eventFifo,
        RADGUI_Event * event );

///
/// @brief Push a new Event on the Event fifo
/// @param eventFifo    pointer to EventFifo structure
/// @param event        pointer to the Event to be pushed
///
void RADGUI_EventFifoPush( RADGUI_EventFifo * eventFifo, RADGUI_Event * event );

/*
 * RADGUI MultipleConsumerFifo
 * ---------------------------------------------------------------------- */

void RADGUI_initMultipleConsumerFifo( RADGUI_MultipleConsumerFifo * multConsFifo );
void RADGUI_destroyMultipleConsumerFifo( RADGUI_MultipleConsumerFifo * multConsFifo );
RADGUI_FifoListElement * RADGUI_MultipleConsumerFifoCreate(
        RADGUI_MultipleConsumerFifo * multConsFifo );
void RADGUI_MultipleConsumerFifoRelease( RADGUI_MultipleConsumerFifo * multConsFifo,
        RADGUI_FifoListElement * f );
void RADGUI_MultipleConsumerFifoPush( RADGUI_MultipleConsumerFifo * multConsFifo,
        RADGUI_Event * event );

#endif // RADGUI_Fifo_H
