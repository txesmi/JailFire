//********************************************************************************************
//
//   ------- JAIL FIRE ---------
//   a tiny click&drag adventure
//
//********************************************************************************************
//
//   This game has been created using raylib (https://www.raylib.com) 
//   for its 5K game jam (https://itch.io/jam/raylib-5k-gamejam)
//
//   Copyright José Miguel Rodríguez Chávarri aka @txes
//
//********************************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef PLATFORM_DESKTOP
	#include <emscripten/emscripten.h>
#endif

#define SUPPORT_TOUCH_AS_MOUSE

#include "raylib.h"
#include "raymath.h"

#include "TxStack.h"
#include "TxQuad.h"
#include "TxControls.h"


//********************************************************************************************

#define DRAG_THRESHOLD      10
#define TXELEMENT_MAX		128
#define TXFADED_MAX	    	128


// TX_ELEMENT
//********************************************************************************************

typedef struct {
	int type;

} TxElement;

typedef struct {
	int type;
	float counter;
	unsigned char alpha;

} TxFaded;

enum TxElementTypes
{
	ELEMENT_HAND = 1,
	ELEMENT_BURNING_DOOR,
	ELEMENT_DOOR,
	ELEMENT_BED,
	ELEMENT_TURNED_BED,
	ELEMENT_TOILET,
	ELEMENT_WINDOW,
	ELEMENT_GRILLES,
	ELEMENT_WATER,
	ELEMENT_SPRING,
	ELEMENT_BLANKET,
	ELEMENT_SHREDS,
	ELEMENT_LOCK,
	ELEMENT_ROPE,
	ELEMENT_AWL,
	ELEMENT_IRON_BAR,
	ELEMENT_DEBRIS,
	ELEMENT_HOOK,
	ELEMENT_PILLOW,
	ELEMENT_WET_CLOTH,
	ELEMENT_BROKEN_GRILLES,
	ELEMENT_STOOL,
	ELEMENT_GLASS,

	ELEMENT_TUBE,
	ELEMENT_WEB,
	ELEMENT_SLEEP,
	ELEMENT_QUESTION,
	ELEMENT_CLOSE,
	ELEMENT_BREAK,

	ELEMENT_COUNT = 64
};

enum TxElementFlags
{
	FLAG_STATIC = (1 << 30),
	FLAG_DISPERSE = (1 << 31),
};


// GLOBAL VARIABLES
//********************************************************************************************

const int gScreenWidth = 800;
const int gScreenHeight = 450;

TxElement gTxElements[TXELEMENT_MAX] = { 0 };
TxStack gTxElementStack = { 0 };
TxElement gTxFaded[TXFADED_MAX] = { 0 };
TxStack gTxFadedStack = { 0 };
void *TxElementEvents[ELEMENT_COUNT] = { 0 };

Vector2 gMouseOrigin = { 0, 0 };
Vector2 gTxQuadOrigin = { 0, 0 };

Vector2 gTxElementsMin = { 0, 0 };
Vector2 gTxElementsMax = { 0, 0 };

TxQuad *gScreenStart = NULL;
TxQuad *gScreenIntro = NULL;
TxQuad *gScreenGameplay = NULL;
TxQuad *gScreenEnd = NULL;
TxQuad *gTxQuadLab = NULL;

Vector2 gDispersionCenter = { 0 };

Texture2D gTexTitle = { 0 };
Texture2D gTexStart = { 0 };
Texture2D gTexElements = { 0 };
Texture2D gTexFlames = { 0 };
Texture2D gTexSmoke = { 0 };
Texture2D gTexEnd = { 0 };
Texture2D gTexHanging = { 0 };
Texture2D gTexStone = { 0 };

Rectangle gRecElements[64] = { 0 };
Rectangle gRecSmoke[16] = { 0 };

Music gMusicBackground = { 0 };
Sound gSoundAlarm = { 0 };


void gScreenEndCreate(void);

// TX_ELEMENT
//********************************************************************************************

int TxFadedSchedule(TxScheduler *_sch)
{
	TxQuad *_quad = _sch->quad;
	TxFaded *_data = (TxFaded*)_quad->data;
	_data->counter += GetFrameTime() * 2;
	if (_data->counter > 2.0)
	{
		TxQuadRemove(_quad);
		return SCH_REMOVE;
	}
	if (_data->counter > 1.0)
	{
		_data->alpha = (2.0f - _data->counter) * 255.0f;
	}

	return SCH_CONTINUE;
}

int TxFadedEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxFaded *_data = (TxFaded*)_quad->data;
		Color _col = WHITE;
		_col.a = _data->alpha;
		DrawTextureRec(
			gTexElements,
			gRecElements[_data->type],
			_quad->screenPos,
			_col
		);

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxFadedStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

TxQuad *TxFadedCreate(
	int _type,
	TxQuad *_parent,
	Vector2 _position
)
{
	TxFaded *_data = TxStackGet(&gTxFadedStack);
	if (_data == NULL)
	{
		TraceLog(LOG_FATAL, "TxFadedCreate: Unable to create data");
		return NULL;
	}
	Vector2 _size = {
		64,
		64
	};
	TxQuad *_quad = TxQuadCreate(
		_parent,
		_position,
		_size,
		Vector2Zero(),
		TxFadedEvent,
		TXQUAD_DRAW | TXQUAD_REMOVE,
		_data
	);
	if (_quad == NULL)
	{
		TxStackPut(&gTxFadedStack, (char*)_data);
		TraceLog(LOG_FATAL, "TxFadedCreate: Unable to create quad");
		return NULL;
	}

	_data->type = _type;
	_data->counter = 0;
	_data->alpha = 255;

	return _quad;
}

TxQuad *TxBubbleCreate(
	int _type
)
{
	Vector2 _pos = GetMousePosition();
	_pos.x -= 32;
	_pos.y -= 64;
	TxQuad *_bubble = TxFadedCreate(
		_type,
		NULL,
		_pos
	);
	TxQuadShow(_bubble, MODE_UNTOUCHABLE);
	TxSchedulerAdd(_bubble, TxFadedSchedule);

	return _bubble;
}

//-------------------------------------------------------------------------------------------

int TxFlamesSchedule(TxScheduler *_sch)
{
	TxQuad *_quad = _sch->quad;
	TxFaded *_data = (TxFaded*)_quad->data;

	_data->counter += GetFrameTime() * 128;

	if (_data->counter > 128.0f)
	{
		TxQuadRemove(_quad);
		return SCH_REMOVE;
	}

	return SCH_CONTINUE;
}

