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

// vm.c -- virtual machine
#include "q_shared.h"
#include "qcommon.h"

/*
a dll has one imported function: VM_SystemCall
and one exported function: VM_Main
*/

vm_t*           currentVM = NULL;	// bk001212
vm_t*           lastVM = NULL;	// bk001212

#define	MAX_VM		3
vm_t            vmTable[MAX_VM];



/*
==============
VM_VmInfo_f
==============
*/
static void VM_VmInfo_f( void )
{
	vm_t*           vm;
	int             i;
	
	Com_Printf( "Registered virtual machines:\n" );
	for( i = 0; i < MAX_VM; i++ )
	{
		vm = &vmTable[i];
		if( !vm->name[0] )
		{
			break;
		}
		Com_Printf( "%s : ", vm->name );
		if( vm->dllHandle )
		{
			Com_Printf( "native\n" );
			continue;
		}
		
#if USE_LLVM
		if( vm->llvmModuleProvider )
		{
			Com_Printf( "llvm\n" );
			continue;
		}
#endif
	}
}

/*
==============
VM_Init
==============
*/
void VM_Init( void )
{
	Cvar_Get( "vm_cgame", "1", CVAR_ARCHIVE );
	Cvar_Get( "vm_game", "1", CVAR_ARCHIVE );
	Cvar_Get( "vm_ui", "1", CVAR_ARCHIVE );
	
//	Cmd_AddCommand("vmprofile", VM_VmProfile_f);
	Cmd_AddCommand( "vminfo", VM_VmInfo_f );
	
	Com_Memset( vmTable, 0, sizeof( vmTable ) );
}

/*
=================
VM_Restart

Reload the data, but leave everything else in place
This allows a server to do a map_restart without changing memory allocation
=================
*/
vm_t*           VM_Restart( vm_t* vm )
{
	// DLL's can't be restarted in place
	char            name[MAX_QPATH];
	vmInterpret_t	interpret;
	
	intptr_t( *systemCall )( intptr_t* parms );
	
	systemCall = vm->systemCall;
	Q_strncpyz( name, vm->name, sizeof( name ) );
	interpret = vm->interpret;
	
	VM_Free( vm );
	
	vm = VM_Create( name, systemCall, interpret );
	return vm;
}


void VM_Forced_Unload_Start( void )
{
}

void VM_Forced_Unload_Done( void )
{
}

/*
==============
VM_Free
==============
*/
void VM_Free( vm_t* vm )
{
	if( vm->dllHandle )
	{
		Sys_UnloadDll( vm->dllHandle );
		Com_Memset( vm, 0, sizeof( *vm ) );
	}
	
#if USE_LLVM
	if( vm->llvmModuleProvider )
	{
		VM_UnloadLLVM( vm->llvmModuleProvider );
		Com_Memset( vm, 0, sizeof( *vm ) );
	}
#endif
	
	Com_Memset( vm, 0, sizeof( *vm ) );
	
	currentVM = NULL;
	lastVM = NULL;
}

void VM_Clear( void )
{
	int             i;
	
	for( i = 0; i < MAX_VM; i++ )
	{
		if( vmTable[i].dllHandle )
		{
			Sys_UnloadDll( vmTable[i].dllHandle );
		}
		
		Com_Memset( &vmTable[i], 0, sizeof( vm_t ) );
	}
	currentVM = NULL;
	lastVM = NULL;
}

void*           VM_ArgPtr( intptr_t intValue )
{
	if( !intValue )
	{
		return NULL;
	}
	
	// bk001220 - currentVM is missing on reconnect
	if( currentVM == NULL )
	{
		return NULL;
	}
	
	return ( void* )( currentVM->dataBase + intValue );
}

void*           VM_ExplicitArgPtr( vm_t* vm, intptr_t intValue )
{
	if( !intValue )
	{
		return NULL;
	}
	
	// bk010124 - currentVM is missing on reconnect here as well?
	if( currentVM == NULL )
	{
		return NULL;
	}
	
	return ( void* )( vm->dataBase + intValue );
}

