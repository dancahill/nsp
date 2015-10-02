/*
    NESLA NullLogic Embedded Scripting Language
    Copyright (C) 2007-2015 Dan Cahill

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "nsp/nsplib.h"
#include "base.h"

#ifdef HAVE_THREADS

/*
* this is the function that terminates orphans
*/
void thread_murder(nsp_state *N, obj_t *cobj)
{
#define __FN__ __FILE__ ":thread_murder()"
	OS_THREAD *thread;

	n_warn(N, __FN__, "reaper is claiming another lost soul");
	if (cobj->val->type != NT_CDATA || cobj->val->d.str == NULL || strcmp(cobj->val->d.str, "thread") != 0)
		n_error(N, NE_SYNTAX, __FN__, "expected a thread");
	thread = (OS_THREAD *)cobj->val->d.str;
	//do any checks for thread status here...
	n_free(N, (void *)&cobj->val->d.str, sizeof(OS_THREAD) + 1);
	cobj->val->size = 0;
	return;
#undef __FN__
}

NSP_FUNCTION(libnsp_base_thread_thread)
{
#define __FN__ __FILE__ ":libnsp_base_thread_thread()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	OS_THREAD *thread;

	if (!nsp_istable(thisobj)) n_error(N, NE_SYNTAX, __FN__, "expected a table for 'this'");
	if ((thread = n_alloc(N, sizeof(OS_THREAD) + 1, 1)) == NULL) {
		n_warn(N, __FN__, "couldn't alloc %d bytes", sizeof(OS_THREAD) + 1);
		return -1;
	}
	nc_strncpy(thread->obj_type, "thread", sizeof(thread->obj_type) - 1);
	thread->obj_term = (NSP_CFREE)thread_murder;
	cobj = nsp_setcdata(N, thisobj, "_thread", NULL, 0);
	cobj->val->d.str = (void *)thread;
	cobj->val->size = sizeof(OS_THREAD) + 1;

	if ((thread->N = nsp_newstate()) == NULL) return -1;
	thread->parentN = N;
	//nsp_zlink(N, &thread->N->g, &N->g);

//	if (nsp_istable((cobj = nsp_getobj(thread->N, &thread->N->g, "_GLOBALS")))) nsp_unlinkval(thread->N, cobj);
//	nsp_freetable(thread->N, &thread->N->g);

	nsp_unlinkval(thread->N, &thread->N->g);
	nsp_linkval(thread->N, &thread->N->g, &N->g);

	return 0;
#undef __FN__
}


#ifdef WIN32
static unsigned _stdcall thread_main(void *x)
#else
static void *thread_main(void *x)
#endif
{
	OS_THREAD *thread = x;
	obj_t *thisobj;
	obj_t *cobj;

	thisobj = nsp_setbool(thread->N, &thread->N->l, "this", 0);
	nsp_linkval(thread->N, thisobj, &thread->this);

	cobj = nsp_getobj(thread->N, thisobj, "_thread");
	if (cobj->val->type != NT_CDATA || cobj->val->d.str == NULL || nc_strcmp(cobj->val->d.str, "thread") != 0)
		n_error(thread->N, NE_SYNTAX, "", "expected a thread");

	nsp_exec(thread->N, "do_thread_stuff(this);");

	nsp_unlinkval(thread->N, &thread->N->g);
	if (thread->N->err) goto err;
err:
	//if (N->err) printf("%s\r\n", N->errbuf);
	nsp_freestate(thread->N);
	//if (intstatus || N->allocs != N->frees) printstate(N, fn);
//	nsp_endstate(thread->N);
	thread->N = NULL;

	nsp_state *PN = thread->parentN;
	n_free(PN, (void *)&cobj->val->d.str, sizeof(OS_THREAD) + 1);
	cobj->val->size = 0;
	cobj->val->d.num = 0;
	cobj->val->type = NT_BOOLEAN;

	return 0;
}

NSP_FUNCTION(libnsp_base_thread_start)
{
#define __FN__ __FILE__ ":libnsp_base_thread_start()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	OS_THREAD *thread;

	if (!nsp_istable(thisobj) || nsp_isnull(nsp_getobj(N, thisobj, "_thread"))) {
		thisobj = nsp_getobj(N, &N->l, "1");
	}
	if (!nsp_istable(thisobj))
		n_error(N, NE_SYNTAX, __FN__, "expected a thread");
	cobj = nsp_getobj(N, thisobj, "_thread");
	if (cobj->val->type != NT_CDATA || cobj->val->d.str == NULL || nc_strcmp(cobj->val->d.str, "thread") != 0)
		n_error(N, NE_SYNTAX, __FN__, "expected a thread");
	thread = (OS_THREAD *)cobj->val->d.str;

	nsp_linkval(N, &thread->this, thisobj);

	unsigned long int id;
	thread->handle = (HANDLE)_beginthreadex(NULL, 0, thread_main, thread, 0, &id);

	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_base_thread_finish)
{
#define __FN__ __FILE__ ":libnsp_base_thread_finish()"
	obj_t *thisobj = nsp_getobj(N, &N->l, "this");
	obj_t *cobj;
	OS_THREAD *thread;

	if (!nsp_istable(thisobj) || nsp_isnull(nsp_getobj(N, thisobj, "_thread"))) {
		thisobj = nsp_getobj(N, &N->l, "1");
	}
	if (!nsp_istable(thisobj))
		n_error(N, NE_SYNTAX, __FN__, "expected a thread");
	cobj = nsp_getobj(N, thisobj, "_thread");
	if (cobj->val->type != NT_CDATA || cobj->val->d.str == NULL || nc_strcmp(cobj->val->d.str, "thread") != 0)
		n_error(N, NE_SYNTAX, __FN__, "expected a thread");
	thread = (OS_THREAD *)cobj->val->d.str;

	thread->N->err = 0;
	n_error(thread->N, thread->N->err, __FN__, "exiting normally");

	return 0;
#undef __FN__
}

NSP_FUNCTION(libnsp_base_thread_kill)
{
#define __FN__ __FILE__ ":libnsp_base_thread_kill()"
	return 0;
#undef __FN__
}

#endif /* HAVE_THREADS */