int TxFlamesEvent2(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxFaded *_data = (TxFaded*)_quad->data;
		Color _col = WHITE;
		float _counter = 64 - fabs(64 - _data->counter);
		_col.a = _data->alpha;
		int _frame = (int)(GetTime() * 64 + (long)_quad) % 64;
		Vector2 _origin = {
			_counter / 2,
			_counter
		};
		Rectangle _dst = {
			_quad->screenPos.x,
			_quad->screenPos.y,
			_counter,
			_counter
		};
		DrawTexturePro(
			gTexFlames,
			gRecElements[_frame],
			_dst,
			_origin,
			0,
			_col
		);

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxFadedStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

TxQuad *TxFlamesCreate(
	Vector2 _position
)
{
	TxFaded *_data = (TxFaded*)TxStackGet(&gTxFadedStack);
	if (_data == NULL)
	{
		TraceLog(LOG_FATAL, "TxFlamesCreate: Unable to create data");
		return NULL;
	}
	Vector2 _size = {
		64,
		64
	};
	TxQuad *_quad = TxQuadCreate(
		gScreenEnd,
		_position,
		_size,
		Vector2Zero(),
		TxFlamesEvent2,
		TXQUAD_DRAW | TXQUAD_REMOVE,
		_data
	);
	if (_quad == NULL)
	{
		TxStackPut(&gTxFadedStack, (char*)_data);
		TraceLog(LOG_FATAL, "TxFlamesCreate: Unable to create quad");
		return NULL;
	}

	_data->type = 0;
	_data->counter = 0;
	_data->alpha = 255;

	TxQuadShow(_quad, MODE_DEFAULT);

	TxSchedulerAdd(_quad, TxFlamesSchedule);

	return _quad;
}

//-------------------------------------------------------------------------------------------

TxQuad *TxElementCreate(
	int _type,
	TxQuad *_parent,
	Vector2 _position
);

void TxElementsBounds(
	TxQuad *_parent
)
{
	gTxElementsMin = (Vector2) { 0, 0 };
	gTxElementsMax = (Vector2) { gScreenWidth, gScreenHeight };

	for (TxQuad *_quad = _parent->childFirst; _quad != NULL; _quad = _quad->next)
	{
		if (_quad->position.x < gTxElementsMin.x)
			gTxElementsMin.x = _quad->position.x;
		if (_quad->position.y < gTxElementsMin.y)
			gTxElementsMin.y = _quad->position.y;
		if (_quad->position.x + _quad->size.x > gTxElementsMax.x)
			gTxElementsMax.x = _quad->position.x + _quad->size.x;
		if (_quad->position.y + _quad->size.y > gTxElementsMax.y)
			gTxElementsMax.y = _quad->position.y + _quad->size.y;
	}

}

int TxElementDisperse(
	TxScheduler *_scheduler
)
{
	TxQuad *_quad = _scheduler->quad;
	TxQuad *_parent = _quad->parent;

	if (!(TxQuadIsVisible(_quad)))
	{
		_quad->flags &= ~FLAG_DISPERSE;
		return SCH_REMOVE;
	}

	bool _modified = false;
	for (TxQuad *_child = _parent->childFirst; _child != NULL; _child = _child->next)
	{
		if (_child == _quad)
			continue;

		if (!TxQuadIsVisible(_child))
			continue;

		Vector2 _dir = Vector2Subtract(_quad->position, _child->position);
		float _length = Vector2Length(_dir);
		if (_length > 74.0f)
			continue;
		if(_length < 1)
			_dir = (Vector2) { 1, 0 };

		float _step = GetFrameTime() * 300.0f;
		_dir = Vector2Scale(_dir, _step / _length);
		Vector2 _dir2 = Vector2Subtract(_quad->position, gDispersionCenter);
		if (Vector2Length(_dir2) < 1)
			_dir2 = (Vector2) { 1, 0 };
		if (Vector2DotProduct(_dir2, _dir) > 0)
		{
			_quad->position = Vector2Add(_quad->position, _dir);
			_quad->screenPos = Vector2Add(_quad->screenPos, _dir);
		}
		else if(!(_child->flags & FLAG_STATIC))
		{
			_dir = Vector2Scale(_dir, 2.0f);
			_child->position = Vector2Subtract(_child->position, _dir);
			_child->screenPos = Vector2Subtract(_child->screenPos, _dir);
		}
		else
		{
			_quad->position = Vector2Subtract(_quad->position, _dir);
			_quad->screenPos = Vector2Subtract(_quad->screenPos, _dir);
		}

		_modified = true;

		if (!(_child->flags & (FLAG_DISPERSE | FLAG_STATIC)))
		{
			_child->flags |= FLAG_DISPERSE;
			TxSchedulerAdd(_child, TxElementDisperse);
		}
	}

	if (_modified == true)
	{
		return SCH_CONTINUE;
	}
	else
	{
		TxElementsBounds(_quad->parent);
		_quad->flags &= ~FLAG_DISPERSE;
		return SCH_REMOVE;
	}

}

int TxElementMove(
	TxScheduler *_scheduler
)
{
	TxQuad *_quad = _scheduler->quad;
	TxQuad *_parent = _quad->parent;

	Vector2 _offset = Vector2Subtract(GetMousePosition(), gMouseOrigin);
	_quad->position = Vector2Add(gTxQuadOrigin, _offset);

	if (_quad->position.x + _parent->origin.x < 0)
		_parent->origin.x += 0 - _quad->position.x - _parent->origin.x;
	if (_quad->position.x + _parent->origin.x > _parent->size.x - 64)
		_parent->origin.x -= _quad->position.x + _parent->origin.x - _parent->size.x + 64;
	if (_quad->position.y + _parent->origin.y < 0)
		_parent->origin.y += 0 - _quad->position.y - _parent->origin.y;
	if (_quad->position.y + _parent->origin.y > _parent->size.y - 64)
		_parent->origin.y -= _quad->position.y + _parent->origin.y - _parent->size.y + 64;

	TxElementsBounds(_parent);

	if (_parent->origin.x < _parent->size.x / 2 - gTxElementsMax.x)
		_parent->origin.x = _parent->size.x / 2 - gTxElementsMax.x;
	if (_parent->origin.y < _parent->size.y / 2 - gTxElementsMax.y)
		_parent->origin.y = _parent->size.y / 2 - gTxElementsMax.y;
	if (_parent->origin.x > _parent->size.x / 2 - gTxElementsMin.x)
		_parent->origin.x = _parent->size.x / 2 - gTxElementsMin.x;
	if (_parent->origin.y > _parent->size.y / 2 - gTxElementsMin.y)
		_parent->origin.y = _parent->size.y / 2 - gTxElementsMin.y;

	if (IsMouseButtonReleased(0))
		return SCH_REMOVE;
	else
		return SCH_CONTINUE;

}

void TxElementDraw(
	TxQuad *_quad
)
{
	TxQuad *_parent = _quad->parent;
	TxElement *_element = (TxElement*)_quad->data;
	Color _color = WHITE;

	float _size = _quad->size.x;
	Vector2 _spriteCenter = { _quad->size.x / 2, _quad->size.y / 2 };
	Vector2 _parentCenter = { _parent->size.x / 2, _parent->size.y / 2 };

	bool _outOfScreen = false;
	Vector2 _pos = Vector2Subtract(
		Vector2Add(
			Vector2Subtract(
				_quad->screenPos,
				_parent->screenPos
			),
			_spriteCenter
		),
		_parentCenter
	);
	if (fabs(_pos.x) > _parentCenter.x)
	{
		_outOfScreen = true;
		float _factor = _parentCenter.x / fabs(_pos.x);
		_pos = Vector2Scale(_pos, _factor);
		_color.a = (float)_color.a * _factor;
		_size *= _factor * _factor;
	}
	if (fabs(_pos.y) > _parentCenter.y)
	{
		_outOfScreen = true;
		float _factor = _parentCenter.y / fabs(_pos.y);
		_pos = Vector2Scale(_pos, _factor);
		_color.a = (float)_color.a * _factor;
		_size *= _factor * _factor;
	}
	_pos = Vector2Add(
		_pos,
		Vector2Add(
			_parentCenter,
			_parent->screenPos
		)
	);

	if (_outOfScreen == true)
	{
		Rectangle _dst = {
			_pos.x - _size / 2,
			_pos.y - _size / 2,
			_size,
			_size
		};
		DrawTexturePro(
			gTexElements,
			gRecElements[_element->type],
			_dst,
			Vector2Zero(),
			0.0f,
			_color
		);
	}
	else
	{
		DrawTextureRec(
			gTexElements,
			gRecElements[_element->type],
			Vector2Subtract(
				_pos,
				_spriteCenter
			),
			WHITE
		);
	}

}

TxQuad *TxElementGetDroped(void)
{
	TxQuad *_dropped = TxQuadCurrentGet();
	if (_dropped != NULL)
		if (_dropped->Event != NULL)
			if (TxQuadPointCollision(_dropped, GetMousePosition()))
				return _dropped;

	return NULL;

}

void TxElementTransform(
	TxQuad *_quad,
	int _type
)
{
	TxElement *_element = _quad->data;
	_element->type = _type;
	_quad->Event = TxElementEvents[_type];

}

void TxElementSingleSmoke(TxQuad *_quad)
{
	Vector2 _pos;
	int _frame = (int)(GetTime() * 16 + (long)_quad) % 16;
	_pos.x = _quad->screenPos.x + 16;
	_pos.y = _quad->screenPos.y + 10 - _frame * 3;
	DrawTextureRec(
		gTexSmoke,
		gRecSmoke[_frame],
		_pos,
		WHITE
	);

	_frame = (_frame + 8) % 16;
	_pos.x = _quad->screenPos.x + 16;
	_pos.y = _quad->screenPos.y + 10 - _frame * 3;
	DrawTextureRec(
		gTexSmoke,
		gRecSmoke[_frame],
		_pos,
		WHITE
	);

}

void TxElementSmoke(TxQuad *_quad)
{
	Vector2 _pos;
	int _frame = (int)(GetTime() * 16 + (long)_quad) % 16;
	_pos.x = _quad->screenPos.x + 8;
	_pos.y = _quad->screenPos.y + 44 - _frame * 5;
	DrawTextureRec(
		gTexSmoke,
		gRecSmoke[_frame],
		_pos,
		WHITE
	);

	_frame = (_frame + 3) % 16;
	_pos.x = _quad->screenPos.x + 24;
	_pos.y = _quad->screenPos.y + 44 - _frame * 5;
	DrawTextureRec(
		gTexSmoke,
		gRecSmoke[_frame],
		_pos,
		WHITE
	);

	_frame = (_frame + 3) % 16;
	_pos.x = _quad->screenPos.x + 12;
	_pos.y = _quad->screenPos.y + 44 - _frame * 5;
	DrawTextureRec(
		gTexSmoke,
		gRecSmoke[_frame],
		_pos,
		WHITE
	);

	_frame = (_frame + 3) % 16;
	_pos.x = _quad->screenPos.x + 20;
	_pos.y = _quad->screenPos.y + 44 - _frame * 5;
	DrawTextureRec(
		gTexSmoke,
		gRecSmoke[_frame],
		_pos,
		WHITE
	);

	_frame = (_frame + 3) % 16;
	_pos.x = _quad->screenPos.x + 16;
	_pos.y = _quad->screenPos.y + 44 - _frame * 5;
	DrawTextureRec(
		gTexSmoke,
		gRecSmoke[_frame],
		_pos,
		WHITE
	);

}

void TxElementBurn(TxQuad *_quad)
{
	int _frame = (int)(GetTime() * 60 + (long)_quad * 4) % 64;
	DrawTextureRec(
		gTexFlames,
		gRecElements[(_frame + 32) % 64],
		_quad->screenPos,
		WHITE
	);

}

//-------------------------------------------------------------------------------------------

int TxGlassEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		_quad->flags |= FLAG_DISPERSE;
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
			_dropped->position = gTxQuadOrigin;

		TxBubbleCreate(ELEMENT_CLOSE);

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxStoolEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		_quad->flags |= FLAG_DISPERSE;
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
			_dropped->position = gTxQuadOrigin;

		TxBubbleCreate(ELEMENT_CLOSE);

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBrokenGrillesEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_HOOK:
			{
				TxQuadHide(gScreenGameplay);
				TxQuadRemoveScheduled(gScreenGameplay);

				gScreenEndCreate();

				TxQuadShow(gScreenEnd, MODE_DEFAULT);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxWetClothEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		_quad->flags |= FLAG_DISPERSE;
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WET_CLOTH:
			case ELEMENT_WATER:
			{
				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}
		}
	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBurningPillowEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);
		TxElementBurn(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				TxElementTransform(_quad, ELEMENT_PILLOW);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxPillowEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		float _length = Vector2Length(
			Vector2Subtract(
				gTxQuadOrigin,
				_quad->position
			)
		);

		if (_length < DRAG_THRESHOLD)
		{
			TxElementTransform(_quad, ELEMENT_BLANKET);
		}
		else
		{
			gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
			_quad->flags |= FLAG_DISPERSE;
			TxSchedulerAdd(_quad, TxElementDisperse);
		}

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				TxElementTransform(_dropped, ELEMENT_WET_CLOTH);

				gTxQuadOrigin = _quad->position;
				_dropped->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_dropped, TxElementDisperse);

			} break;

			case ELEMENT_GLASS:
			{
				Vector2 _pos = {
					_quad->position.x + (rand() % 10) - 5,
					_quad->position.y + (rand() % 10) - 5
				};
				TxQuad *_shreds = TxElementCreate(
					ELEMENT_SHREDS,
					_quad->parent,
					_pos
				);
				gDispersionCenter = _quad->position;
				_shreds->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_shreds, TxElementDisperse);

				_dropped->position = gTxQuadOrigin;

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxHookEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);
		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		_quad->flags |= FLAG_DISPERSE;
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
			_dropped->position = gTxQuadOrigin;

		TxBubbleCreate(ELEMENT_CLOSE);

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxDebrisEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);
		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		_quad->flags |= FLAG_DISPERSE;
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
				case ELEMENT_ROPE:
				{
					TxElementTransform(_quad, ELEMENT_HOOK);

					TxQuadHide(_dropped);
					TxQuadRemoveScheduled(_dropped);

				} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxIronBarEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);
		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		_quad->flags |= FLAG_DISPERSE;
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
			_dropped->position = gTxQuadOrigin;

		TxBubbleCreate(ELEMENT_CLOSE);

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxAwlEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);
		_quad->flags |= FLAG_DISPERSE;
		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
			_dropped->position = gTxQuadOrigin;

		TxBubbleCreate(ELEMENT_CLOSE);

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxRopeEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);
		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		_quad->flags |= FLAG_DISPERSE;
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_SHREDS:
			{
				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			case ELEMENT_DEBRIS:
			{
				TxElementTransform(_quad, ELEMENT_HOOK);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxLockEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

		TxElementSingleSmoke(_quad);


	} break;

	case TXQUAD_RELEASE:
	{
		if (TxQuadPointCollision(_quad, GetMousePosition()))
		{
			TxElementTransform(_quad, ELEMENT_DOOR);
		}

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_SPRING:
			{
				Vector2 _pos = {
					_quad->position.x + (rand() % 10) - 5,
					_quad->position.y + (rand() % 10) - 5
				};
				TxQuad *_awl = TxElementCreate(
					ELEMENT_AWL,
					_quad->parent,
					_pos
				);
				gDispersionCenter = _quad->position;
				_awl->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_awl, TxElementDisperse);

				_dropped->position = gTxQuadOrigin;

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBurningShredsEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);
		TxElementBurn(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				TxElementTransform(_quad, ELEMENT_WET_CLOTH);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxShredsEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);
		_quad->flags |= FLAG_DISPERSE;
		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				TxElementTransform(_dropped, ELEMENT_WET_CLOTH);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			case ELEMENT_SHREDS:
			{
				TxElementTransform(_quad, ELEMENT_ROPE);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBurningBlanketEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);
		TxElementBurn(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				TxElementTransform(_quad, ELEMENT_BLANKET);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBlanketEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		float _length = Vector2Length(
			Vector2Subtract(
				gTxQuadOrigin,
				_quad->position
			)
		);

		if (_length < DRAG_THRESHOLD)
		{
			TxElementTransform(_quad, ELEMENT_BED);
		}
		else
		{
			gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
			_quad->flags |= FLAG_DISPERSE;
			TxSchedulerAdd(_quad, TxElementDisperse);
		}

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_GLASS:
			{
				Vector2 _pos = {
					_quad->position.x + (rand() % 10) - 5,
					_quad->position.y + (rand() % 10) - 5
				};
				TxQuad *_shreds = TxElementCreate(
					ELEMENT_SHREDS,
					_quad->parent,
					_pos
				);
				gDispersionCenter = _quad->position;
				_shreds->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_shreds, TxElementDisperse);

				_dropped->position = gTxQuadOrigin;

			} break;

			case ELEMENT_WATER:
			{
				TxElementTransform(_dropped, ELEMENT_WET_CLOTH);

				gTxQuadOrigin = _quad->position;
				_dropped->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_dropped, TxElementDisperse);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxSpringEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		float _length = Vector2Length(
			Vector2Subtract(
				gTxQuadOrigin,
				_quad->position
			)
		);

		if (_length < DRAG_THRESHOLD)
		{
			TxElementTransform(_quad, ELEMENT_TURNED_BED);
		}
		else
		{
			gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
			_quad->flags |= FLAG_DISPERSE;
			TxSchedulerAdd(_quad, TxElementDisperse);
		}

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
			_dropped->position = gTxQuadOrigin;

		TxBubbleCreate(ELEMENT_CLOSE);

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxWaterEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);
		_quad->flags |= FLAG_DISPERSE;
		gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
		TxSchedulerAdd(_quad, TxElementDisperse);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_SHREDS:
			case ELEMENT_BLANKET:
			case ELEMENT_PILLOW:
			{
				TxElementTransform(_quad, ELEMENT_WET_CLOTH);

				_dropped->position = gTxQuadOrigin;

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxGrillesEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_AWL:
			{
				TxElementTransform(_quad, ELEMENT_BROKEN_GRILLES);

				_dropped->position = gTxQuadOrigin;

				Vector2 _pos = {
					_quad->position.x + (rand() % 10) - 5,
					_quad->position.y + (rand() % 10) - 5
				};
				TxQuad *_bar = TxElementCreate(
					ELEMENT_IRON_BAR,
					_quad->parent,
					_pos
				);

				_bar->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_bar, TxElementDisperse);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxTurnedBedEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		float _length = Vector2Length(
			Vector2Subtract(
				gTxQuadOrigin,
				_quad->position
			)
		);

		if (_length < DRAG_THRESHOLD)
		{
			TxElementTransform(_quad, ELEMENT_SPRING);
		}
		else
		{
			TxQuadShow(_quad, MODE_DEFAULT);
			_quad->flags |= FLAG_DISPERSE;
			gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
			TxSchedulerAdd(_quad, TxElementDisperse);
		}

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_HAND:
			{
				TxElementTransform(_quad, ELEMENT_BED);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBurningBedEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);
		TxElementBurn(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				TxElementTransform(_quad, ELEMENT_BED);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBedEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadToFront(_quad);
		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxElementMove);

	} break;

	case TXQUAD_RELEASE:
	{
		TxQuadShow(_quad, MODE_DEFAULT);

		float _length = Vector2Length(
			Vector2Subtract(
				gTxQuadOrigin,
				_quad->position
			)
		);

		if (_length < DRAG_THRESHOLD)
		{
			TxElementTransform(_quad, ELEMENT_PILLOW);
		}
		else
		{
			TxQuadShow(_quad, MODE_DEFAULT);
			_quad->flags |= FLAG_DISPERSE;
			gDispersionCenter = ScreenToTxQuadSpace(_quad->parent, GetMousePosition());
			TxSchedulerAdd(_quad, TxElementDisperse);
		}

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_HAND:
			{
				TxElementTransform(_quad, ELEMENT_TURNED_BED);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxToiletEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_HAND:
			{
				Vector2 _pos = {
					_quad->position.x + (rand() % 10) - 5,
					_quad->position.y + (rand() % 10) - 5
				};
				TxQuad *_water = TxElementCreate(
					ELEMENT_WATER,
					_quad->parent,
					_pos
				);
				gDispersionCenter = _quad->position;
				_water->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_water, TxElementDisperse);

			} break;

			case ELEMENT_PILLOW:
			case ELEMENT_BLANKET:
			{
				Vector2 _pos = {
					_quad->position.x + (rand() % 10) - 5,
					_quad->position.y + (rand() % 10) - 5
				};
				TxQuad *_cloth = TxElementCreate(
					ELEMENT_WET_CLOTH,
					_quad->parent,
					_pos
				);
				gDispersionCenter = _quad->position;
				_cloth->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_cloth, TxElementDisperse);

				_dropped->position = gTxQuadOrigin;

			} break;

			case ELEMENT_SHREDS:
			{
				TxElementTransform(_dropped, ELEMENT_WET_CLOTH);
				gDispersionCenter = _quad->position;
				_dropped->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_dropped, TxElementDisperse);

			} break;

			case ELEMENT_IRON_BAR:
			{
				TxElementTransform(_quad, ELEMENT_DEBRIS);

				_dropped->position = gTxQuadOrigin;

			} break;

			case ELEMENT_WATER:
			{
				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxDoorEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);
		TxElementSmoke(_quad);

	} break;

	case TXQUAD_RELEASE:
	{
		if (TxQuadPointCollision(_quad, GetMousePosition()))
		{
			TxElementTransform(_quad, ELEMENT_LOCK);
		}

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WET_CLOTH:
			case ELEMENT_WATER:
			{
				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxTrapDoorEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);
		TxElementSmoke(_quad);

	} break;

	case TXQUAD_RELEASE:
	{
		TxElementTransform(_quad, ELEMENT_BURNING_DOOR);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			case ELEMENT_WET_CLOTH:
			{
				TxElementTransform(_quad, ELEMENT_DOOR);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			default:
			{
				TxElementTransform(_quad, ELEMENT_BURNING_DOOR);
				_dropped->position = gTxQuadOrigin;

			} break;

			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxBurningDoorEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);
		TxElementBurn(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_WATER:
			{
				_quad->Event = TxTrapDoorEvent;

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			case ELEMENT_WET_CLOTH:
			{
				TxElementTransform(_quad, ELEMENT_DOOR);

				TxQuadHide(_dropped);
				TxQuadRemoveScheduled(_dropped);

			} break;

			case ELEMENT_BLANKET:
			{
				_dropped->Event = TxBurningBlanketEvent;

				gTxQuadOrigin = _quad->position;
				_dropped->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_dropped, TxElementDisperse);

			} break;

			case ELEMENT_PILLOW:
			{
				_dropped->Event = TxBurningPillowEvent;

				gTxQuadOrigin = _quad->position;
				_dropped->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_dropped, TxElementDisperse);

			} break;

			case ELEMENT_SHREDS:
			{
				_dropped->Event = TxBurningShredsEvent;

				gTxQuadOrigin = _quad->position;
				_dropped->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_dropped, TxElementDisperse);

			} break;

			case ELEMENT_BED:
			case ELEMENT_TURNED_BED:
			{
				_dropped->Event = TxBurningBedEvent;

				gTxQuadOrigin = _quad->position;
				_dropped->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_dropped, TxElementDisperse);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxWindowEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_DROP:
	{
		TxQuad *_dropped = TxElementGetDroped();
		if (_dropped != NULL)
		{
			int _type = _dropped->Event(_dropped, TXQUAD_TYPE);
			switch (_type)
			{
			case ELEMENT_BED:
			case ELEMENT_TURNED_BED:
			case ELEMENT_STOOL:
			{
				TxElementTransform(_quad, ELEMENT_GRILLES);

				_dropped->position = gTxQuadOrigin;

				Vector2 _pos = {
					_quad->position.x,
					_quad->position.y + 2
				};
				TxQuad *_glass = TxElementCreate(
					ELEMENT_GLASS,
					_quad->parent,
					_pos
				);
				gDispersionCenter = _quad->position;
				_glass->flags |= FLAG_DISPERSE;
				TxSchedulerAdd(_glass, TxElementDisperse);

			} break;

			default:
			{
				_dropped->position = gTxQuadOrigin;

				TxBubbleCreate(ELEMENT_CLOSE);

			} break;
			}

		}

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

int TxHandMove(TxScheduler *_scheduler)
{
	TxQuad *_quad = _scheduler->quad;

	Vector2 _offset = Vector2Subtract(GetMousePosition(), gMouseOrigin);
	_quad->position = Vector2Add(gTxQuadOrigin, _offset);

	if (IsMouseButtonReleased(0))
	{
		return SCH_REMOVE;
	}
	else
	{
		return SCH_CONTINUE;
	}
}

int TxHandEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementDraw(_quad);

	} break;

	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->position;

		TxQuadShow(_quad, MODE_UNTOUCHABLE);

		TxSchedulerAdd(_quad, TxHandMove);

	} break;

	case TXQUAD_RELEASE:
	{
		_quad->position = gTxQuadOrigin;
		TxQuadShow(_quad, MODE_DEFAULT);

	} break;

	case TXQUAD_TYPE:
	{
		TxElement *_element = (TxElement*)_quad->data;
		return _element->type;

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxElementStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

TxQuad *TxElementCreate(
	int _type,
	TxQuad *_parent,
	Vector2 _position
)
{
	TxElement *_element = TxStackGet(&gTxElementStack);
	if (_element == NULL)
	{
		TraceLog(LOG_FATAL, "TxElementCreate: Unable to create element");
		return NULL;
	}

	Vector2 _size = { 64, 64 };
	TxQuad *_quad = TxQuadCreate(
		_parent,
		_position,
		_size,
		Vector2Zero(),
		TxElementEvents[_type],
		TXQUAD_DRAW | TXQUAD_SELECT | TXQUAD_RELEASE | TXQUAD_DROP | TXQUAD_REMOVE,
		_element
	);
	if (_quad == NULL)
	{
		TraceLog(LOG_FATAL, "TxElementCreate: Unable to create quad");
		TxStackPut(&gTxElementStack, (char*)_element);
		return NULL;
	}

	_element->type = _type;

	TxElementsBounds(_quad->parent);

	return _quad;
}


// END SCREEN
//********************************************************************************************

int gScreenEndWait(TxScheduler *_sch)
{
	if (IsMouseButtonReleased(0))
	{
		StopMusicStream(gMusicBackground);

		TxQuadRemove(gScreenEnd);

		TxQuadShow(gScreenStart, MODE_DEFAULT);

		return SCH_REMOVE;
	}

	return SCH_CONTINUE;
}

int gScreenEndFall(TxScheduler *_sch)
{
	static float _speed = 0;
	_speed += GetFrameTime() * 500;
	TxQuad *_quad = _sch->quad;
	_quad->position.y += _speed * GetFrameTime();
	if (_quad->position.y > gScreenHeight)
	{
		_speed = 0;

		Vector2 _pos = {
			gScreenEnd->size.x / 2 - 32,
			gScreenEnd->size.y / 2 - 32,
		};
		TxTextureRecCreate(
			gScreenEnd,
			_pos,
			&gTexElements,
			gRecElements[ELEMENT_CLOSE]
		);

		TxQuadRemove(_quad);

		PlaySound(gSoundAlarm);

		_sch->Event = gScreenEndWait;
	}

	return SCH_CONTINUE;
}

int gScreenEndSchedule(TxScheduler *_sch)
{
	static int _counter = -250;
	_counter += 5;
	SetMusicVolume(gMusicBackground, 1.0f - (float)(_counter + 250) / 506.0f);

	if (_counter > 0)
	{
		if (_counter % 20 == 0)
		{
			Vector2 _pos = {
				273 + _counter,
				190 - abs(_counter - 128) / 8
			};
			TxFlamesCreate(_pos);

		}
		if (_counter > 256)
		{
			_sch->Event = gScreenEndFall;
			_counter = -250;
		}
	}

	return SCH_CONTINUE;
}

void gScreenEndCreate(void)
{
	Vector2 _pos, _size;

	_size.x = gScreenWidth;
	_size.y = gScreenHeight;
	gScreenEnd = TxQuadCreate(
		NULL,
		Vector2Zero(),
		_size,
		Vector2Zero(),
		NULL,
		0,
		NULL
	);

	_pos.x = gScreenWidth / 2 - 256;
	_pos.y = gScreenHeight / 2 - 128;
	TxTextureCreate(
		gScreenEnd,
		_pos,
		&gTexEnd
	);

	_pos.x += 129;
	_pos.y += 68;
	TxQuad *_hanging = TxTextureCreate(
		gScreenEnd,
		_pos,
		&gTexHanging
	);

	_pos.x = 291;
	_pos.y = 0;
	TxTextureCreate(
		_hanging,
		_pos,
		&gTexStone
	);

	TxSchedulerAdd(_hanging, gScreenEndSchedule);

}


// GAMEPLAY SCREEN
//********************************************************************************************

int GameMoveOriginEvent(TxScheduler *_sch)
{
	TxQuad *_quad = _sch->quad;

	Vector2 _offset = Vector2Subtract(GetMousePosition(), gMouseOrigin);
	_quad->origin = Vector2Add(gTxQuadOrigin, _offset);

	if (_quad->origin.x < _quad->size.x / 2 - gTxElementsMax.x)
		_quad->origin.x = _quad->size.x / 2 - gTxElementsMax.x;
	if (_quad->origin.y < _quad->size.y / 2 - gTxElementsMax.y)
		_quad->origin.y = _quad->size.y / 2 - gTxElementsMax.y;
	if (_quad->origin.x > _quad->size.x / 2 - gTxElementsMin.x)
		_quad->origin.x = _quad->size.x / 2 - gTxElementsMin.x;
	if (_quad->origin.y > _quad->size.y / 2 - gTxElementsMin.y)
		_quad->origin.y = _quad->size.y / 2 - gTxElementsMin.y;

	if (IsMouseButtonReleased(0))
	{
		TxQuadUnlock();
		return SCH_REMOVE;
	}
	else
	{
		return SCH_CONTINUE;
	}
}

int gScreenGamePlayEvent(
	TxQuad *_quad,
	int _eventType
)
{
	switch (_eventType)
	{
	case TXQUAD_SELECT:
	{
		gMouseOrigin = GetMousePosition();
		gTxQuadOrigin = _quad->origin;

		TxSchedulerAdd(_quad, GameMoveOriginEvent);

		TxQuadLock();

	} break;

	}

	return 0;
}

void gScreenGameplayCreate(void)
{
	Vector2 _pos, _size;

	_size.x = gScreenWidth;
	_size.y = gScreenHeight;
	gScreenGameplay = TxQuadCreate(
		NULL,
		Vector2Zero(),
		_size,
		Vector2Zero(),
		NULL,
		0,
		NULL
	);

	_pos.x = 96;
	_pos.y = 16;
	_size.x = gScreenWidth - 112;
	_size.y = gScreenHeight - 32;
	gTxQuadLab = TxQuadCreate(
		gScreenGameplay,
		_pos,
		_size,
		Vector2Zero(),
		gScreenGamePlayEvent,
		TXQUAD_SELECT,
		NULL
	);

	_pos.x = 8;
	_pos.y = 8;
	TxElementCreate(
		ELEMENT_HAND,
		gScreenGameplay,
		_pos
	);

	_pos.x = gTxQuadLab->size.x / 2 - 110;
	_pos.y = 70;
	TxQuad *_door = TxElementCreate(
		ELEMENT_BURNING_DOOR,
		gTxQuadLab,
		_pos
	);
	_door->flags |= FLAG_STATIC;

	_pos.x = gTxQuadLab->size.x / 2 + 10;
	_pos.y = 260;
	TxQuad *_window = TxElementCreate(
		ELEMENT_WINDOW,
		gTxQuadLab,
		_pos
	);
	_window->flags |= FLAG_STATIC;

	_pos.x = gTxQuadLab->size.x / 2 + 42;
	_pos.y = 110;
	TxQuad *_toilet = TxElementCreate(
		ELEMENT_TOILET,
		gTxQuadLab,
		_pos
	);
	_toilet->flags |= FLAG_STATIC;

	_pos.x = gTxQuadLab->size.x / 2 - 190;
	_pos.y = 210;
	TxElementCreate(
		ELEMENT_BED,
		gTxQuadLab,
		_pos
	);

	_pos.x = gTxQuadLab->size.x / 2 - 130;
	_pos.y = 270;
	TxElementCreate(
		ELEMENT_STOOL,
		gTxQuadLab,
		_pos
	);
}


// INTRO SCREEN
//********************************************************************************************

int IntroEvent3(TxScheduler *_sch)
{
	static float _timer = 0;

	_timer += GetFrameTime();
	if (_timer > 1.0f)
	{
		_timer = 0;

		PlayMusicStream(gMusicBackground);

		TxQuadRemove(_sch->quad);

		return SCH_REMOVE;
	}
	else
	{
		return SCH_CONTINUE;
	}
}

int IntroEvent2(TxScheduler *_sch)
{
	static float _timer = 0;

	_timer += GetFrameTime();
	if (_timer > 2.0f)
	{
		TxQuad *_quad = _sch->quad;
		_timer = 0;

		_sch->Event = IntroEvent3;

		gScreenGameplayCreate();
		_sch->quad = TxTextureRecCreate(
			gScreenGameplay,
			_quad->position,
			&gTexElements,
			gRecElements[ELEMENT_QUESTION]
		);

		TxQuadRemoveScheduled(gScreenIntro);

		TxQuadShow(gScreenGameplay, MODE_DEFAULT);
	}
	
	return SCH_CONTINUE;
}

int IntroEvent(TxScheduler *_sch)
{
	static float _timer = 0;

	_timer += GetFrameTime();
	if (_timer > 2.0f)
	{
		TxQuad *_quad = _sch->quad;
		_timer = 0;

		_sch->Event = IntroEvent2;

		TxTextureRecSet(
			_quad,
			&gTexElements,
			gRecElements[ELEMENT_QUESTION]
		);
	}
	
	return SCH_CONTINUE;
}

void gScreenIntroCreate(void)
{
	Vector2 _pos, _size;

	_size.x = gScreenWidth;
	_size.y = gScreenHeight;
	gScreenIntro = TxQuadCreate(
		NULL,
		Vector2Zero(),
		_size,
		Vector2Zero(),
		NULL,
		0,
		NULL
	);

	_pos.x = gScreenStart->size.x / 2.0f - 32.0f;
	_pos.y = gScreenStart->size.y / 2.0f - 32.0f;
	TxTextureRecCreate(
		gScreenIntro,
		_pos,
		&gTexElements,
		gRecElements[ELEMENT_SLEEP]
	);

}


// GAME START SCREEN
//********************************************************************************************

int TxFlamesEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementBurn(_quad);

	} break;

	}

	return 0;
}

int TxSmokeEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementSmoke(_quad);

	} break;

	}

	return 0;
}

int TxSmokeSingleEvent(TxQuad *_quad, int _eventType)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxElementSingleSmoke(_quad);

	} break;

	}

	return 0;
}

void GameStartEvent(
	void *_ptr
)
{
	TxQuadHide(gScreenStart);

	SetMusicVolume(gMusicBackground, 1.0f);
	PlaySound(gSoundAlarm);

	gScreenIntroCreate();
	TxQuadShow(gScreenIntro, MODE_DEFAULT);

	TxSchedulerAdd(gScreenIntro->childLast, IntroEvent);
}

void gScreenStartCreate(void)
{
	Vector2 _pos, _size;

	_size.x = gScreenWidth;
	_size.y = gScreenHeight;
	gScreenStart = TxQuadCreate(
		NULL,
		Vector2Zero(),
		_size,
		Vector2Zero(),
		NULL,
		0,
		NULL
	);

	_size.x = gTexTitle.width;
	_size.y = gTexTitle.height;
	_pos.x = gScreenStart->size.x / 2.0f - _size.x / 2.0f;
	_pos.y = gScreenStart->size.y / 2.0f - 160.0f;
	TxQuad *_texTitle = TxTextureCreate(
		gScreenStart,
		_pos,
		&gTexTitle
	);

	// Smoke
	_pos.x = 169;
	_pos.y = -11;
	_size.x = 64;
	_size.y = 64;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 417;
	_pos.y = 84;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 231;
	_pos.y = 123;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 296;
	_pos.y = -11;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 51;
	_pos.y = 123;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 163;
	_pos.y = 82;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeSingleEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 296;
	_pos.y = 132;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeSingleEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 31;
	_pos.y = 116;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeSingleEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 98;
	_pos.y = 80;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeSingleEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 362;
	_pos.y = 80;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeSingleEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 230;
	_pos.y = 41;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxSmokeSingleEvent,
		TXQUAD_DRAW,
		NULL
	);


	// Flames
	_pos.x = 169;
	_pos.y = -11;
	_size.x = 64;
	_size.y = 64;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxFlamesEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 417;
	_pos.y = 84;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxFlamesEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 231;
	_pos.y = 123;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxFlamesEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 296;
	_pos.y = -11;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxFlamesEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = 51;
	_pos.y = 123;
	TxQuadCreate(
		_texTitle,
		_pos,
		_size,
		Vector2Zero(),
		TxFlamesEvent,
		TXQUAD_DRAW,
		NULL
	);

	_pos.x = gScreenStart->size.x / 2.0f;
	_pos.y = gScreenStart->size.y / 2.0f + 120;

	TxTextureButtonCreate(
		gScreenStart,
		_pos,
		&gTexStart,
		GameStartEvent,
		NULL,
		ALIGN_CENTER_X | ALIGN_TOP
	);

}


