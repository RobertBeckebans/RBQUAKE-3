/*
===========================================================================
Copyright (C) 1998 Steve Yeager
Copyright (C) 2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
//
//  acebot_nodes.c -   This file contains all of the
//                     pathing routines for the ACE bot.


#include "../g_local.h"
#include "acebot.h"

#if defined(ACEBOT)



// Total number of nodes
int             numNodes;

// array for node data
node_t          nodes[MAX_NODES];
short int       path_table[MAX_NODES][MAX_NODES];






// Determin cost of moving from one node to another
int ACEND_FindCost( int from, int to )
{
	int             curnode;
	int             cost = 1;	// Shortest possible is 1
	
	if( from == INVALID || to == INVALID )
	{
		return INVALID;
	}
	
	// If we can not get there then return invalid
	if( path_table[from][to] == INVALID )
	{
		return INVALID;
	}
	
	// Otherwise check the path and return the cost
	curnode = path_table[from][to];
	
	// Find a path (linear time, very fast)
	while( curnode != to )
	{
		curnode = path_table[curnode][to];
		if( curnode == INVALID )	// something has corrupted the path abort
		{
			return INVALID;
		}
		cost++;
	}
	
	return cost;
}

// Find a close node to the player within dist.
//
// Faster than looking for the closest node, but not very
// accurate.
int ACEND_FindCloseReachableNode( gentity_t* self, float range, int type )
{
	vec3_t          v;
	int             i;
	trace_t         tr;
	float           dist;
	
//  range *= range;

	for( i = 0; i < numNodes; i++ )
	{
		if( type == NODE_ALL || type == nodes[i].type )	// check node type
		{
		
			VectorSubtract( nodes[i].origin, self->client->ps.origin, v );	// subtract first
			
			dist = VectorLength( v );
			
			if( dist < range )	// square range instead of sqrt
			{
				// make sure it is visible
				trap_Trace( &tr, self->client->ps.origin, self->r.mins, self->r.maxs, nodes[i].origin, self->s.number,
							MASK_PLAYERSOLID );
							
				if( tr.fraction == 1.0 )
				{
					return i;
				}
			}
		}
	}
	
	return -1;
}



// Find the closest node to the player within a certain range
int ACEND_FindClosestReachableNode( gentity_t* self, float range, int type )
{
	int             i;
	float           closest = 99999;
	float           dist;
	int             node = INVALID;
	vec3_t          v;
	trace_t         tr;
	//float           rng;
	vec3_t          maxs, mins;
	
	VectorCopy( self->r.mins, mins );
	VectorCopy( self->r.maxs, maxs );
	
	mins[2] += STEPSIZE;
	
//  rng = (float)(range * range);   // square range for distance comparison (eliminate sqrt)

	for( i = 0; i < numNodes; i++ )
	{
		if( type == NODE_ALL || type == nodes[i].type )	// check node type
		{
			VectorSubtract( nodes[i].origin, self->client->ps.origin, v );	// subtract first
			
			dist = VectorLength( v );
			
			if( dist < closest && dist < range )
			{
				// make sure it is visible
				trap_Trace( &tr, self->client->ps.origin, mins, maxs, nodes[i].origin, self->s.number, MASK_PLAYERSOLID );
				
				if( tr.fraction == 1.0 )
				{
					node = i;
					closest = dist;
				}
			}
		}
	}
	
	return node;
}



void ACEND_SetGoal( gentity_t* self, int goalNode )
{
	int             node;
	
	self->bs.goalNode = goalNode;
	
	node = ACEND_FindClosestReachableNode( self, NODE_DENSITY * 3, NODE_ALL );
	
	if( node == INVALID )
	{
		return;
	}
	
	if( ace_debug.integer )
	{
		trap_SendServerCommand( -1, va( "print \"%s: new start node selected %d\n\"", self->client->pers.netname, node ) );
	}
	
	self->bs.currentNode = node;
	self->bs.nextNode = self->bs.currentNode;	// make sure we get to the nearest node first
	self->bs.node_timeout = 0;
	
	if( ace_showPath.integer )
	{
		// draw path to LR goal
		ACEND_DrawPath( self->bs.currentNode, self->bs.goalNode );
	}
}

// Move closer to goal by pointing the bot to the next node
// that is closer to the goal
qboolean ACEND_FollowPath( gentity_t* self )
{
	// try again?
	if( self->bs.node_timeout++ > 30 )
	{
		if( self->bs.tries++ > 3 )
		{
			return qfalse;
		}
		else
		{
			ACEND_SetGoal( self, self->bs.goalNode );
		}
	}
	
	// are we there yet?
	
	//if(Distance(self->client->ps.origin, nodes[self->bs.nextNode].origin) < 32)
	if( BoundsIntersectPoint( self->r.absmin, self->r.absmax, nodes[self->bs.nextNode].origin ) )
	{
		// reset timeout
		self->bs.node_timeout = 0;
		
		if( self->bs.nextNode == self->bs.goalNode )
		{
			if( ace_debug.integer )
			{
				trap_SendServerCommand( -1, va( "print \"%s: reached goal!\n\"", self->client->pers.netname ) );
			}
			
			ACEAI_PickLongRangeGoal( self );	// pick a new goal
		}
		else
		{
			self->bs.currentNode = self->bs.nextNode;
			self->bs.nextNode = path_table[self->bs.currentNode][self->bs.goalNode];
		}
	}
	
	if( self->bs.currentNode == INVALID || self->bs.nextNode == INVALID )
	{
		return qfalse;
	}
	
	// set bot's movement vector
	VectorSubtract( nodes[self->bs.nextNode].origin, self->client->ps.origin, self->bs.moveVector );
	
	return qtrue;
}



// Capture when the grappling hook has been fired for mapping purposes.
/*
void ACEND_GrapFired(gentity_t * self)
{
	int             closestNode;
	gentity_t      *owner;

	if(self->r.ownerNum == ENTITYNUM_NONE)
		return;

	owner = &g_entities[self->r.ownerNum];

	// should not be here

	// Check to see if the grapple is in pull mode
	if(self->owner->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL)
	{
		// Look for the closest node of type grapple
		closestNode = ACEND_FindClosestReachableNode(self, NODE_DENSITY, NODE_GRAPPLE);
		if(closestNode == -1)	// we need to drop a node
		{
			closestNode = ACEND_AddNode(self, NODE_GRAPPLE);

			// Add an edge
			ACEND_UpdateNodeEdge(self->owner->lastNode, closestNode);

			self->owner->lastNode = closestNode;
		}
		else
			self->owner->lastNode = closestNode;	// zero out so other nodes will not be linked
	}
}
*/



