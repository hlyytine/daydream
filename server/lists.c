/*
   Dynamic list handling routines

   These look just like AmigaOS implementation of exec's dynamic lists :)
    
   */

#include <daydream.h>

struct List * NewList(void)
{
	struct List *myl;
	
	myl=(struct List *)malloc(sizeof(struct List));
	myl->lh_Head=(struct Node *)&myl->lh_Tail;
	myl->lh_Tail=0;
	myl->lh_TailPred=(struct Node *)&myl->lh_Head;
	return myl;
}  

void AddHead(struct List *myl, struct Node *myn)
{
	struct Node *tnod;
	
	if (myl->lh_TailPred==(struct Node *)&myl->lh_Head) myl->lh_TailPred=myn;
	tnod=myl->lh_Head;
	myl->lh_Head=myn;
	myn->ln_Succ=tnod;
	myn->ln_Pred=(struct Node *)myl;
	tnod->ln_Pred=myn;
} 

void AddTail(struct List *myl, struct Node *myn)
{
	struct Node *tnod;
	
	tnod=myl->lh_TailPred;
	tnod->ln_Succ=myn;
	myl->lh_TailPred=myn;
	myn->ln_Succ=(struct Node *)&myl->lh_Tail;
	myn->ln_Pred=tnod;
}

void Remove(struct Node *myn)
{
	struct Node *pnod;
	struct Node *pnod2;
	
	pnod=myn->ln_Pred;
	pnod->ln_Succ=myn->ln_Succ;
	pnod2=myn->ln_Succ;
	pnod2->ln_Pred=pnod;
}

struct Node *RemTail(struct List *myl)
{
	struct Node *tnod;
	struct Node *pnod;
	
	if (myl->lh_TailPred == (struct Node *)&myl->lh_Head) return 0;
	
	tnod=myl->lh_TailPred;
	pnod=tnod->ln_Pred;
	pnod->ln_Succ=(struct Node *)&myl->lh_Tail;
	myl->lh_TailPred=pnod;
	return tnod;
}
