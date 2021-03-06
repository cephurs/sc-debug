/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "PyrMessage.h"
#include "PyrPrimitiveProto.h"
#include "PyrInterpreter.h"
#include "PyrPrimitive.h"
#include "GC.h"
#include "bullet.h"
#include <stdlib.h>
#include <assert.h>
#include "PredefinedSymbols.h"

#define DEBUGMETHODS 0
#define SANITYCHECK 0
#define METHODMETER 0

PyrMethod **gRowTable;

PyrSlot keywordstack[MAXKEYSLOTS];
bool gKeywordError = true;
extern bool gTraceInterpreter;

long cvxUniqueMethods;
#ifdef SC_WIN32
extern int ivxIdentDict_array;
#else
extern long ivxIdentDict_array;
#endif

void StoreToImmutableB(VMGlobals *g, PyrSlot *& sp, unsigned char *& ip);

void CallStackSanity(VMGlobals *g, char *tagstr);

void initUniqueMethods()
{
	PyrClass *dummyclass;
	cvxUniqueMethods = classVarOffset("Object", "uniqueMethods", &dummyclass);
}

void sendMessageWithKeys(VMGlobals *g, PyrSymbol *selector, long numArgsPushed, long numKeyArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendMessageWithKeys");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = classOfSlot(recvrSlot);

	lookup_again:
	index = classobj->classIndex.ui + selector->u.index;
	meth = gRowTable[index];

	if (meth->name.us != selector) {
		doesNotUnderstandWithKeys(g, selector, numArgsPushed, numKeyArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethodWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				g->sp -= numArgsPushed - 1;
				sp = g->sp;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(&sp[0], &sp[index]);
				} else {
					slotCopy(&sp[0], &meth->prototypeFrame.uo->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(&sp[0], &recvrSlot->uo->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = recvrSlot->uo;
				if (obj->obj_flags & obj_immutable) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], &sp[1]);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(&sp[0], recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], &sp[1]);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(&sp[0], recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				classobj = meth->ownerclass.uoc->superclass.us->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &recvrSlot->uo->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitiveWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
#if SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendMessageWithKeys");
#endif
	//postfl("<-sendMessage\n");
}


void sendMessage(VMGlobals *g, PyrSymbol *selector, long numArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendMessage");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = classOfSlot(recvrSlot);

	lookup_again:
	index = classobj->classIndex.ui + selector->u.index;
	meth = gRowTable[index];

	if (meth->name.us != selector) {
		doesNotUnderstand(g, selector, numArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethod(g, meth, numArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(&sp[0], &sp[index]);
				} else {
					slotCopy(&sp[0], &meth->prototypeFrame.uo->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(&sp[0], &recvrSlot->uo->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = recvrSlot->uo;
				if (obj->obj_flags & obj_immutable) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], &sp[1]);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(&sp[0], recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], &sp[1]);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(&sp[0], recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				classobj = meth->ownerclass.uoc->superclass.us->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &recvrSlot->uo->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitive(g, meth, numArgsPushed);
#if SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
			/*
			case methMultDispatchByClass : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					classobj = sp[index].uo->classptr;
					selector = meth->selectors.us;
					goto lookup_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			case methMultDispatchByValue : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					index = arrayAtIdentityHashInPairs(array, b);
					meth = meth->selectors.uo->slots[index + 1].uom;
					goto meth_select_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			*/

		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendMessage");
#endif
	//postfl("<-sendMessage\n");
}


void sendSuperMessageWithKeys(VMGlobals *g, PyrSymbol *selector, long numArgsPushed, long numKeyArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendSuperMessageWithKeys");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = g->method->ownerclass.uoc->superclass.us->u.classobj;
	//assert(isKindOfSlot(recvrSlot, classobj));

	lookup_again:
	index = classobj->classIndex.ui + selector->u.index;
	meth = gRowTable[index];

	if (meth->name.us != selector) {
		doesNotUnderstandWithKeys(g, selector, numArgsPushed, numKeyArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethodWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				g->sp -= numArgsPushed - 1;
				sp = g->sp;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(&sp[0], &sp[index]);
				} else {
					slotCopy(&sp[0], &meth->prototypeFrame.uo->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(&sp[0], &recvrSlot->uo->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = recvrSlot->uo;
				if (obj->obj_flags & obj_immutable) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], &sp[1]);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(&sp[0], recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], &sp[1]);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(&sp[0], recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				classobj = meth->ownerclass.uoc->superclass.us->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &recvrSlot->uo->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				numArgsPushed = keywordFixStack(g, meth, methraw, numArgsPushed, numKeyArgsPushed);
				numKeyArgsPushed = 0;
				selector = meth->selectors.us;
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitiveWithKeys(g, meth, numArgsPushed, numKeyArgsPushed);
#if SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendSuperMessageWithKeys");
#endif
	//postfl("<-sendMessage\n");
}


void sendSuperMessage(VMGlobals *g, PyrSymbol *selector, long numArgsPushed)
{
	PyrMethod *meth = NULL;
	PyrMethodRaw *methraw;
	PyrSlot *recvrSlot, *sp;
	PyrClass *classobj;
	long index;
	PyrObject *obj;

	//postfl("->sendMessage\n");
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "sendSuperMessage");
#endif
	recvrSlot = g->sp - numArgsPushed + 1;

	classobj = g->method->ownerclass.uoc->superclass.us->u.classobj;
	//assert(isKindOfSlot(recvrSlot, classobj));

	lookup_again:
	index = classobj->classIndex.ui + selector->u.index;
	meth = gRowTable[index];

	if (meth->name.us != selector) {
		doesNotUnderstand(g, selector, numArgsPushed);
	} else {
		methraw = METHRAW(meth);
		//postfl("methraw->methType %d\n", methraw->methType);
		switch (methraw->methType) {
			case methNormal : /* normal msg send */
				executeMethod(g, meth, numArgsPushed);
				break;
			case methReturnSelf : /* return self */
				g->sp -= numArgsPushed - 1;
				break;
			case methReturnLiteral : /* return literal */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &meth->selectors); /* in this case selectors is just a single value */
				break;
			case methReturnArg : /* return an argument */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex; // zero is index of the first argument
				if (index < numArgsPushed) {
					slotCopy(&sp[0], &sp[index]);
				} else {
					slotCopy(&sp[0], &meth->prototypeFrame.uo->slots[index]);
				}
				break;
			case methReturnInstVar : /* return inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				slotCopy(&sp[0], &recvrSlot->uo->slots[index]);
				break;
			case methAssignInstVar : /* assign inst var */
				sp = g->sp -= numArgsPushed - 1;
				index = methraw->specialIndex;
				obj = recvrSlot->uo;
				if (obj->obj_flags & obj_immutable) { StoreToImmutableB(g, sp, g->ip); }
				else {
					if (numArgsPushed >= 2) {
						slotCopy(&obj->slots[index], &sp[1]);
						g->gc->GCWrite(obj, sp + 1);
					} else {
						SetNil(&obj->slots[index]);
					}
					slotCopy(&sp[0], recvrSlot);
				}
				break;
			case methReturnClassVar : /* return class var */
				sp = g->sp -= numArgsPushed - 1;
				slotCopy(&sp[0], &g->classvars->slots[methraw->specialIndex]);
				break;
			case methAssignClassVar : /* assign class var */
				sp = g->sp -= numArgsPushed - 1;
				if (numArgsPushed >= 2) {
					slotCopy(&g->classvars->slots[methraw->specialIndex], &sp[1]);
					g->gc->GCWrite(g->classvars, sp + 1);
				} else {
					SetNil(&g->classvars->slots[methraw->specialIndex]);
				}
				slotCopy(&sp[0], recvrSlot);
				break;
			case methRedirect : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				goto lookup_again;
			case methRedirectSuper : /* send a different selector to self, e.g. this.subclassResponsibility */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				classobj = meth->ownerclass.uoc->superclass.us->u.classobj;
				goto lookup_again;
			case methForwardInstVar : /* forward to an instance variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				index = methraw->specialIndex;
				slotCopy(recvrSlot, &recvrSlot->uo->slots[index]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methForwardClassVar : /* forward to a class variable */
				if (numArgsPushed < methraw->numargs) { // not enough args pushed
					/* push default arg values */
					PyrSlot *pslot, *qslot;
					long m, mmax;
					pslot = g->sp;
					qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
					for (m=0, mmax=methraw->numargs - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
					numArgsPushed = methraw->numargs;
					g->sp += mmax;
				}
				selector = meth->selectors.us;
				slotCopy(recvrSlot, &g->classvars->slots[methraw->specialIndex]);

				classobj = classOfSlot(recvrSlot);

				goto lookup_again;
			case methPrimitive : /* primitive */
				doPrimitive(g, meth, numArgsPushed);
#if SANITYCHECK
	g->gc->SanityCheck();
#endif
				break;
			/*
			case methMultDispatchByClass : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					classobj = sp[index].uo->classptr;
					selector = meth->selectors.us;
					goto lookup_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			case methMultDispatchByValue : {
				index = methraw->specialIndex;
				if (index < numArgsPushed) {
					index = arrayAtIdentityHashInPairs(array, b);
					meth = meth->selectors.uo->slots[index + 1].uom;
					goto meth_select_again;
				} else {
					doesNotUnderstand(g, selector, numArgsPushed);
				}
			} break;
			*/

		}
	}
#if TAILCALLOPTIMIZE
	g->tailCall = 0;
#endif
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<sendSuperMessage");
#endif
	//postfl("<-sendMessage\n");
}

#ifdef SC_WIN32
int arrayAtIdentityHashInPairs(PyrObject *array, PyrSlot *key); // this is the implementation prototype
#else
long arrayAtIdentityHashInPairs(PyrObject *array, PyrSlot *key);
#endif

extern PyrClass *class_identdict;
void doesNotUnderstandWithKeys(VMGlobals *g, PyrSymbol *selector,
	long numArgsPushed, long numKeyArgsPushed)
{
	PyrSlot *qslot, *pslot, *pend;
	long i, index;
	PyrSlot *uniqueMethodSlot, *arraySlot, *recvrSlot, *selSlot, *slot;
	PyrClass *classobj;
	PyrMethod *meth;
	PyrObject *array;

#if SANITYCHECK
	g->gc->SanityCheck();
#endif
	// move args up by one to make room for selector
	qslot = g->sp + 1;
	pslot = g->sp + 2;
	pend = pslot - numArgsPushed + 1;
	while (pslot > pend) *--pslot = *--qslot;

	selSlot = g->sp - numArgsPushed + 2;
	SetSymbol(selSlot, selector);
	g->sp++;

	recvrSlot = selSlot - 1;

	classobj = classOfSlot(recvrSlot);

	index = classobj->classIndex.ui + s_nocomprendo->u.index;
	meth = gRowTable[index];


	if (meth->ownerclass.uoc == class_object) {
		// lookup instance specific method
		uniqueMethodSlot = &g->classvars->slots[cvxUniqueMethods];
		if (isKindOfSlot(uniqueMethodSlot, class_identdict)) {
			arraySlot = uniqueMethodSlot->uo->slots + ivxIdentDict_array;
			if ((IsObj(arraySlot) && (array = arraySlot->uo)->classptr == class_array)) {
				i = arrayAtIdentityHashInPairs(array, recvrSlot);
				if (i >= 0) {
					slot = array->slots + i;
					if (NotNil(slot)) {
						++slot;
						if (isKindOfSlot(slot, class_identdict)) {
							arraySlot = slot->uo->slots + ivxIdentDict_array;
							if ((IsObj(arraySlot) && (array = arraySlot->uo)->classptr == class_array)) {
								i = arrayAtIdentityHashInPairs(array, selSlot);
								if (i >= 0) {
									slot = array->slots + i;
									if (NotNil(slot)) {
										++slot;
										slotCopy(selSlot, recvrSlot);
										slotCopy(recvrSlot, slot);
										blockValueWithKeys(g, numArgsPushed+1, numKeyArgsPushed);
										return;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	executeMethodWithKeys(g, meth, numArgsPushed+1, numKeyArgsPushed);

#if SANITYCHECK
	g->gc->SanityCheck();
#endif
}

int blockValue(struct VMGlobals *g, int numArgsPushed);

void doesNotUnderstand(VMGlobals *g, PyrSymbol *selector,
	long numArgsPushed)
{
	PyrSlot *qslot, *pslot, *pend;
	long i, index;
	PyrSlot *uniqueMethodSlot, *arraySlot, *recvrSlot, *selSlot, *slot;
	PyrClass *classobj;
	PyrMethod *meth;
	PyrObject *array;

#if SANITYCHECK
	g->gc->SanityCheck();
#endif
	// move args up by one to make room for selector
	qslot = g->sp + 1;
	pslot = g->sp + 2;
	pend = pslot - numArgsPushed + 1;
	while (pslot > pend) *--pslot = *--qslot;

	selSlot = g->sp - numArgsPushed + 2;
	SetSymbol(selSlot, selector);
	g->sp++;

	recvrSlot = selSlot - 1;

	classobj = classOfSlot(recvrSlot);

	index = classobj->classIndex.ui + s_nocomprendo->u.index;
	meth = gRowTable[index];


	if (meth->ownerclass.uoc == class_object) {
		// lookup instance specific method
		uniqueMethodSlot = &g->classvars->slots[cvxUniqueMethods];
		if (isKindOfSlot(uniqueMethodSlot, class_identdict)) {
			arraySlot = uniqueMethodSlot->uo->slots + ivxIdentDict_array;
			if ((IsObj(arraySlot) && (array = arraySlot->uo)->classptr == class_array)) {
				i = arrayAtIdentityHashInPairs(array, recvrSlot);
				if (i >= 0) {
					slot = array->slots + i;
					if (NotNil(slot)) {
						++slot;
						if (isKindOfSlot(slot, class_identdict)) {
							arraySlot = slot->uo->slots + ivxIdentDict_array;
							if ((IsObj(arraySlot) && (array = arraySlot->uo)->classptr == class_array)) {
								i = arrayAtIdentityHashInPairs(array, selSlot);
								if (i >= 0) {
									slot = array->slots + i;
									if (NotNil(slot)) {
										++slot;
										slotCopy(selSlot, recvrSlot);
										slotCopy(recvrSlot, slot);
										blockValue(g, numArgsPushed+1);
										return;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	executeMethod(g, meth, numArgsPushed+1);

#if SANITYCHECK
	g->gc->SanityCheck();
#endif
}

void executeMethodWithKeys(VMGlobals *g, PyrMethod *meth, long allArgsPushed, long numKeyArgsPushed)
{
	PyrMethodRaw *methraw;
	PyrFrame *frame;
	PyrFrame *caller;
	PyrSlot *pslot, *qslot;
	PyrSlot *rslot;
	PyrSlot *vars;
	PyrObject *proto;
	long i, j, m, mmax, numtemps, numargs, numArgsPushed;

#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "executeMethodWithKeys");
#endif
#if DEBUGMETHODS
	if (gTraceInterpreter) {
		if (g->method) {
			postfl(" %s:%s -> %s:%s\n",
				g->method->ownerclass.uoc->name.us->name, g->method->name.us->name,
				meth->ownerclass.uoc->name.us->name, meth->name.us->name);
		} else {
			postfl(" top -> %s:%s\n",
				meth->ownerclass.uoc->name.us->name, meth->name.us->name);
		}
	}
#endif
#if METHODMETER
	if (gTraceInterpreter) {
		meth->callMeter.ui++;
	}
#endif

#if TAILCALLOPTIMIZE
	int tailCall = g->tailCall;
	if (tailCall) {
		if (tailCall == 1) {
			returnFromMethod(g);
		} else {
			returnFromBlock(g);
		}
	}
#endif

	g->execMethod = 10;

	proto = meth->prototypeFrame.uo;
	methraw = METHRAW(meth);
	numtemps = methraw->numtemps;
	numargs = methraw->numargs;
	caller = g->frame;
	numArgsPushed = allArgsPushed - (numKeyArgsPushed<<1);
	//DumpStack(g, g->sp);
	//postfl("executeMethod allArgsPushed %d numKeyArgsPushed %d\n", allArgsPushed, numKeyArgsPushed);

	frame = (PyrFrame*)g->gc->NewFrame(methraw->frameSize, 0, obj_slot, methraw->needsHeapContext);
	vars = frame->vars - 1;
	frame->classptr = class_frame;
	frame->size = FRAMESIZE + proto->size;
	SetObject(&frame->method, meth);
	SetObject(&frame->homeContext, frame);
	SetObject(&frame->context, frame);
	SetInt(&frame->line, 0);
	SetInt(&frame->character, 0);

	if (caller) {
		SetPtr(&caller->ip, g->ip);
		SetObject(&frame->caller, caller);
	} else {
		SetInt(&frame->caller, 0);
	}
	SetPtr(&frame->ip,  0);
	g->method = meth;

	g->ip = meth->code.uob->b - 1;
	g->frame = frame;
	g->block = (PyrBlock*)meth;

	g->sp -= allArgsPushed;
	qslot = g->sp;
	pslot = vars;

	if (numArgsPushed <= numargs) {	/* not enough args pushed */
		/* push all args to frame */
		for (m=0,mmax=numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);

		/* push default arg & var values */
		pslot = vars + numArgsPushed;
		qslot = proto->slots + numArgsPushed - 1;
		for (m=0, mmax=numtemps - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
	} else if (methraw->varargs) {
		PyrObject *list;
		PyrSlot *lslot;

		/* push all normal args to frame */
		for (m=0,mmax=numargs; m<mmax; ++m) slotCopy(++pslot, ++qslot);

		/* push list */
		i = numArgsPushed - numargs;
		list = newPyrArray(g->gc, i, 0, false);
		list->size = i;

		rslot = pslot+1;
		SetObject(rslot, list);
		//SetObject(vars + numargs + 1, list);

		/* put extra args into list */
		lslot = (list->slots - 1);
		// fixed and raw sizes are zero
		for (m=0,mmax=i; m<mmax; ++m) slotCopy(++lslot, ++qslot);

		if (methraw->numvars) {
			/* push default keyword and var values */
			pslot = vars + numargs + 1;
			qslot = proto->slots + numargs;
			for (m=0,mmax=methraw->numvars; m<mmax; ++m) slotCopy(++pslot, ++qslot);
		}
	} else {
		/* push all args to frame */
		for (m=0,mmax=numargs; m<mmax; ++m) slotCopy(++pslot, ++qslot);

		if (methraw->numvars) {
			/* push default keyword and var values */
			pslot = vars + numargs;
			qslot = proto->slots + numargs - 1;
			for (m=0,mmax=methraw->numvars; m<mmax; ++m) slotCopy(++pslot, ++qslot);
		}
	}
	// do keyword lookup:
	if (numKeyArgsPushed && methraw->posargs) {
		PyrSymbol **name0, **name;
		PyrSlot *key;
		name0 = meth->argNames.uosym->symbols + 1;
		key = g->sp + numArgsPushed + 1;
		for (i=0; i<numKeyArgsPushed; ++i, key+=2) {
			name = name0;
			for (j=1; j<methraw->posargs; ++j, ++name) {
				if (*name == key->us) {
					slotCopy(&vars[j+1], &key[1]);
					goto found1;
				}
			}
			if (gKeywordError) {
				post("WARNING: keyword arg '%s' not found in call to %s:%s\n",
					key->us->name, meth->ownerclass.uoc->name.us->name, meth->name.us->name);
			}
			found1: ;
		}
	}

	slotCopy(&g->receiver, &vars[1]);
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<executeMethodWithKeys");
#endif
}


void executeMethod(VMGlobals *g, PyrMethod *meth, long numArgsPushed)
{
	PyrMethodRaw *methraw;
	PyrFrame *frame;
	PyrFrame *caller;
	PyrSlot *pslot, *qslot;
	PyrSlot *rslot;
	PyrSlot *vars;
	PyrObject *proto;
	long i, m, mmax, numtemps, numargs;

#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "executeMethod");
#endif
#if DEBUGMETHODS
	if (gTraceInterpreter) {
		if (g->method) {
			postfl(" %s:%s -> %s:%s\n",
				g->method->ownerclass.uoc->name.us->name, g->method->name.us->name,
				meth->ownerclass.uoc->name.us->name, meth->name.us->name);
		} else {
			postfl(" top -> %s:%s\n",
				meth->ownerclass.uoc->name.us->name, meth->name.us->name);
		}
	}
#endif
#if METHODMETER
	if (gTraceInterpreter) {
		meth->callMeter.ui++;
	}
#endif

#if TAILCALLOPTIMIZE
	int tailCall = g->tailCall;
	if (tailCall) {
		if (tailCall == 1) {
			returnFromMethod(g);
		} else {
			returnFromBlock(g);
		}
	}
#endif

	g->execMethod = 20;

	proto = meth->prototypeFrame.uo;
	methraw = METHRAW(meth);
	numtemps = methraw->numtemps;
	numargs = methraw->numargs;

	caller = g->frame;
	//postfl("executeMethod allArgsPushed %d numKeyArgsPushed %d\n", allArgsPushed, numKeyArgsPushed);

	frame = (PyrFrame*)g->gc->NewFrame(methraw->frameSize, 0, obj_slot, methraw->needsHeapContext);
	vars = frame->vars - 1;
	frame->classptr = class_frame;
	frame->size = FRAMESIZE + proto->size;
	SetObject(&frame->method, meth);
	SetObject(&frame->homeContext, frame);
	SetObject(&frame->context, frame);
	SetInt(&frame->line, 0);
	SetInt(&frame->character, 0);

	if (caller) {
		SetPtr(&caller->ip, g->ip);
		SetObject(&frame->caller, caller);
	} else {
		SetInt(&frame->caller, 0);
	}
	SetPtr(&frame->ip,  0);
	g->method = meth;

	g->ip = meth->code.uob->b - 1;
	g->frame = frame;
	g->block = (PyrBlock*)meth;

	g->sp -= numArgsPushed;
	qslot = g->sp;
	pslot = vars;

	if (numArgsPushed <= numargs) {	/* not enough args pushed */
		/* push all args to frame */
		for (m=0,mmax=numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);

		/* push default arg & var values */
		pslot = vars + numArgsPushed;
		qslot = proto->slots + numArgsPushed - 1;
		for (m=0, mmax=numtemps - numArgsPushed; m<mmax; ++m) slotCopy(++pslot, ++qslot);
	} else if (methraw->varargs) {
		PyrObject *list;
		PyrSlot *lslot;

		/* push all normal args to frame */
		for (m=0,mmax=numargs; m<mmax; ++m) slotCopy(++pslot, ++qslot);

		/* push list */
		i = numArgsPushed - numargs;
		list = newPyrArray(g->gc, i, 0, false);
		list->size = i;

		rslot = pslot+1;
		SetObject(rslot, list);
		//SetObject(vars + numargs + 1, list);

		/* put extra args into list */
		lslot = (list->slots - 1);
		// fixed and raw sizes are zero
		for (m=0,mmax=i; m<mmax; ++m) slotCopy(++lslot, ++qslot);

		if (methraw->numvars) {
			/* push default keyword and var values */
			pslot = vars + numargs + 1;
			qslot = proto->slots + numargs;
			for (m=0,mmax=methraw->numvars; m<mmax; ++m) slotCopy(++pslot, ++qslot);
		}
	} else {
		/* push all args to frame */
		for (m=0,mmax=numargs; m<mmax; ++m) slotCopy(++pslot, ++qslot);

		if (methraw->numvars) {
			/* push default keyword and var values */
			pslot = vars + numargs;
			qslot = proto->slots + numargs - 1;
			for (m=0,mmax=methraw->numvars; m<mmax; ++m) slotCopy(++pslot, ++qslot);
		}
	}
	slotCopy(&g->receiver, &vars[1]);

#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "<executeMethod");
#endif
}

void switchToThread(VMGlobals *g, PyrThread *newthread, int oldstate, int *numArgsPushed);

void returnFromBlock(VMGlobals *g)
{
	PyrFrame *curframe;
	PyrFrame *returnFrame;
	PyrFrame *homeContext;
	PyrBlock *block;
	PyrMethod *meth;
	PyrMethodRaw *methraw;
	PyrMethodRaw *blockraw;

	//if (gTraceInterpreter) postfl("->returnFromBlock\n");
	//printf("->returnFromBlock\n");
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "returnFromBlock");
#endif
	curframe = g->frame;

//again:
	returnFrame = curframe->caller.uof;
	if (returnFrame) {
		block = curframe->method.uoblk;
		blockraw = METHRAW(block);

		g->frame = returnFrame;
		g->ip = (unsigned char *)returnFrame->ip.ui;
		g->block = returnFrame->method.uoblk;
		homeContext = returnFrame->homeContext.uof;
		meth = homeContext->method.uom;
		methraw = METHRAW(meth);
		slotCopy(&g->receiver, &homeContext->vars[0]); //??
		g->method = meth;

		meth = curframe->method.uom;
		methraw = METHRAW(meth);
		if (!methraw->needsHeapContext) {
			g->gc->Free(curframe);
		} else {
			SetNil(&curframe->caller);
		}

	} else {
		////// this should never happen .
		error("return from Function at top of call stack.\n");
		g->method = NULL;
		g->block = NULL;
		g->frame = NULL;
		g->sp = g->gc->Stack()->slots - 1;
		longjmp(g->escapeInterpreter, 1);
	}
	//if (gTraceInterpreter) postfl("<-returnFromBlock\n");
#if SANITYCHECK
	g->gc->SanityCheck();
	CallStackSanity(g, "returnFromBlock");
#endif
}


void returnFromMethod(VMGlobals *g)
{
	PyrFrame *returnFrame, *curframe, *homeContext;
	PyrMethod *meth;
	PyrMethodRaw *methraw;
	curframe = g->frame;

	//assert(curframe->context.uof == NULL);

	/*if (gTraceInterpreter) {
		post("returnFromMethod %s:%s\n", g->method->ownerclass.uoc->name.us->name, g->method->name.us->name);
		post("tailcall %d\n", g->tailCall);
	}*/
#if SANITYCHECK
	g->gc->SanityCheck();
#endif
	homeContext = curframe->context.uof->homeContext.uof;
	if (homeContext == NULL) {
		null_return:
#if TAILCALLOPTIMIZE
		if (g->tailCall) return; // do nothing.
#endif

	/*
		static bool once = true;
		if (once || gTraceInterpreter)
		{
			once = false;
			post("return all the way out. sd %d\n", g->sp - g->gc->Stack()->slots);
			postfl("%s:%s\n",
				g->method->ownerclass.uoc->name.us->name, g->method->name.us->name
			);
			post("tailcall %d\n", g->tailCall);
			post("homeContext %08X\n", homeContext);
			post("returnFrame %08X\n", returnFrame);
			dumpObjectSlot(&homeContext->caller);
			DumpStack(g, g->sp);
			DumpBackTrace(g);
		}
		gTraceInterpreter = false;
	*/
		//if (IsNil(&homeContext->caller)) return; // do nothing.

		// return all the way out.
		PyrSlot *bottom = g->gc->Stack()->slots;
		slotCopy(bottom, g->sp);
		g->sp = bottom; // ??!! pop everybody
		g->method = NULL;
		g->block = NULL;
		g->frame = NULL;
		longjmp(g->escapeInterpreter, 2);
	} else {
		returnFrame = homeContext->caller.uof;

		if (returnFrame == NULL) goto null_return;
		// make sure returnFrame is a caller and find earliest stack frame
		{
			PyrFrame *tempFrame = curframe;
			while (tempFrame != returnFrame) {
				tempFrame = tempFrame->caller.uof;
				if (!tempFrame) {
					if (isKindOf((PyrObject*)g->thread, class_routine) && NotNil(&g->thread->parent)) {
						// not found, so yield to parent thread and continue searching.
						PyrSlot value;
						slotCopy(&value, g->sp);

						int numArgsPushed = 1;
						switchToThread(g, g->thread->parent.uot, tSuspended, &numArgsPushed);

						// on the other side of the looking glass, put the yielded value on the stack as the result..
						g->sp -= numArgsPushed - 1;
						slotCopy(g->sp, &value);

						curframe = tempFrame = g->frame;
					} else {
						slotCopy(&g->sp[2], &g->sp[0]);
						slotCopy(g->sp, &g->receiver);
						g->sp++; SetObject(g->sp, g->method);
						g->sp++;
						sendMessage(g, getsym("outOfContextReturn"), 3);
						return;
					}
				}
			}
		}

		{
			PyrFrame *tempFrame = curframe;
			while (tempFrame != returnFrame) {
				meth = tempFrame->method.uom;
				methraw = METHRAW(meth);
				PyrFrame *nextFrame = tempFrame->caller.uof;
				if (!methraw->needsHeapContext) {
					g->gc->Free(tempFrame);
				} else {
					if (tempFrame != homeContext) SetNil(&tempFrame->caller);
				}
				tempFrame = nextFrame;
			}
		}

		// return to it
		g->ip = (unsigned char *)returnFrame->ip.ui;
		g->frame = returnFrame;
		g->block = returnFrame->method.uoblk;

		homeContext = returnFrame->homeContext.uof;
		meth = homeContext->method.uom;
		methraw = METHRAW(meth);

#if DEBUGMETHODS
if (gTraceInterpreter) {
	postfl("%s:%s <- %s:%s\n",
		meth->ownerclass.uoc->name.us->name, meth->name.us->name,
		g->method->ownerclass.uoc->name.us->name, g->method->name.us->name
	);
}
#endif

		g->method = meth;
		slotCopy(&g->receiver, &homeContext->vars[0]);

	}
#if SANITYCHECK
	g->gc->SanityCheck();
#endif
}


int keywordFixStack(VMGlobals *g, PyrMethod *meth, PyrMethodRaw *methraw, long allArgsPushed,
		long numKeyArgsPushed)
{
	PyrSlot *pslot, *qslot;
	long i, j, m, diff, numArgsPushed, numArgsNeeded;

	if (numKeyArgsPushed) {
		// evacuate keyword args to separate area
		pslot = keywordstack + (numKeyArgsPushed<<1);
		qslot = g->sp + 1;
		for (m=0; m<numKeyArgsPushed; ++m) {
			*--pslot = *--qslot;
			*--pslot = *--qslot;
		}
	}

	PyrSlot *vars = g->sp - allArgsPushed + 1;

	numArgsPushed = allArgsPushed - (numKeyArgsPushed<<1);
	numArgsNeeded = methraw->numargs;
	diff = numArgsNeeded - numArgsPushed;
	if (diff > 0) {  // not enough args
		pslot = vars + numArgsPushed - 1;
		qslot = meth->prototypeFrame.uo->slots + numArgsPushed - 1;
		for (m=0; m<diff; ++m) slotCopy(++pslot, ++qslot);
		numArgsPushed = numArgsNeeded;
	}

	// do keyword lookup:
	if (numKeyArgsPushed && methraw->posargs) {
		PyrSymbol **name0 = meth->argNames.uosym->symbols + 1;
		PyrSlot *key = keywordstack;
		for (i=0; i<numKeyArgsPushed; ++i, key+=2) {
			PyrSymbol **name = name0;
			for (j=1; j<methraw->posargs; ++j, ++name) {
				if (*name == key->us) {
					slotCopy(&vars[j], &key[1]);
					goto found;
				}
			}
			if (gKeywordError) {
					post("WARNING: keyword arg '%s' not found in call to %s:%s\n",
						key->us->name, meth->ownerclass.uoc->name.us->name, meth->name.us->name);
			}
			found: ;
		}
	}

	g->sp += numArgsPushed - allArgsPushed;
	return numArgsPushed;
}

