//********************************************************************************************
//
//   TxQuad
//   Screen quadrant input event manager
//
//   Copyright José Miguel Rodríguez Chávarri aka @txes
//
//********************************************************************************************

#ifndef _TX_QUAD_H_
#define _TX_QUAD_H_

/********************************************************************************************/

#define TXQUAD_MAX			256
#define TXSCHEDULER_MAX     256


/********************************************************************************************/

typedef struct TxQuad
{
	Vector2 position;
	Vector2 size;

	Vector2 origin;
	Vector2 screenPos;

	int(*Event)(struct TxQuad *_quad, int _eventType);
	int eventMask;

	struct TxQuad *parent;
	struct TxQuad *childFirst;
	struct TxQuad *childLast;
	struct TxQuad *prev;
	struct TxQuad *next;

	int flags;

	void *data;

} TxQuad;

typedef struct TxScheduler
{
	TxQuad *quad;
	int(*Event)(struct TxScheduler *_scheduler);
	struct TxScheduler *prev;
	struct TxScheduler *next;

} TxScheduler;


/********************************************************************************************/

enum TxQuadEvents
{
	TXQUAD_DRAW = (1 << 0),
	TXQUAD_DRAW_END = (1 << 1),

	TXQUAD_SHOW = (1 << 2),
	TXQUAD_HIDE = (1 << 3),

	TXQUAD_SELECT = (1 << 10),
	TXQUAD_UNSELECT = (1 << 11),
	TXQUAD_RELEASE = (1 << 12),

	TXQUAD_DROP = (1 << 20),

	TXQUAD_TYPE = (1 << 26),

	TXQUAD_REMOVE = (1 << 31),

	TXQUAD_EVENT_LAST
};

enum TxQuadShowModes
{
	MODE_DEFAULT,
	MODE_UNTOUCHABLE,
	MODE_MODAL,
	MODE_CONTEXT,

	MODE_LAST
};

enum TxSchedulerReturns
{
	SCH_CONTINUE = 0,
	SCH_REMOVE,

	SCH_LAST
};


/********************************************************************************************/

void TxQuadInit(void);
void TxQuadLock(void);
void TxQuadUnlock(void);
void TxQuadUpdateAll(void);
void TxQuadDrawAll(void);


/********************************************************************************************/

TxQuad *TxQuadCreate(
	TxQuad *_parent,
	Vector2 _position,
	Vector2 _size,
	Vector2 _origin,
	void *_event,
	int _eventMask,
	void *_data
);

void TxQuadShow(
	TxQuad *_quad,
	int _mode
);

void TxQuadHide(
	TxQuad *_quad
);

void TxQuadToFront(
	TxQuad *_quad
);

void TxQuadRemove(
	TxQuad *_quad
);

void TxQuadRemoveScheduled(
	TxQuad *_quad
);

bool TxQuadPointCollision(
	TxQuad *_quad,
	Vector2 _point
);

bool TxQuadQuadCollision(
	TxQuad *_quad0,
	TxQuad *_quad1,
	float _margin
);

TxQuad *TxQuadCurrentGet(void);

bool TxQuadIsVisible(
	TxQuad *_quad
);

//Vector2 TxQuadToScreenSpace(
//	TxQuad *_quad,
//	Vector2 _pos
//);

Vector2 ScreenToTxQuadSpace(
	TxQuad *_quad,
	Vector2 _pos
);


/********************************************************************************************/

TxScheduler *TxSchedulerAdd(
	TxQuad *_quad, 
	void *_event
);

void TxSchedulerRemove(
	TxScheduler *_scheduler
);


/********************************************************************************************/

#endif //_TX_QUAD_H_