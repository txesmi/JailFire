//********************************************************************************************
//
//   TxQuad
//   Screen quadrant input event manager
//
//   Copyright José Miguel Rodríguez Chávarri aka @txes
//
//********************************************************************************************

//#define _DEBUG_CURRENT_QUAD
//#define _DEBUG_GLOBALS
//#define _DEBUG_GET_POINTED_CHILD

/********************************************************************************************/

#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#include "TxStack.h"
#include "TxQuad.h"


/********************************************************************************************/

enum TxQuadFlags
{
	FLAG_VISIBLE		= (1 << 0),
	FLAG_UNTOUCHABLE	= (1 << 1),

	FLAG_MODAL			= (1 << 10),
	FLAG_CONTEXT		= (1 << 11),

	FLAG_LAST
};


/********************************************************************************************/

TxScheduler gTxSchedulers[TXSCHEDULER_MAX] = { 0 };
TxStack gTxSchedulerStack = { 0 };
TxScheduler gTxSchedulerFirst = { 0 };
TxScheduler *gTxSchedulerLast = &gTxSchedulerFirst;

TxQuad gTxQuads[TXQUAD_MAX] = { 0 };
TxStack gTxQuadStack = { 0 };
TxQuad gTxQuadVisible = { 0 };
TxQuad gTxQuadHidden = { 0 };
TxQuad *gTxQuadCurrent = NULL;
bool gTxQuadLocked = false;


/********************************************************************************************/

TxScheduler *TxSchedulerAdd(
	TxQuad *_quad, 
	void *_event
)
{
	TxScheduler *_sch = TxStackGet(&gTxSchedulerStack);
	if (_sch == NULL)
	{
		TraceLog(LOG_FATAL, "TxSchedulerAdd: Unable to create scheduler");
		return NULL;
	}

	_sch->quad = _quad;
	_sch->Event = _event;

	if (gTxSchedulerFirst.next == NULL)
		gTxSchedulerFirst.next = _sch;
	_sch->prev = gTxSchedulerLast;
	_sch->next = NULL;
	gTxSchedulerLast->next = _sch;
	gTxSchedulerLast = _sch;

	return _sch;
}

void TxSchedulerRemove(TxScheduler *_sch)
{
	_sch->prev->next = _sch->next;
	if (_sch == gTxSchedulerLast)
		gTxSchedulerLast = _sch->prev;
	else
		_sch->next->prev = _sch->prev;

	TxStackPut(&gTxSchedulerStack, (char*)_sch);

}

void TxSchedulerRun(void)
{
	for (TxScheduler *_sch = gTxSchedulerFirst.next; _sch != NULL; _sch = _sch->next)
	{
		int _res = _sch->Event(_sch);
		if (_res == SCH_CONTINUE)
			continue;
		switch (_res)
		{
		case SCH_REMOVE:
		{
			TxScheduler *_prev = _sch->prev;
			TxSchedulerRemove(_sch);
			_sch = _prev;

		} break;

		}

	}
}


/********************************************************************************************/

void TxQuadLinkLast(
	TxQuad *_parent,
	TxQuad *_child
)
{
	if (_parent == NULL)
		_parent = &gTxQuadHidden;

	TxQuad *_childLast = _parent->childLast;
	if (_childLast != NULL)
	{
		_child->parent = _parent;
		_child->prev = _childLast;
		_child->next = NULL;
		_childLast->next = _child;
		_parent->childLast = _child;
	}
	else
	{
		_parent->childFirst = _child;
		_parent->childLast = _child;
		_child->parent = _parent;
		_child->prev = NULL;
		_child->next = NULL;
	}
}

void TxQuadUnLink(
	TxQuad *_quad
)
{
	TxQuad *_parent = _quad->parent;
	TxQuad *_next = _quad->next;
	TxQuad *_prev = _quad->prev;

	if (_parent->childFirst == _quad)
		_parent->childFirst = _next;

	if (_parent->childLast == _quad)
		_parent->childLast = _prev;

	if (_next != NULL)
		_next->prev = _prev;

	if (_prev != NULL)
		_prev->next = _next;

	_quad->parent = NULL;
	_quad->prev = NULL;
	_quad->next = NULL;
}

