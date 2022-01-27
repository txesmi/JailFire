//********************************************************************************************
//
//   TxControls
//   Some controls based on TxQuad
//
//   Copyright José Miguel Rodríguez Chávarri aka @txes
//
//********************************************************************************************

#ifndef _TX_CONTROLS_H_
#define _TX_CONTROLS_H_

/********************************************************************************************/

//#define VECTOR2_MAX		128
//#define RECTANGLE_MAX		128
#define TXBUTTON_MAX		128
#define TXTEXT_MAX			128
#define TXTEXTUREREC_MAX	128


/********************************************************************************************/

typedef struct
{
	char *text;
	float size;
	Color color;

} TxText;

typedef struct
{
	void *ptr;
	void(*Event)(void *_ptr);

} TxButton;

typedef struct
{
	Texture2D *texture;
	Rectangle rectangle;

} TxTextureRec;


/********************************************************************************************/

enum TxControlAligment
{
	ALIGN_LEFT		= (1 << 0),
	ALIGN_CENTER_X	= (1 << 1),
	ALIGN_RIGHT		= (1 << 2),
	ALIGN_TOP		= (1 << 3),
	ALIGN_CENTER_Y	= (1 << 4),
	ALIGN_BOTTOM	= (1 << 5)
};


/********************************************************************************************/

void TxControlsInit(void);

TxQuad *TxScissorCreate(
	TxQuad *_parent,
	Vector2 _position,
	Vector2 _size
);

TxQuad *TxTextCreate(
	TxQuad *_parent,
	Vector2 _position,
	char *_text,
	float _height,
	Color _color,
	int _align
);

TxQuad *TxTextureCreate(
	TxQuad *_parent,
	Vector2 _position,
	Texture2D *_texture
);

TxQuad *TxTextureRecCreate(
	TxQuad *_parent,
	Vector2 _position,
	Texture2D *_texture,
	Rectangle _sourceRec
);

void TxTextureRecSet(
	TxQuad *_quad,
	Texture2D *_texture,
	Rectangle _sourceRec
);

TxQuad *TxButtonCreate(
	TxQuad *_parent,
	Vector2 _position,
	Vector2 _size,
	void *_ptr,
	void *_event
);

TxQuad *TxTextButtonCreate(
	TxQuad *_parent,
	Vector2 _position,
	char *_str,
	float _height,
	Color _color,
	void *_event,
	void *_ptr,
	int _align
);

TxQuad *TxTextureButtonCreate(
	TxQuad *_parent,
	Vector2 _position,
	Texture2D *_texture,
	void *_event,
	void *_ptr,
	int _align
);



#endif //_TX_CONTROLS_H_