/*
============
VM_DllSyscall

Dlls will call this directly

 rcg010206 The horror; the horror.

  The syscall mechanism relies on stack manipulation to get it's args.
   This is likely due to C's inability to pass "..." parameters to
   a function in one clean chunk. On PowerPC Linux, these parameters
   are not necessarily passed on the stack, so while (&arg[0] == arg)
   is true, (&arg[1] == 2nd function parameter) is not necessarily
   accurate, as arg's value might have been stored to the stack or
   other piece of scratch memory to give it a valid address, but the
   next parameter might still be sitting in a register.

  Quake's syscall system also assumes that the stack grows downward,
   and that any needed types can be squeezed, safely, into a signed int.

  This hack below copies all needed values for an argument to a
   array in memory, so that Quake can get the correct values. This can
   also be used on systems where the stack grows upwards, as the
   presumably standard and safe stdargs.h macros are used.

  As for having enough space in a signed int for your datatypes, well,
   it might be better to wait for DOOM 3 before you start porting.  :)

  The original code, while probably still inherently dangerous, seems
   to work well enough for the platforms it already works on. Rather
   than add the performance hit for those platforms, the original code
   is still in use there.

  For speed, we just grab 15 arguments, and don't worry about exactly
   how many the syscall actually needs; the extra is thrown away.
============
*/
intptr_t QDECL VM_DllSyscall( intptr_t arg, ... )
{
#if !id386
	// rcg010206 - see commentary above
	intptr_t        args[16];
	int             i;
	va_list         ap;
	
	args[0] = arg;
	
	va_start( ap, arg );
	for( i = 1; i < sizeof( args ) / sizeof( args[i] ); i++ )
	{
		args[i] = va_arg( ap, intptr_t );
	}
	va_end( ap );
	
	return currentVM->systemCall( args );
#else
	// original id code
	return currentVM->systemCall( &arg );
#endif
}

/*
==============
VM_Call

rcg010207 -  see dissertation at top of VM_DllSyscall() in this file.
==============
*/
intptr_t QDECL VM_Call( vm_t* vm, int callnum, ... )
{
	vm_t*           oldVM;
	intptr_t        r;
	int             i;
	int             args[10];
	va_list         ap;
	
	if( !vm )
	{
		Com_Error( ERR_FATAL, "VM_Call with NULL vm" );
	}
	
	oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;
	
	va_start( ap, callnum );
	for( i = 0; i < sizeof( args ) / sizeof( args[i] ); i++ )
	{
		args[i] = va_arg( ap, int );
	}
	va_end( ap );
	
	r = vm->entryPoint( callnum, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9] );
	
	if( oldVM != NULL )			// bk001220 - assert(currentVM!=NULL) for oldVM==NULL
	{
		currentVM = oldVM;
	}
	
	return r;
}

/*
================
VM_Create
================
*/
vm_t*           VM_Create( const char* module, intptr_t( *systemCalls )( intptr_t* ), vmInterpret_t interpret )
{
	vm_t*           vm;
	int             i, remaining, retval;
	char filename[MAX_OSPATH];
	void* startSearch = NULL;
	
	if( !module || !module[0] || !systemCalls )
	{
		Com_Error( ERR_FATAL, "VM_Create: bad parms" );
	}
	
	remaining = Hunk_MemoryRemaining();
	
	// see if we already have the VM
	for( i = 0; i < MAX_VM; i++ )
	{
		if( !Q_stricmp( vmTable[i].name, module ) )
		{
			vm = &vmTable[i];
			return vm;
		}
	}
	
	// find a free vm
	for( i = 0; i < MAX_VM; i++ )
	{
		if( !vmTable[i].name[0] )
		{
			break;
		}
	}
	
	if( i == MAX_VM )
	{
		Com_Error( ERR_FATAL, "VM_Create: no free vm_t" );
	}
	
	vm = &vmTable[i];
	
	Q_strncpyz( vm->name, module, sizeof( vm->name ) );
	
	do
	{
		retval = FS_FindVM( &startSearch, filename, sizeof( filename ), module, ( interpret == VMI_NATIVE ) );
		
		if( retval == VMI_NATIVE )
		{
			Com_DPrintf( "Try loading dll file %s\n", filename );
			
			vm->dllHandle = Sys_LoadGameDll( filename, &vm->entryPoint, VM_DllSyscall );
			
			if( vm->dllHandle )
			{
				vm->systemCall = systemCalls;
				vm->interpret = VMI_NATIVE;
				return vm;
			}
			
			Com_DPrintf( "Failed loading dll, trying next\n" );
		}
#ifdef USE_QVM
		else if( retval == VMI_COMPILED )
		{
			vm->searchPath = startSearch;
			if( ( header = VM_LoadQVM( vm, qtrue, qfalse ) ) )
			{
				break;
			}
			
			// VM_Free overwrites the name on failed load
			Q_strncpyz( vm->name, module, sizeof( vm->name ) );
		}
#endif
	}
	while( retval >= 0 );
	
	if( retval < 0 )
	{
		return NULL;
	}
	
	vm->systemCall = systemCalls;
	
	return NULL;
}