// This routine is called to hook in the pathing code and sets
// the current node if valid.
void ACEND_PathMap( gentity_t* self )
{
	int             closestNode;
	static float    lastUpdate = 0;	// start off low
	vec3_t          v;
	//qboolean        isJumping;
	//int             i;
	
#if 0
	if( level.time < lastUpdate )
	{
		return;
	}
#endif
	
	lastUpdate = level.time + 150;	// slow down updates a bit
	
#if 0
	if( self->r.svFlags & SVF_BOT )
	{
		return;
	}
#endif
	
	// don't add links when you went into a trap
	if( self->health <= 0 )
	{
		return;
	}
	
#if 1
	if( self->s.groundEntityNum == ENTITYNUM_NONE && !( self->r.svFlags & SVF_BOT ) )
	{
#if 0
		isJumping = qfalse;
		for( i = 0; i < self->client->ps.eventSequence; i++ )
		{
			if( self->client->ps.events[i] == EV_JUMP )
			{
				isJumping = qtrue;
			}
		}
		
		if( isJumping )
#else
		if( ( self->client->ps.pm_flags & PMF_JUMP_HELD ) )
#endif
		{
			if( ace_debug.integer )
			{
				trap_SendServerCommand( -1, va( "print \"%s: jumping\n\"", self->client->pers.netname ) );
			}
			
			// see if there is a closeby jump landing node (prevent adding too many)
			closestNode = ACEND_FindClosestReachableNode( self, 64, NODE_JUMP );
			
			if( closestNode == INVALID )
			{
				closestNode = ACEND_AddNode( self, NODE_JUMP );
			}
			
			// now add link
			if( self->bs.lastNode != INVALID )
			{
				ACEND_UpdateNodeEdge( self->bs.lastNode, closestNode );
			}
			
			self->bs.isJumping = qfalse;
			return;
		}
	}
#endif
	
	// not on ground, and not in the water, so bail
	if( self->s.groundEntityNum == ENTITYNUM_NONE )
	{
		/*
		   if(self->bs.lastNode != INVALID)
		   {
		   // we might have been pushed by a jump pad
		   if(nodes[self->bs.lastNode].type != NODE_JUMPPAD)
		   return;
		   }
		   else if(!self->waterlevel)
		   {
		   return;
		   }
		 */
	}
	
	
	// lava / slime
	VectorCopy( self->client->ps.origin, v );
	v[2] -= 18;
	
	if( trap_PointContents( self->client->ps.origin, -1 ) & ( CONTENTS_LAVA | CONTENTS_SLIME ) )
	{
		return;    // no nodes in slime
	}
	
	// Grapple
	// Do not add nodes during grapple, added elsewhere manually
	
	/*
	   if(ctf->value && self->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL)
	   return;
	 */
	
	// iterate through all nodes to make sure far enough apart
	closestNode = ACEND_FindClosestReachableNode( self, NODE_DENSITY, NODE_ALL );
	
	
	// Special Check for Platforms
	/* FIXME
	   if(self->groundentity && self->groundentity->use == Use_Plat)
	   {
	   if(closestNode == INVALID)
	   return;              // Do not want to do anything here.
	
	   // Here we want to add links
	   if(closestNode != self->lastNode && self->lastNode != INVALID)
	   ACEND_UpdateNodeEdge(self->lastNode, closestNode);
	
	   self->lastNode = closestNode;    // set visited to last
	   return;
	   }
	 */
	
	
	if( closestNode != INVALID )
	{
		// add automatically some links between nodes
		
		if( closestNode != self->bs.lastNode && self->bs.lastNode != INVALID )
		{
			ACEND_UpdateNodeEdge( self->bs.lastNode, closestNode );
			if( ace_showLinks.integer )
			{
				ACEND_DrawPath( self->bs.lastNode, closestNode );
			}
		}
		
		self->bs.lastNode = closestNode;	// set visited to last
	}
#if 1
	else if( closestNode == INVALID && self->s.groundEntityNum != ENTITYNUM_NONE )
	{
		// add nodes in the water as needed
		if( self->waterlevel )
		{
			closestNode = ACEND_AddNode( self, NODE_WATER );
		}
		else
		{
			closestNode = ACEND_AddNode( self, NODE_MOVE );
		}
		
		// now add link
		if( self->bs.lastNode != INVALID )
		{
			ACEND_UpdateNodeEdge( self->bs.lastNode, closestNode );
			
			if( ace_showLinks.integer )
			{
				ACEND_DrawPath( self->bs.lastNode, closestNode );
			}
		}
		
		self->bs.lastNode = closestNode;	// set visited to last
	}
#endif
}


