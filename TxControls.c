//********************************************************************************************
//
//   TxControls
//   Some controls based on TxQuad
//
//   Copyright José Miguel Rodríguez Chávarri aka @txes
//
//********************************************************************************************

#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#include "TxStack.h"
#include "TxQuad.h"
#include "TxControls.h"


//********************************************************************************************

//Vector2 gVector2s[VECTOR2_MAX] = { 0 };
//Rectangle gRectangles[RECTANGLE_MAX] = { 0 };
TxText gTxTexts[TXTEXT_MAX] = { 0 };
TxButton gTxButtons[TXBUTTON_MAX] = { 0 };
TxTextureRec gTxTexRecs[TXTEXTUREREC_MAX] = { 0 };

//TxStack gVector2Stack = { 0 };
//TxStack gRectangleStack = { 0 };
TxStack gTxTextStack = { 0 };
TxStack gTxButtonStack = { 0 };
TxStack gTxTextureRecStack = { 0 };

void TxControlsInit(void)
{
	//TxStackInit(&gVector2Stack, (char*)gVector2s, VECTOR2_MAX, sizeof(Vector2));
	//TxStackInit(&gRectangleStack, (char*)gRectangles, RECTANGLE_MAX, sizeof(Rectangle));

	TxStackInit(&gTxTextStack, (char*)gTxTexts, TXTEXT_MAX, sizeof(TxText));
	TxStackInit(&gTxButtonStack, (char*)gTxButtons, TXBUTTON_MAX, sizeof(TxButton));
	TxStackInit(&gTxTextureRecStack, (char*)gTxTexRecs, TXTEXTUREREC_MAX, sizeof(TxButton));
}


// TX_SCISSOR
//********************************************************************************************

int TxScissorEvent(
	TxQuad *_quad,
	int _eventType
)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		BeginScissorMode(
			_quad->screenPos.x,
			_quad->screenPos.y,
			_quad->size.x,
			_quad->size.y
		);

	} break;

	case TXQUAD_DRAW_END:
	{
		EndScissorMode();

	} break;

	}

	return 0;
}

TxQuad *TxScissorCreate(
	TxQuad *_parent,
	Vector2 _position,
	Vector2 _size
)
{
	TxQuad *_quad = TxQuadCreate(
		_parent,
		_position,
		_size,
		Vector2Zero(),
		TxScissorEvent,
		TXQUAD_DRAW | TXQUAD_DRAW_END,
		NULL
	);
	if (_quad == NULL)
	{
		TraceLog(LOG_FATAL, "TxScissorCreate: Unable to create quad");
		return NULL;
	}


	return _quad;
}


// TX_TEXT
//********************************************************************************************

int TxTextEvent(
	TxQuad *_quad,
	int _eventType
)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxText *_text = (TxText*)_quad->data;
		DrawText(
			_text->text,
			_quad->screenPos.x,
			_quad->screenPos.y,
			_text->size,
			_text->color
		);

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxTextStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

TxQuad *TxTextCreate(
	TxQuad *_parent,
	Vector2 _position,
	char *_text,
	float _height,
	Color _color,
	int _align
)
{
	TxText *_data = (TxText*)TxStackGet(&gTxTextStack);
	if (_data == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextCreate: Unable to create data");
		return NULL;
	}

	Vector2 _size = {
		MeasureText(_text, _height),
		_height
	};

	if (_align & ALIGN_RIGHT)
		_position.x -= _size.x;
	else if (_align & ALIGN_CENTER_X)
		_position.x -= _size.x / 2;
	if (_align & ALIGN_BOTTOM)
		_position.y -= _size.y;
	else if (_align & ALIGN_CENTER_Y)
		_position.y -= _size.y / 2;

	TxQuad *_quad = TxQuadCreate(
		_parent,
		_position,
		_size,
		Vector2Zero(),
		TxTextEvent,
		TXQUAD_DRAW | TXQUAD_REMOVE,
		(void*)_data
	);
	if (_quad == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextCreate: Unable to create quad");
		TxStackPut(&gTxTextStack, (char*)_data);
		return NULL;
	}

	_data->text = _text;
	_data->size = _height;
	_data->color = _color;

	return _quad;
}


// TX_TEXTURE
/********************************************************************************************/

int TxTextureEvent(
	TxQuad *_quad,
	int _eventType
)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		DrawTexture(
			*(Texture2D*)_quad->data,
			_quad->screenPos.x,
			_quad->screenPos.y,
			WHITE
		);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

TxQuad *TxTextureCreate(
	TxQuad *_parent,
	Vector2 _position,
	Texture2D *_texture
)
{
	Vector2 _size = {
		_texture->width,
		_texture->height
	};

	TxQuad *_quad = TxQuadCreate(
		_parent,
		_position,
		_size,
		Vector2Zero(),
		TxTextureEvent,
		TXQUAD_DRAW,
		(void*)_texture
	);
	if (_quad == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextureCreate: Unable to create quad");
		return NULL;
	}

	return _quad;
}


// TX_TEXTURE_RECTANGLE
/********************************************************************************************/