// RESOURCES
//********************************************************************************************

void GameTexturesLoad(void)
{
	gTexTitle = LoadTexture("resources/title.png");
	gTexStart = LoadTexture("resources/start.png");
	gTexElements = LoadTexture("resources/elements.png");
	SetTextureFilter(gTexElements, TEXTURE_FILTER_BILINEAR);
	gTexFlames = LoadTexture("resources/flames.png");
	gTexSmoke = LoadTexture("resources/smoke.png");
	gTexEnd = LoadTexture("resources/end.png");
	gTexHanging = LoadTexture("resources/hanging.png");
	gTexStone = LoadTexture("resources/stone.png");

}

void GameTexturesUnload(void)
{
	UnloadTexture(gTexTitle);
	UnloadTexture(gTexStart);
	UnloadTexture(gTexElements);
	UnloadTexture(gTexFlames);
	UnloadTexture(gTexSmoke);
	UnloadTexture(gTexEnd);
	UnloadTexture(gTexHanging);
	UnloadTexture(gTexStone);

}

void GameSoundsLoad(void)
{
	gMusicBackground = LoadMusicStream("resources/dead-home.ogg");
	gSoundAlarm = LoadSound("resources/alarm.ogg");

}

void GameSoundsUnload(void)
{
	StopMusicStream(gMusicBackground);
	UnloadMusicStream(gMusicBackground);
	UnloadSound(gSoundAlarm);

}