//-------------------------------------------------------------------------------------------

void TxQuadEventCallChildren(
	TxQuad *_quad,
	int _eventType
)
{
	if (_quad->eventMask & _eventType)
		_quad->Event(_quad, _eventType);

	for (TxQuad *_child = _quad->childFirst; _child != NULL; _child = _child->next)
		TxQuadEventCallChildren(_child, _eventType);

}

bool TxQuadPointCollision(
	TxQuad *_quad,
	Vector2 _point
)
{
	if (_quad->screenPos.x > _point.x)
		return false;
	if (_quad->screenPos.y > _point.y)
		return false;
	if (_quad->screenPos.x + _quad->size.x < _point.x)
		return false;
	if (_quad->screenPos.y + _quad->size.y < _point.y)
		return false;

	return true;
}

bool TxQuadQuadCollision(
	TxQuad *_quad0,
	TxQuad *_quad1,
	float _margin
)
{
	if (_quad0->screenPos.x > _quad1->screenPos.x + _quad1->size.x + _margin)
		return false;
	if (_quad0->screenPos.y > _quad1->screenPos.y + _quad1->size.y + _margin)
		return false;
	if (_quad0->screenPos.x + _quad0->size.x + _margin < _quad1->screenPos.x)
		return false;
	if (_quad0->screenPos.y + _quad0->size.y + _margin < _quad1->screenPos.y)
		return false;

	return true;

}

TxQuad *TxQuadGetPointedChild(
	TxQuad *_parent,
	Vector2 _pos
)
{
	if (gTxQuadVisible.childLast != NULL)
	{
		TxQuad *_quad = gTxQuadVisible.childLast;
		if (_quad->flags & FLAG_MODAL)
		{
			if (TxQuadPointCollision(_quad, _pos))
			{
				TxQuad *_child = TxQuadGetPointedChild(_quad, _pos);
				if (_child == NULL)
					return _quad;
				else
					return _child;
			}
			else
			{
				return NULL;
			}
		}
	}
	else
	{
		return NULL;
	}

	for (TxQuad *_quad = _parent->childLast; _quad != NULL; _quad = _quad->prev)
	{
		if (_quad->flags & FLAG_UNTOUCHABLE)
			continue;

		if (TxQuadPointCollision(_quad, _pos))
		{
			TxQuad *_child = TxQuadGetPointedChild(_quad, _pos);
			if (_child == NULL)
				return _quad;
			else
				return _child;
		}
	}

	return NULL;
}

bool TxQuadContains(
	TxQuad *_parent,
	TxQuad *_child
)
{
	if (_parent == _child)
		return true;

	for (TxQuad *_quad = _parent->childFirst; _quad != NULL; _quad = _quad->next)
		if (TxQuadContains(_quad, _child))
			return true;

	return false;
}

TxQuad *TxQuadCurrentGet(void)
{
	return gTxQuadCurrent;
}

bool TxQuadIsVisible(
	TxQuad *_quad
)
{
	if (_quad->flags & FLAG_VISIBLE)
		return true;
	else
		return false;
}

Vector2 TxQuadToScreenSpace(
	TxQuad *_quad,
	Vector2 _pos
)
{

	return _pos;
}

Vector2 ScreenToTxQuadSpace(
	TxQuad *_quad,
	Vector2 _pos
)
{
	for (; _quad->parent != NULL; _quad = _quad->parent)
	{
		_pos = Vector2Subtract(
			Vector2Subtract(
				_pos, 
				_quad->position
			),
			_quad->parent->origin
		);
	}
	return _pos;
}


//-------------------------------------------------------------------------------------------