int TxTextureRecEvent(
	TxQuad *_quad,
	int _eventType
)
{
	switch (_eventType)
	{
	case TXQUAD_DRAW:
	{
		TxTextureRec *_data = (TxTextureRec*)_quad->data;
		DrawTextureRec(
			*_data->texture,
			_data->rectangle,
			_quad->screenPos,
			WHITE
		);

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxTextureRecStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

TxQuad *TxTextureRecCreate(
	TxQuad *_parent,
	Vector2 _position,
	Texture2D *_texture,
	Rectangle _recSource
)
{
	TxTextureRec *_data = TxStackGet(&gTxTextureRecStack);
	if (_data == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextureRecCreate: Unable to create data");
		return NULL;
	}


	Vector2 _size = {
		_texture->width,
		_texture->height
	};

	TxQuad *_quad = TxQuadCreate(
		_parent,
		_position,
		_size,
		Vector2Zero(),
		TxTextureRecEvent,
		TXQUAD_DRAW | TXQUAD_REMOVE,
		(void*)_data
	);
	if (_quad == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextureRecCreate: Unable to create quad");
		return NULL;
	}

	_data->texture = _texture;
	_data->rectangle = _recSource;

	return _quad;
}

//-------------------------------------------------------------------------------------------

void TxTextureRecSet(
	TxQuad *_quad,
	Texture2D *_texture,
	Rectangle _sourceRec
)
{
	TxTextureRec *_data = _quad->data;
	_data->texture = _texture;
	_data->rectangle = _sourceRec;
}


// TX_BUTTON
/********************************************************************************************/

int TxButtonEvent(
	TxQuad *_quad,
	int _eventType
)
{
	switch (_eventType)
	{
	case TXQUAD_RELEASE:
	{
		TxButton *_button = (TxButton*)_quad->data;
		_button->Event(_button->ptr);

	} break;

	case TXQUAD_REMOVE:
	{
		TxStackPut(&gTxButtonStack, (char*)_quad->data);

	} break;

	}

	return 0;
}

//-------------------------------------------------------------------------------------------

TxQuad *TxButtonCreate(
	TxQuad *_parent,
	Vector2 _position,
	Vector2 _size,
	void *_ptr,
	void *_event
)
{
	TxButton *_data = TxStackGet(&gTxButtonStack);
	if (_data == NULL)
	{
		TraceLog(LOG_FATAL, "TxButtonCreate: Unable to create data");
		return NULL;
	}

	TxQuad *_quad = TxQuadCreate(
		_parent,
		_position,
		_size,
		Vector2Zero(),
		TxButtonEvent,
		TXQUAD_RELEASE | TXQUAD_REMOVE,
		(void*)_data
	);
	if (_quad == NULL)
	{
		TraceLog(LOG_FATAL, "TxButtonCreate: Unable to create quad");
		TxStackPut(&gTxButtonStack, (char*)_data);
		return NULL;
	}

	_data->Event = _event;
	_data->ptr = _ptr;

	return _quad;
}


// TX_TEXT_BUTTON
/********************************************************************************************/

TxQuad *TxTextButtonCreate(
	TxQuad *_parent,
	Vector2 _position,
	char *_str,
	float _height,
	Color _color,
	void *_event,
	void *_ptr,
	int _align
)
{
	Vector2 _size = {
		MeasureText(_str, _height),
		_height
	};
	if (_align & ALIGN_RIGHT)
		_position.x -= _size.x;
	else if (_align & ALIGN_CENTER_X)
		_position.x -= _size.x / 2;
	if (_align & ALIGN_BOTTOM)
		_position.y -= _size.y;
	else if (_align & ALIGN_CENTER_Y)
		_position.y -= _size.y / 2;

	TxQuad *_button = TxButtonCreate(
		_parent,
		_position,
		_size,
		_ptr,
		_event
	);
	if (_button == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextButtonCreate: Unable to create button");
		return NULL;
	}

	TxQuad *_text = TxTextCreate(
		_button,
		Vector2Zero(),
		_str,
		_height,
		_color,
		0
	);
	if (_text == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextButtonCreate: Unable to create text");
		TxQuadRemove(_button);
		return NULL;
	}
	TxQuadShow(_text, MODE_UNTOUCHABLE);

	return _button;
}


// TX_TEXTURE_BUTTON
//********************************************************************************************

TxQuad *TxTextureButtonCreate(
	TxQuad *_parent,
	Vector2 _position,
	Texture2D *_texture,
	void *_event,
	void *_ptr,
	int _align
)
{
	Vector2 _size = {
		_texture->width,
		_texture->height
	};
	if (_align & ALIGN_RIGHT)
		_position.x -= _size.x;
	else if (_align & ALIGN_CENTER_X)
		_position.x -= _size.x / 2;
	if (_align & ALIGN_BOTTOM)
		_position.y -= _size.y;
	else if (_align & ALIGN_CENTER_Y)
		_position.y -= _size.y / 2;

	TxQuad *_button = TxButtonCreate(
		_parent,
		_position,
		_size,
		_ptr,
		_event
	);
	if (_button == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextureButtonCreate: Unable to create button");
		return NULL;
	}

	TxQuad *_img = TxTextureCreate(
		_button,
		Vector2Zero(),
		_texture
	);
	if (_img == NULL)
	{
		TraceLog(LOG_FATAL, "TxTextureButtonCreate: Unable to create texture");
		TxQuadRemove(_button);
		return NULL;
	}
	TxQuadShow(_img, MODE_UNTOUCHABLE);

	return _button;
}