// GAME
//********************************************************************************************

void GameLoop(void)
{
	// Update
	//------------------------------------------------------------------------------------
	UpdateMusicStream(gMusicBackground);
	TxQuadUpdateAll();

	// Draw
	//------------------------------------------------------------------------------------
	BeginDrawing();
	ClearBackground(WHITE);

	TxQuadDrawAll();

	EndDrawing();
}

//-------------------------------------------------------------------------------------------

bool GameInit(void)
{
	// Init modules
	TxQuadInit();
	TxControlsInit();

	TxStackInit(&gTxElementStack, (char*)gTxElements, TXELEMENT_MAX, sizeof(TxElement));
	TxStackInit(&gTxFadedStack, (char*)gTxFaded, TXFADED_MAX, sizeof(TxFaded));

	// Load resources
	GameTexturesLoad();
	GameSoundsLoad();

	// Fill TxElementEvents
	TxElementEvents[ELEMENT_HAND] = TxHandEvent;
	TxElementEvents[ELEMENT_BURNING_DOOR] = TxBurningDoorEvent;
	TxElementEvents[ELEMENT_DOOR] = TxDoorEvent;
	TxElementEvents[ELEMENT_BED] = TxBedEvent;
	TxElementEvents[ELEMENT_TURNED_BED] = TxTurnedBedEvent;
	TxElementEvents[ELEMENT_TOILET] = TxToiletEvent;
	TxElementEvents[ELEMENT_WINDOW] = TxWindowEvent;
	TxElementEvents[ELEMENT_GRILLES] = TxGrillesEvent;
	TxElementEvents[ELEMENT_WATER] = TxWaterEvent;
	TxElementEvents[ELEMENT_BLANKET] = TxBlanketEvent;
	TxElementEvents[ELEMENT_SPRING] = TxSpringEvent;
	TxElementEvents[ELEMENT_SHREDS] = TxShredsEvent;
	TxElementEvents[ELEMENT_LOCK] = TxLockEvent;
	TxElementEvents[ELEMENT_ROPE] = TxRopeEvent;
	TxElementEvents[ELEMENT_AWL] = TxAwlEvent;
	TxElementEvents[ELEMENT_IRON_BAR] = TxIronBarEvent;
	TxElementEvents[ELEMENT_DEBRIS] = TxDebrisEvent;
	TxElementEvents[ELEMENT_HOOK] = TxHookEvent;
	TxElementEvents[ELEMENT_PILLOW] = TxPillowEvent;
	TxElementEvents[ELEMENT_WET_CLOTH] = TxWetClothEvent;
	TxElementEvents[ELEMENT_BROKEN_GRILLES] = TxBrokenGrillesEvent;
	TxElementEvents[ELEMENT_STOOL] = TxStoolEvent;
	TxElementEvents[ELEMENT_GLASS] = TxGlassEvent;


	// Init gTexElements source rectangles
	Rectangle *_rec = gRecElements;
	for (int _y = 0; _y < 8; _y += 1)
	{
		for (int _x = 0; _x < 8; _x += 1, ++_rec)
		{
			_rec->x = _x * 64;
			_rec->y = _y * 64;
			_rec->width = 64;
			_rec->height = 64;
		}
	}

	// Init gTexSmoke source rectangles
	_rec = gRecSmoke;
	for (int _y = 0; _y < 4; _y += 1)
	{
		for (int _x = 0; _x < 4; _x += 1, ++_rec)
		{
			_rec->x = _x * 32;
			_rec->y = _y * 32;
			_rec->width = 32;
			_rec->height = 32;
		}
	}

	// Show starting screen
	gScreenStartCreate();
	TxQuadShow(gScreenStart, MODE_DEFAULT);

	return true;
}

void GameClose(void)
{
	GameTexturesUnload();
	GameSoundsUnload();

	//TxQuadRemoveAll();
}


// MAIN
//********************************************************************************************

int main(void)
{
	InitWindow(gScreenWidth, gScreenHeight, "Jail Fire: a tiny click&drag adventure");
	InitAudioDevice(); 
	GameInit();

#ifndef PLATFORM_DESKTOP
	emscripten_set_main_loop(GameLoop, 60, 1);
#else
	SetTargetFPS(60);

	while (!WindowShouldClose())
	{
		GameLoop();
	}
#endif

	GameClose();
	CloseAudioDevice();
	CloseWindow();

	return 0;
}