TxQuad *TxQuadCreate(
	TxQuad *_parent,
	Vector2 _position,
	Vector2 _size,
	Vector2 _origin,
	void *_event,
	int _eventMask,
	void *_data
)
{
	TxQuad *_quad = TxStackGet(&gTxQuadStack);
	if (_quad == NULL)
	{
		TraceLog(LOG_FATAL, "TxQuadCreate: gTxQuadStack: No more data available");
		return NULL;
	}

	_quad->position = _position;
	_quad->size = _size;
	_quad->origin = _origin;
	_quad->Event = _event;
	_quad->eventMask = _eventMask;
	_quad->flags = 0;
	_quad->data = _data;

	_quad->parent = NULL;
	_quad->childFirst = NULL;
	_quad->childLast = NULL;
	_quad->prev = NULL;
	_quad->next = NULL;

	if (_parent == NULL)
	{
		_quad->screenPos = _quad->position;
	}
	else
	{
		_quad->flags |= FLAG_VISIBLE;
		_quad->screenPos = Vector2Add(
			Vector2Add(
				_parent->screenPos,
				_parent->origin
			),
			_quad->position
		);
	}

	TxQuadLinkLast(_parent, _quad);

	return _quad;
}

void TxQuadShow(
	TxQuad *_quad,
	int _mode
)
{
	if (_quad->parent->parent == NULL)
	{
		TxQuadUnLink(_quad);
		TxQuadLinkLast(&gTxQuadVisible, _quad);
	}

	_quad->flags = FLAG_VISIBLE;

	switch (_mode)
	{
	case MODE_UNTOUCHABLE:	_quad->flags |= FLAG_UNTOUCHABLE; break;
	case MODE_MODAL:		_quad->flags |= FLAG_MODAL; break;
	case MODE_CONTEXT:		_quad->flags |= FLAG_CONTEXT; break;
	}

	TxQuadEventCallChildren(_quad, TXQUAD_SHOW);
}

void TxQuadHide(
	TxQuad *_quad
)
{
	if (TxQuadContains(_quad, gTxQuadCurrent) == true)
	{
		if (gTxQuadCurrent->eventMask & TXQUAD_UNSELECT)
			gTxQuadCurrent->Event(gTxQuadCurrent, TXQUAD_UNSELECT);
		gTxQuadCurrent = NULL;
	}

	_quad->flags &= ~FLAG_VISIBLE;

	TxQuadEventCallChildren(_quad, TXQUAD_HIDE);

	if (_quad->parent->parent == NULL)
	{
		TxQuadUnLink(_quad);
		TxQuadLinkLast(&gTxQuadHidden, _quad);
	}
}

void TxQuadToFront(
	TxQuad *_quad
)
{
	TxQuad *_parent = _quad->parent;
	TxQuadUnLink(_quad);
	TxQuadLinkLast(_parent, _quad);
}

void TxQuadRemove(
	TxQuad *_quad
)
{
	for (TxQuad *_child = _quad->childLast; _child != NULL; _child = _child->prev)
		TxQuadRemove(_child);

	if (_quad == gTxQuadCurrent)
	{
		if (_quad->eventMask & TXQUAD_UNSELECT)
			_quad->Event(_quad, TXQUAD_UNSELECT);
		gTxQuadCurrent = NULL;
	}

	if (_quad->eventMask & TXQUAD_REMOVE)
		_quad->Event(_quad, TXQUAD_REMOVE);

	TxQuadUnLink(_quad);

	TxStackPut(&gTxQuadStack, (char*)_quad);
}

int TxQuadRemoveScheduled2(TxScheduler *_scheduler)
{
	TxQuadRemove(_scheduler->quad);

	return SCH_REMOVE;
}

void TxQuadRemoveScheduled(
	TxQuad *_quad
)
{
	TxSchedulerAdd(_quad, TxQuadRemoveScheduled2);
}

void TxQuadRemoveAll(void)
{
	for (TxQuad *_child = gTxQuadHidden.childLast; _child != NULL; _child = _child->prev)
		TxQuadRemove(_child);

	for (TxQuad *_child = gTxQuadVisible.childLast; _child != NULL; _child = _child->prev)
		TxQuadRemove(_child);
}

//-------------------------------------------------------------------------------------------