void ACEND_InitNodes( void )
{
	// init node array (set all to INVALID)
	numNodes = 0;
	memset( nodes, 0, sizeof( node_t ) * MAX_NODES );
	memset( path_table, INVALID, sizeof( short int ) * MAX_NODES * MAX_NODES );
	
}

// show the node for debugging (utility function)
void ACEND_ShowNode( int node )
{
	gentity_t*      ent;
	
	if( !ace_showNodes.integer )
	{
		return;
	}
	
	ent = G_Spawn();
	
	ent->s.eType = ET_AI_NODE;
	ent->r.svFlags |= SVF_BROADCAST;
	
	ent->classname = "ACEND_node";
	
	VectorCopy( nodes[node].origin, ent->s.origin );
	VectorCopy( nodes[node].origin, ent->s.pos.trBase );
	
	// otherEntityNum is transmitted with GENTITYNUM_BITS so enough for 1000 nodes
	ent->s.otherEntityNum = node;
	
	//ent->nextthink = level.time + 200000;
	//ent->think = G_FreeEntity;
	
	trap_LinkEntity( ent );
}

// draws the current path (utility function)
void ACEND_DrawPath( int currentNode, int goalNode )
{
	int             nextNode;
	
	if( currentNode == INVALID || goalNode == INVALID )
	{
		return;
	}
	
	nextNode = path_table[currentNode][goalNode];
	
	// Now set up and display the path
	while( currentNode != goalNode && currentNode != -1 )
	{
		gentity_t*      ent;
		
		ent = G_Spawn();
		
		ent->s.eType = ET_AI_LINK;
		ent->r.svFlags |= SVF_BROADCAST;
		
		ent->classname = "ACEND_link";
		
		VectorCopy( nodes[currentNode].origin, ent->s.origin );
		VectorCopy( nodes[currentNode].origin, ent->s.pos.trBase );
		VectorCopy( nodes[nextNode].origin, ent->s.origin2 );
		
		ent->nextthink = level.time + 3000;
		ent->think = G_FreeEntity;
		
		trap_LinkEntity( ent );
		
		
		currentNode = nextNode;
		nextNode = path_table[currentNode][goalNode];
	}
}

