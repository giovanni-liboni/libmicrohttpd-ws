#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Fifo.h"
#include "Utils.h"
#include "Event.h"

#include "utlist.h"

/*
 * RADGUI EventFifo
 * ----------------------------------------------------------------- */

void RADGUI_initEventFifo( RADGUI_EventFifo * eventFifo )
{
    RADGUI_ConditionInit( &eventFifo->condition );
    RADGUI_MutexInit( &eventFifo->mutex );
    // initialize utlist's head
    eventFifo->container = NULL;
}

void RADGUI_destroyEventFifo( RADGUI_EventFifo * eventFifo )
{
    RADGUI_MutexLock( &eventFifo->mutex );

    // destroy the event queue
    while ( eventFifo->container != NULL )
        eventFifo->container = RADGUI_QueuePop( eventFifo->container );

// TODO *** Memory leak.. why ?! ***
//    RADGUI_Queue * iter_el, *tmp_el;
//    DL_FOREACH_SAFE( eventFifo->container, iter_el, tmp_el )
//    {
//        RADGUI_destroyEvent( &iter_el->val );
//        DL_DELETE( eventFifo->container, iter_el );
//    }

    RADGUI_MutexUnlock( &eventFifo->mutex );

    // destroy mutex and condition
    RADGUI_MutexDestroy( &eventFifo->mutex );
    RADGUI_ConditionDestroy( &eventFifo->condition );
}

int RADGUI_EventFifoIsEmpty( RADGUI_EventFifo * eventFifo )
{
    int e = 0;

    RADGUI_MutexLock( &eventFifo->mutex );
    e = RADGUI_QueueIsEmpty( eventFifo->container );
    RADGUI_MutexUnlock( &eventFifo->mutex );

    return e;
}

void RADGUI_EventFifoPopOrSleepIfEmpty( RADGUI_EventFifo * eventFifo,
        RADGUI_Event * event )
{
    RADGUI_Event * queueFront;

    RADGUI_MutexLock( &eventFifo->mutex );
    while ( 1 )
    {
        if ( !RADGUI_QueueIsEmpty( eventFifo->container ) )
            break;

        RADGUI_ConditionWait( &eventFifo->condition, &eventFifo->mutex );
    }

    queueFront = RADGUI_QueueFront( eventFifo->container );
    event->param = strdup( queueFront->param );
    event->value = strdup( queueFront->value );

    eventFifo->container = RADGUI_QueuePop( eventFifo->container );

    RADGUI_MutexUnlock( &eventFifo->mutex );
}

void RADGUI_EventFifoPush( RADGUI_EventFifo * eventFifo, RADGUI_Event * event )
{
    int wasEmpty = 0;
    RADGUI_MutexLock( &eventFifo->mutex );

    wasEmpty = RADGUI_QueueIsEmpty( eventFifo->container );
    eventFifo->container = RADGUI_QueuePush( eventFifo->container, event );

    if ( wasEmpty )
        RADGUI_ConditionSignal( &eventFifo->condition );

    RADGUI_MutexUnlock( &eventFifo->mutex );
}

/*
 * RADGUI MultipleConsumerFifo
 * ----------------------------------------------------------------- */

void RADGUI_initMultipleConsumerFifo( RADGUI_MultipleConsumerFifo * multConsFifo )
{
    RADGUI_MutexInit( &multConsFifo->mutex );
    // initialize utlist's head
    multConsFifo->container = NULL;
}

void RADGUI_destroyMultipleConsumerFifo( RADGUI_MultipleConsumerFifo * multConsFifo )
{
    RADGUI_FifoListElement * iter_el;

    RADGUI_MutexLock( &multConsFifo->mutex );
    // destroy the list of event fifos
    DL_FOREACH( multConsFifo->container, iter_el )
    {
        RADGUI_MultipleConsumerFifoRelease( multConsFifo, iter_el );
    }
    RADGUI_MutexUnlock( &multConsFifo->mutex );

    // destroy mutex
    RADGUI_MutexDestroy( &multConsFifo->mutex );
}

RADGUI_FifoListElement * RADGUI_MultipleConsumerFifoCreate(
        RADGUI_MultipleConsumerFifo * multConsFifo )
{
    RADGUI_FifoListElement * newElement;

    RADGUI_MutexLock( &multConsFifo->mutex );
    // create and append a new event FIFO
    SAFE_MALLOC( newElement, RADGUI_FifoListElement );
    RADGUI_initEventFifo( &newElement->entry );

    DL_APPEND( multConsFifo->container, newElement );

    RADGUI_MutexUnlock( &multConsFifo->mutex );

    return newElement;
}

void RADGUI_MultipleConsumerFifoRelease( RADGUI_MultipleConsumerFifo * multConsFifo,
        RADGUI_FifoListElement * f )
{
    RADGUI_MutexLock( &multConsFifo->mutex );
    RADGUI_destroyEventFifo( &f->entry );
    DL_DELETE( multConsFifo->container, f );
    RADGUI_MutexUnlock( &multConsFifo->mutex );
}

void RADGUI_MultipleConsumerFifoPush( RADGUI_MultipleConsumerFifo * multConsFifo,
        RADGUI_Event * event )
{
    RADGUI_FifoListElement * iter_el;

    RADGUI_MutexLock( &multConsFifo->mutex );
    DL_FOREACH( multConsFifo->container, iter_el )
    {
        RADGUI_EventFifoPush( &iter_el->entry, event );
    }
    RADGUI_MutexUnlock( &multConsFifo->mutex );
}

/*
 * RADGUI Queue of RADGUI_Event struct.
 * Implemented on top of utlist data structure (double-linked list)
 * ----------------------------------------------------------------- */

int RADGUI_QueueIsEmpty( RADGUI_Queue * queue )
{
    return queue == NULL ;
}

RADGUI_Event * RADGUI_QueueFront( RADGUI_Queue * queue )
{
    assert( queue != NULL );

    if ( RADGUI_QueueIsEmpty( queue ) )
        return NULL ;

    return &queue->val;
}

RADGUI_Queue * RADGUI_QueuePop( RADGUI_Queue * queue )
{
    RADGUI_Queue * first;

    assert( queue != NULL );
    first = queue;

    // free resources
    RADGUI_destroyEvent( &first->val );

    // Delete the queue's head.
    // The pointer to the queue itself identify the head
    DL_DELETE( queue, queue );

    free( first );
    return queue;
}

RADGUI_Queue * RADGUI_QueuePush( RADGUI_Queue * queue, RADGUI_Event * event )
{
    RADGUI_QueueElement * newElement;
    SAFE_MALLOC( newElement, RADGUI_QueueElement );

    newElement->val.param = strdup( event->param );
    newElement->val.value = strdup( event->value );

    DL_APPEND( queue, newElement );
    return queue;
}