void TxQuadDraw(
	TxQuad *_quad
)
{
	_quad->screenPos = Vector2Add(
		Vector2Add(_quad->parent->screenPos, _quad->parent->origin),
		_quad->position
	);
	_quad->screenPos.x = floor(_quad->screenPos.x);
	_quad->screenPos.y = floor(_quad->screenPos.y);

	if (_quad->flags & FLAG_VISIBLE)
	{
		if (_quad->eventMask & TXQUAD_DRAW)
			_quad->Event(_quad, TXQUAD_DRAW);

		for (TxQuad *_child = _quad->childFirst; _child != NULL; _child = _child->next)
			TxQuadDraw(_child);

		if (_quad->eventMask & TXQUAD_DRAW_END)
			_quad->Event(_quad, TXQUAD_DRAW_END);
	}
}


/********************************************************************************************/

void TxQuadInit(void)
{
	TxStackInit(&gTxQuadStack, (char*)gTxQuads, TXQUAD_MAX, sizeof(TxQuad));
	TxStackInit(&gTxSchedulerStack, (char*)gTxSchedulers, TXSCHEDULER_MAX, sizeof(TxScheduler));
}

void TxQuadLock(void)
{
	gTxQuadLocked = true;
}

void TxQuadUnlock(void)
{
	gTxQuadLocked = false;
}

void TxQuadUpdateAll(void)
{
	TxQuad *_quad;

	// Scheduler
	TxSchedulerRun();

	if (gTxQuadLocked == false)
	{
		// Mouse
		if (IsMouseButtonPressed(0))
		{
			_quad = TxQuadGetPointedChild(&gTxQuadVisible, GetMousePosition());

			if (gTxQuadVisible.childLast != NULL)
				if (gTxQuadVisible.childLast->flags & FLAG_CONTEXT)
					if (!TxQuadContains(gTxQuadVisible.childLast, _quad))
						TxQuadHide(gTxQuadVisible.childLast);

			if (gTxQuadCurrent != NULL)
				if (gTxQuadCurrent != _quad)
					if (gTxQuadCurrent->eventMask & TXQUAD_UNSELECT)
						gTxQuadCurrent->Event(gTxQuadCurrent, TXQUAD_UNSELECT);

			if (_quad != NULL)
			{
				gTxQuadCurrent = _quad;
				if (gTxQuadCurrent->eventMask & TXQUAD_SELECT)
					gTxQuadCurrent->Event(gTxQuadCurrent, TXQUAD_SELECT);
			}

		}
		else if (IsMouseButtonReleased(0))
		{
			if (gTxQuadCurrent != NULL)
			{
				_quad = TxQuadGetPointedChild(&gTxQuadVisible, GetMousePosition());

				if (gTxQuadCurrent->eventMask & TXQUAD_RELEASE)
					gTxQuadCurrent->Event(gTxQuadCurrent, TXQUAD_RELEASE);

				if (_quad != NULL)
				{
					if (_quad != gTxQuadCurrent)
					{
						if (_quad->eventMask & TXQUAD_DROP)
							_quad->Event(_quad, TXQUAD_DROP);

						//if (gTxQuadCurrent->eventMask & TXQUAD_UNSELECT)
						//	gTxQuadCurrent->Event(gTxQuadCurrent, TXQUAD_UNSELECT);

						//gTxQuadCurrent = _quad;
						//if (gTxQuadCurrent->eventMask & TXQUAD_SELECT)
						//	gTxQuadCurrent->Event(gTxQuadCurrent, TXQUAD_SELECT);

					}
				}
			}
		}
	}

}

void TxQuadDrawAll(void)
{
	for (TxQuad *_quad = gTxQuadVisible.childFirst; _quad != NULL; _quad = _quad->next)
		TxQuadDraw(_quad);

	DrawRectangleLines(
		0,
		0,
		GetScreenWidth(),
		GetScreenHeight(),
		BLACK
	);

#ifdef _DEBUG_CURRENT_QUAD
	if (gTxQuadCurrent != NULL)
	{
		DrawRectangleLines(
			gTxQuadCurrent->screenPos.x,
			gTxQuadCurrent->screenPos.y,
			gTxQuadCurrent->size.x,
			gTxQuadCurrent->size.y,
			YELLOW
		);
	}
#endif

#ifdef _DEBUG_GLOBALS
	DrawText(
		TextFormat(
			"Current: %p\n",
			gTxQuadCurrent
		),
		10,
		50,
		10,
		WHITE
	);
#endif

}

