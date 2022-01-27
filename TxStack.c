//********************************************************************************************
//
//   TxStack
//   Static memory manager
//
//   Copyright José Miguel Rodríguez Chávarri aka @txes
//
//********************************************************************************************


#include <stdlib.h>

#include "raylib.h"

#include "TxStack.h"

/********************************************************************************************/

TxLink gTxLinks[TXLINK_MAX] = { 0 };
TxLink *gTxLinkStack = NULL;


/********************************************************************************************/

TxLink *TxLinkGet()
{
	static int _count = 0;

	if (gTxLinkStack != NULL)
	{
		TxLink *_link = gTxLinkStack;
		gTxLinkStack = gTxLinkStack->next;
		return _link;
	}

	if (_count == TXLINK_MAX)
	{
		TraceLog(LOG_FATAL, "TxLinkGet: No more links available");
		return NULL;
	}

	TxLink *_link = gTxLinks + _count;
	_count += 1;

	return _link;
}

void TxLinkRemove(
	TxLink *_link
)
{
	_link->next = gTxLinkStack;
	gTxLinkStack = _link;
}


/********************************************************************************************/

void TxStackInit(
	TxStack *_stack,
	char *_data,
	int _count,
	int _size
)
{
	_stack->data = _data;
	_stack->count = _count;
	_stack->size = _size;
	_stack->stack = NULL;
}

void *TxStackGet(
	TxStack *_stack
)
{
	if (_stack->stack != NULL)
	{
		TxLink *_link = _stack->stack;
		_stack->stack = _link->next;

		void *_data = _link->data;
		TxLinkRemove(_link);

		return _data;
	}

	if (_stack->count == 0)
	{
		TraceLog(LOG_FATAL, "TxStackGet: No more data available");
		return NULL;
	}

	void *_data = _stack->data;
	_stack->data += _stack->size;
	_stack->count -= 1;

	return _data;
}

bool TxStackPut(
	TxStack *_stack,
	char *_data
)
{
	TxLink *_link = TxLinkGet();
	if (_link == NULL)
		return false;

	_link->data = _data;

	_link->next = _stack->stack;
	_stack->stack = _link;

	return true;
}