// Turns on showing of the path, set goal to -1 to
// shut off. (utility function)
void ACEND_ShowPath( gentity_t* self, int goalNode )
{
	int             currentNode;
	
	currentNode = ACEND_FindClosestReachableNode( self, NODE_DENSITY, NODE_ALL );
	if( currentNode == INVALID )
	{
		trap_SendServerCommand( self - g_entities, "print \"no closest reachable node!\n\"" );
		return;
	}
	
	ACEND_DrawPath( currentNode, goalNode );
}


int ACEND_AddNode( gentity_t* self, int type )
{
	vec3_t          v, v2;
	const char*     entityName;
	
	// block if we exceed maximum
	if( numNodes >= MAX_NODES )
	{
		return INVALID;
	}
	
#if 0
	// it's better when bots do not create any path nodes ..
	if( self->r.svFlags & SVF_BOT )
	{
		return INVALID;
	}
#endif
	
	if( self->name )
	{
		entityName = self->name;
	}
	else
	{
		entityName = self->classname;
	}
	
	// set location
	if( self->client )
	{
		VectorCopy( self->client->ps.origin, nodes[numNodes].origin );
	}
	else
	{
		VectorCopy( self->s.origin, nodes[numNodes].origin );
	}
	
	// set type
	nodes[numNodes].type = type;
	
	// move the z location up just a bit for items
	if( type == NODE_ITEM )
	{
		//nodes[numNodes].origin[2] += 16;
	}
	// teleporters
	/*
	   else if(type == NODE_TARGET_TELEPORT)
	   {
	   nodes[numNodes].origin[2] += 32;
	   }
	 */
	else if( type == NODE_TRIGGER_TELEPORT )
	{
		VectorAdd( self->r.absmin, self->r.absmax, v );
		VectorScale( v, 0.5, v );
		
		VectorCopy( v, nodes[numNodes].origin );
	}
	else if( type == NODE_JUMPPAD )
	{
		VectorAdd( self->r.absmin, self->r.absmax, v );
		VectorScale( v, 0.5, v );
		
		// add jumppad target offset
		VectorNormalize2( self->s.origin2, v2 );
		VectorMA( v, 32, v2, v );
		
		VectorCopy( v, nodes[numNodes].origin );
	}
	
	/*
	   // for platforms drop two nodes one at top, one at bottom
	   if(type == NODE_PLATFORM)
	   {
	   VectorCopy(self->r.maxs, v1);
	   VectorCopy(self->r.mins, v2);
	
	   // to get the center
	   nodes[numNodes].origin[0] = (v1[0] - v2[0]) / 2 + v2[0];
	   nodes[numNodes].origin[1] = (v1[1] - v2[1]) / 2 + v2[1];
	   nodes[numNodes].origin[2] = self->r.maxs[2];
	
	   if(ace_debug.integer)
	   ACEND_ShowNode(numNodes);
	
	   numNodes++;
	
	   nodes[numNodes].origin[0] = nodes[numNodes - 1].origin[0];
	   nodes[numNodes].origin[1] = nodes[numNodes - 1].origin[1];
	   nodes[numNodes].origin[2] = self->r.mins[2] + 64;
	
	   nodes[numNodes].type = NODE_PLATFORM;
	
	   // add a link
	   ACEND_UpdateNodeEdge(numNodes, numNodes - 1);
	
	   if(ace_debug.integer)
	   {
	   debug_printf("Node %d added for entity %s type: Platform\n", numNodes, entityName);
	   ACEND_ShowNode(numNodes);
	   }
	
	   numNodes++;
	
	   return numNodes - 1;
	   }
	 */
	
	SnapVector( nodes[numNodes].origin );
	
	if( ace_debug.integer )
	{
		G_Printf( "node %d added for entity %s type: %s pos: %f %f %f\n", numNodes, entityName,
				  ACEND_NodeTypeToString( nodes[numNodes].type ), nodes[numNodes].origin[0], nodes[numNodes].origin[1],
				  nodes[numNodes].origin[2] );
				  
		ACEND_ShowNode( numNodes );
	}
	
	// add a link
	//ACEND_UpdateNodeEdge(numNodes, numNodes - 1);
	
	numNodes++;
	return numNodes - 1;		// return the node added
}

