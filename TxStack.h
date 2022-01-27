//********************************************************************************************
//
//   TxStack
//   Static memory manager
//
//   Copyright José Miguel Rodríguez Chávarri aka @txes
//
//********************************************************************************************

#ifndef _TX_STACK_H_
#define _TX_STACK_H_

/********************************************************************************************/

#define TXLINK_MAX			256


/********************************************************************************************/

typedef struct TxLink
{
	void *data;
	struct TxLink *next;

} TxLink;

typedef struct
{
	char *data;
	int count;
	int size;
	TxLink *stack;

} TxStack;


/********************************************************************************************/

void TxStackInit(
	TxStack *_stack,
	char *_data,
	int _count,
	int _size
);

void *TxStackGet(
	TxStack *_stack
);

bool TxStackPut(
	TxStack *_stack,
	char *_data
);


/********************************************************************************************/

#endif //_TX_STACK_H_
