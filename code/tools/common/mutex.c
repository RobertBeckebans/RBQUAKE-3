/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cmdlib.h"
#include "threads.h"
#include "mutex.h"

/*
===================================================================

WIN32

===================================================================
*/
#ifdef WIN32

	#define USED

	#include <windows.h>

void MutexLock( mutex_t* m )
{
	CRITICAL_SECTION* crit;

	if( !m )
	{
		return;
	}
	crit = ( CRITICAL_SECTION* )m;
	EnterCriticalSection( crit );
}

void MutexUnlock( mutex_t* m )
{
	CRITICAL_SECTION* crit;

	if( !m )
	{
		return;
	}
	crit = ( CRITICAL_SECTION* )m;
	LeaveCriticalSection( crit );
}

mutex_t* MutexAlloc()
{
	CRITICAL_SECTION* crit;

	if( numthreads == 1 )
	{
		return NULL;
	}
	crit = ( CRITICAL_SECTION* )safe_malloc( sizeof( CRITICAL_SECTION ) );
	InitializeCriticalSection( crit );
	return ( void* )crit;
}

#endif

/*
===================================================================

OSF1

===================================================================
*/

#ifdef __osf__
	#define USED

	#include <pthread.h>

void MutexLock( mutex_t* m )
{
	pthread_mutex_t* my_mutex;

	if( !m )
	{
		return;
	}
	my_mutex = ( pthread_mutex_t* )m;
	pthread_mutex_lock( my_mutex );
}

void MutexUnlock( mutex_t* m )
{
	pthread_mutex_t* my_mutex;

	if( !m )
	{
		return;
	}
	my_mutex = ( pthread_mutex_t* )m;
	pthread_mutex_unlock( my_mutex );
}

mutex_t* MutexAlloc()
{
	pthread_mutex_t*	my_mutex;
	pthread_mutexattr_t mattrib;

	if( numthreads == 1 )
	{
		return NULL;
	}
	my_mutex = safe_malloc( sizeof( *my_mutex ) );
	if( pthread_mutexattr_create( &mattrib ) == -1 )
	{
		Error( "pthread_mutex_attr_create failed" );
	}
	if( pthread_mutexattr_setkind_np( &mattrib, MUTEX_FAST_NP ) == -1 )
	{
		Error( "pthread_mutexattr_setkind_np failed" );
	}
	if( pthread_mutex_init( my_mutex, mattrib ) == -1 )
	{
		Error( "pthread_mutex_init failed" );
	}
	return ( void* )my_mutex;
}

#endif

/*
===================================================================

IRIX

===================================================================
*/

#ifdef _MIPS_ISA
	#define USED

	#include <task.h>
	#include <abi_mutex.h>
	#include <sys/types.h>
	#include <sys/prctl.h>

void MutexLock( mutex_t* m )
{
	abilock_t* lck;

	if( !m )
	{
		return;
	}
	lck = ( abilock_t* )m;
	spin_lock( lck );
}

void MutexUnlock( mutex_t* m )
{
	abilock_t* lck;

	if( !m )
	{
		return;
	}
	lck = ( abilock_t* )m;
	release_lock( lck );
}

mutex_t* MutexAlloc()
{
	abilock_t* lck;

	if( numthreads == 1 )
	{
		return NULL;
	}
	lck = ( abilock_t* )safe_malloc( sizeof( abilock_t ) );
	init_lock( lck );
	return ( void* )lck;
}

#endif

/*
=======================================================================

  SINGLE THREAD

=======================================================================
*/

#ifndef USED

void MutexLock( mutex_t* m )
{
}

void MutexUnlock( mutex_t* m )
{
}

mutex_t* MutexAlloc()
{
	return NULL;
}

#endif