const char*     ACEND_NodeTypeToString( int type )
{
	if( type == NODE_MOVE )
	{
		return "move";
	}
	else if( type == NODE_PLATFORM )
	{
		return "platform";
	}
	else if( type == NODE_TRIGGER_TELEPORT )
	{
		return "trigger_teleport";
	}
	/*
	   else if(type == NODE_TARGET_TELEPORT)
	   {
	   return "target_teleport";
	   }
	 */
	else if( type == NODE_ITEM )
	{
		return "item";
	}
	else if( type == NODE_WATER )
	{
		return "water";
	}
	else if( type == NODE_JUMP )
	{
		return "jump";
	}
	else if( type == NODE_JUMPPAD )
	{
		return "jumppad";
	}
	
	return "BAD";
}


// add / update node connections (paths)
void ACEND_UpdateNodeEdge( int from, int to )
{
	int             i;
	
	if( from == -1 || to == -1 || from == to )
	{
		return;    // safety
	}
	
	// Add the link
	path_table[from][to] = to;
	
	// Now for the self-referencing part, linear time for each link added
	for( i = 0; i < numNodes; i++ )
	{
		if( path_table[i][from] != INVALID )
		{
			if( i == to )
			{
				path_table[i][to] = INVALID;    // make sure we terminate
			}
			else
			{
				path_table[i][to] = path_table[i][from];
			}
		}
	}
	
	if( ace_showLinks.integer )
	{
		trap_SendServerCommand( -1, va( "print \"Link %d -> %d\n\"", from, to ) );
	}
}

// remove a node edge
void ACEND_RemoveNodeEdge( gentity_t* self, int from, int to )
{
	int             i;
	
	if( ace_showLinks.integer )
	{
		trap_SendServerCommand( -1, va( "print \"%s: removing link %d -> %d\n\"", self->client->pers.netname, from, to ) );
	}
	
	path_table[from][to] = INVALID;	// set to invalid
	
	// Make sure this gets updated in our path array
	for( i = 0; i < numNodes; i++ )
	{
		if( path_table[from][i] == to )
		{
			path_table[from][i] = INVALID;
		}
	}
}

