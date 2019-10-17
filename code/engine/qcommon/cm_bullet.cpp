/*
===========================================================================
Copyright (C) 2011 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// cm_bullet.cpp -- Bullet Physics SDK interface

extern "C"
{

#include "cm_local.h"
#include "cm_public.h"

}

#include "LinearMath/btVector3.h"
#include "LinearMath/btGeometryUtil.h"
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "btBulletDynamicsCommon.h"

/*
class BulletLocals
{
public:

	~BulletLocals()
	{
		// delete collision shapes
		for (int i = 0; i < collisionShapes.size(); i++)
		{
			btCollisionShape* shape = collisionShapes[i];
			delete shape;
		}
	}

	btAlignedObjectArray<btCollisionShape*>	collisionShapes;
};

static BulletLocals* cm_bulletLocals = NULL;
*/

void CM_InitBullet()
{
	//cm_bulletLocals = new BulletLocals();
}

void CM_ShutdownBullet()
{
	/*
	if(cm_bulletLocals)
	{
		delete cm_bulletLocals;
		cm_bulletLocals = NULL;
	}
	*/
}

#define USE_MOTIONSTATE 1

extern "C"
{
	void CM_AddWorldBrushesToDynamicsWorld( void* collisionShapesHandle, plDynamicsWorldHandle* dynamicsWorldHandle )
	{
		btAlignedObjectArray<btCollisionShape*>* collisionShapes = reinterpret_cast<btAlignedObjectArray<btCollisionShape*>*>( collisionShapesHandle );
		btDynamicsWorld* dynamicsWorld = reinterpret_cast< btDynamicsWorld* >( dynamicsWorldHandle );
		
		cm.checkcount++;
		
		for( int i = 0; i < cm.numLeafs; i++ )
		{
			const cLeaf_t* leaf = &cm.leafs[i];
			
			for( int j = 0; j < leaf->numLeafBrushes; j++ )
			{
				int brushnum = cm.leafbrushes[leaf->firstLeafBrush + j];
				
				cbrush_t* brush = &cm.brushes[brushnum];
				if( brush->checkcount == cm.checkcount )
				{
					// already checked this brush in another leaf
					continue;
				}
				brush->checkcount = cm.checkcount;
				
				if( brush->numsides == 0 )
				{
					// don't care about invalid brushes
					continue;
				}
				
				if( !( brush->contents & CONTENTS_SOLID ) )
				{
					// don't care about non-solid brushes
					continue;
				}
				
				btAlignedObjectArray<btVector3> planeEquations;
				
				for( int k = 0; k < brush->numsides; k++ )
				{
					const cbrushside_t* side = brush->sides + k;
					const cplane_t* plane = side->plane;
					
					btVector3 planeEq( plane->normal[0], plane->normal[1], plane->normal[2] );
					planeEq[3] = -plane->dist;
					
					planeEquations.push_back( planeEq );
				}
				
				btAlignedObjectArray<btVector3>	vertices;
				btGeometryUtil::getVerticesFromPlaneEquations( planeEquations, vertices );
				
				if( vertices.size() > 0 )
				{
					btCollisionShape* shape = new btConvexHullShape( &( vertices[0].getX() ), vertices.size() );
					collisionShapes->push_back( shape );
					
					float mass = 0.f;
					btTransform startTransform;
					
					startTransform.setIdentity();
					//startTransform.setOrigin(btVector3(0,0,-10.f));
					
					
#ifdef USE_MOTIONSTATE
					btDefaultMotionState* motionState = new btDefaultMotionState( startTransform );
					
					btVector3 localInertia( 0, 0, 0 );
					btRigidBody::btRigidBodyConstructionInfo cInfo( mass, motionState, shape, localInertia );
					
					btRigidBody* body = new btRigidBody( cInfo );
					
					// FIXME check this
					body->setContactProcessingThreshold( BT_LARGE_FLOAT );
					
#else
					btRigidBody* body = new btRigidBody( mass, 0, shape, localInertia );
					body->setWorldTransform( startTransform );
#endif//
					
					dynamicsWorld->addRigidBody( body );
				}
				
			}
		}
	}
	
} // extern "C"