// This function will resolve all paths that are incomplete
// usually called before saving to disk
void ACEND_ResolveAllPaths()
{
	int             i, from, to;
	int             num = 0;
	
	G_Printf( "Resolving all paths..." );
	
	for( from = 0; from < numNodes; from++ )
	{
		for( to = 0; to < numNodes; to++ )
		{
			// update unresolved paths
			// Not equal to itself, not equal to -1 and equal to the last link
			if( from != to && path_table[from][to] == to )
			{
				num++;
				
				// Now for the self-referencing part linear time for each link added
				for( i = 0; i < numNodes; i++ )
				{
					if( path_table[i][from] != -1 )
					{
						if( i == to )
						{
							path_table[i][to] = -1;    // make sure we terminate
						}
						else
						{
							path_table[i][to] = path_table[i][from];
						}
					}
				}
			}
		}
	}
	
	G_Printf( "done (%d updated)\n", num );
}

// Save to disk file
//
// Since my compression routines are one thing I did not want to
// release, I took out the compressed format option. Most levels will
// save out to a node file around 50-200k, so compression is not really
// a big deal.
void ACEND_SaveNodes()
{
	fileHandle_t    file;
	char            filename[MAX_QPATH];
	int             i, j;
	int             version = 1;
	char            mapname[MAX_QPATH];
	
	ACEND_ResolveAllPaths();
	
	trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	Com_sprintf( filename, sizeof( filename ), "nav/%s.nod", mapname );
	
	trap_FS_FOpenFile( filename, &file, FS_WRITE );
	if( !file )
	{
		G_Printf( "WARNING: Couldn't write node table: %s\n", filename );
		return;
	}
	else
	{
		G_Printf( "ACE: Saving node table '%s'...", filename );
	}
	
	trap_FS_Write( &version, sizeof( int ), file );
	trap_FS_Write( &numNodes, sizeof( int ), file );
	trap_FS_Write( nodes, sizeof( node_t ) * numNodes, file );
	
	for( i = 0; i < numNodes; i++ )
		for( j = 0; j < numNodes; j++ )
		{
			trap_FS_Write( &path_table[i][j], sizeof( short int ), file );    // write count
		}
		
	trap_FS_FCloseFile( file );
	
	G_Printf( "done.\n" );
	
	G_Printf( "%i nodes saved\n", numNodes );
}

// Read from disk file
void ACEND_LoadNodes( void )
{
	fileHandle_t    file;
	int             i, j;
	char            filename[MAX_QPATH];
	int             version;
	char            mapname[MAX_QPATH];
	
	trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
	Com_sprintf( filename, sizeof( filename ), "nav/%s.nod", mapname );
	
	trap_FS_FOpenFile( filename, &file, FS_READ );
	if( !file )
	{
		// Create item table
		G_Printf( "ACE: No node file '%s' found\n", filename );
		//ACEIT_BuildItemNodeTable(qfalse);
		//G_Printf("done.\n");
		return;
	}
	
	// determin version
	trap_FS_Read( &version, sizeof( int ), file );	// read version
	
	if( version == 1 )
	{
		G_Printf( "ACE: Loading node table '%s'...\n", filename );
		
		trap_FS_Read( &numNodes, sizeof( int ), file );	// read count
		trap_FS_Read( &nodes, sizeof( node_t ) * numNodes, file );
		
		for( i = 0; i < numNodes; i++ )
			for( j = 0; j < numNodes; j++ )
			{
				trap_FS_Read( &path_table[i][j], sizeof( short int ), file );    // write count
			}
			
		if( ace_showNodes.integer )
		{
			for( i = 0; i < numNodes; i++ )
			{
				ACEND_ShowNode( i );
			}
		}
	}
	else
	{
		// Create item table
		G_Printf( "ACE: '%s' has wrong version %i\n", filename, version );
		//ACEIT_BuildItemNodeTable(qfalse);
		//G_Printf("done.\n");
	}
	
	trap_FS_FCloseFile( file );
	
	G_Printf( "done.\n" );
	G_Printf( "%i nodes loaded\n", numNodes );
	
	ACEIT_BuildItemNodeTable( qtrue );
}

#endif
