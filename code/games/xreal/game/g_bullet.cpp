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

/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/


extern "C" 
{
#include "g_local.h"
}

#include "LinearMath/btVector3.h"
#include "LinearMath/btGeometryUtil.h"
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "btBulletDynamicsCommon.h"


#define USE_MOTIONSTATE 1

class BulletPhysics
{
public:

private:
	btAlignedObjectArray<btCollisionShape*>	_collisionShapes;

	btDynamicsWorld*				_dynamicsWorld;
	btCollisionConfiguration*		_collisionConfiguration;
	btCollisionDispatcher*			_dispatcher;
	btBroadphaseInterface*			_broadphase;
	btConstraintSolver*				_solver;

public:
	BulletPhysics()
	{
		_collisionConfiguration = new btDefaultCollisionConfiguration();

		// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		_dispatcher = new btCollisionDispatcher(_collisionConfiguration);
		_dispatcher->registerCollisionCreateFunc(BOX_SHAPE_PROXYTYPE, BOX_SHAPE_PROXYTYPE, _collisionConfiguration->getCollisionAlgorithmCreateFunc(CONVEX_SHAPE_PROXYTYPE, CONVEX_SHAPE_PROXYTYPE));

		_broadphase = new btDbvtBroadphase();

		// the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
		_solver = sol;

		_dynamicsWorld = new btDiscreteDynamicsWorld(_dispatcher, _broadphase, _solver, _collisionConfiguration);
		//_dynamicsWorld ->setDebugDrawer(&sDebugDrawer);
		_dynamicsWorld->getSolverInfo().m_splitImpulse = true;
		_dynamicsWorld->getSolverInfo().m_numIterations = 20;

		if (g_physUseCCD.integer)
		{
			_dynamicsWorld->getDispatchInfo().m_useContinuous = true;
		}
		else
		{
			_dynamicsWorld->getDispatchInfo().m_useContinuous = false;
		}

		trap_Bullet_AddWorldBrushesToDynamicsWorld(&_collisionShapes, (plDynamicsWorldHandle*) _dynamicsWorld);
	}

	~BulletPhysics()
	{
		// remove the rigidbodies from the dynamics world and delete them
		for (int i = _dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = _dynamicsWorld->getCollisionObjectArray()[i];
			
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				delete body->getMotionState();
			}
			_dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}

		// delete collision shapes
		for (int i = 0; i < _collisionShapes.size(); i++)
		{
			btCollisionShape* shape = _collisionShapes[i];
			delete shape;
		}

		delete _dynamicsWorld;
		delete _solver;
		delete _broadphase;
		delete _dispatcher;
		delete _collisionConfiguration;
	}

	btDynamicsWorld*		GetDynamicsWorld()
	{
		return _dynamicsWorld;
	}

	btRigidBody* CreateRigidBody(float mass, const btTransform& startTransform, btCollisionShape* shape)
	{
		btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

		//rigidbody is dynamic if and only if mass is non zero, otherwise static
		bool isDynamic = (mass != 0.f);

		btVector3 localInertia(0,0,0);
		if (isDynamic)
			shape->calculateLocalInertia(mass,localInertia);

		//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
#ifdef USE_MOTIONSTATE
		btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);

		btRigidBody::btRigidBodyConstructionInfo cInfo(mass, motionState, shape, localInertia);

		btRigidBody* body = new btRigidBody(cInfo);
		body->setContactProcessingThreshold(BT_LARGE_FLOAT);

#else
		btRigidBody* body = new btRigidBody(mass, 0, shape, localInertia);
		body->setWorldTransform(startTransform);
#endif//

		_dynamicsWorld->addRigidBody(body);

		return body;
	}

	void RunPhysics(int deltaTime)
	{
		_dynamicsWorld->setGravity(btVector3(g_gravityX.value, g_gravityY.value, g_gravityZ.value));

		//_dynamicsWorld->stepSimulation(1./60.)
		_dynamicsWorld->stepSimulation(deltaTime * 0.001f, 10);

		for (int i = _dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
		{
			btCollisionObject* obj = _dynamicsWorld->getCollisionObjectArray()[i];
			
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState())
			{
				gentity_t* ent = (gentity_t*) body->getUserPointer();
				if(ent != NULL && ent->physics != NULL)
				{
					ent->physics(ent);
				}
			}
		}
	}
};

static BulletPhysics* g_bulletPhysics = NULL;


extern "C"
{

void G_InitBulletPhysics()
{
	G_Printf("------- Bullet Physics Initialization -------\n");

	g_bulletPhysics = new BulletPhysics();
}

void G_ShutdownBulletPhysics()
{
	G_Printf("------- Bullet Physics Shutdown -------\n");

	if(g_bulletPhysics)
	{
		delete g_bulletPhysics;
		g_bulletPhysics = NULL;
	}
}

void G_RunPhysics(int deltaTime)
{
	g_bulletPhysics->RunPhysics(deltaTime);
}




static void G_PhysicsBox_Physics(gentity_t * ent)
{
	btCollisionObject* obj = (btCollisionObject*) ent->physicsRigidBody;
	btRigidBody* body = btRigidBody::upcast(obj);

	if (body && body->getMotionState())
	{
		// set entityState_t::pos
		btTransform trans;
		body->getMotionState()->getWorldTransform(trans);

		vec3_t pos;
		const btVector3& p = trans.getOrigin();
		VectorSet(pos, p.x(), p.y(), p.z());

		VectorCopy(pos, ent->s.pos.trBase);

		if(body->isActive())
		{
			ent->s.pos.trType =  TR_LINEAR;

			const btVector3& linearVelocity = body->getLinearVelocity();
			ent->s.pos.trDelta[0] = linearVelocity[0];
			ent->s.pos.trDelta[1] = linearVelocity[1];
			ent->s.pos.trDelta[2] = linearVelocity[2];
		}
		else
		{
			ent->s.pos.trType =  TR_STATIONARY;
			ent->s.pos.trDuration = 0;
			VectorClear(ent->s.pos.trDelta);
		}
		ent->s.pos.trTime = level.time;

		VectorCopy(pos, ent->r.currentOrigin);
		

		// set entityState_t::apos
		btQuaternion rotation = trans.getRotation();
		ent->s.apos.trType = TR_STATIONARY;
		ent->s.apos.trTime = level.time;

		QuatToAngles(rotation, ent->s.apos.trBase);

		ent->s.generic1 = body->getActivationState();

		trap_LinkEntity(ent);
	}
}

static void G_PhysicsBox_Think(gentity_t * ent)
{
	btCollisionObject* obj = (btCollisionObject*) ent->physicsRigidBody;
			
	btRigidBody* body = btRigidBody::upcast(obj);
	if (body && body->getMotionState())
	{
		delete body->getMotionState();
	}
	g_bulletPhysics->GetDynamicsWorld()->removeCollisionObject(obj);
	delete obj;

	G_FreeEntity(ent);

	//ent->nextthink = level.time + FRAMETIME;
	//trap_LinkEntity(ent);
}

static void G_PhysicsBox_InitPhysics(gentity_t * ent, const vec3_t start, const vec3_t dir)
{
	vec3_t          mins = { -8, -8, -8 };
	vec3_t          maxs = { 8, 8, 8 };
		
	btBoxShape* boxShape = new btBoxShape(btVector3(maxs[0], maxs[1], maxs[2]));
	boxShape->initializePolyhedralFeatures();
		
	//g_bulletPhysics->.getCollisionShapes().add(collisionShape);

	const btVector3 btStart(start[0], start[1], start[2]);

	btTransform startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btStart);

	float mass = 1000.0f;

	// rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic) 
	{
		boxShape->calculateLocalInertia(mass, localInertia);
	}

	btRigidBody* body = g_bulletPhysics->CreateRigidBody(mass, startTransform, boxShape);
	body->setLinearFactor(btVector3(1, 1, 1));
		
	body->getWorldTransform().setOrigin(btStart);

	btVector3 vel(dir[0], dir[1], dir[2]);
	vel *= 150;

	body->setLinearVelocity(vel);
		
	body->setAngularVelocity(btVector3(0,0,0));
	body->setContactProcessingThreshold(1e30);
		
	//enable CCD if the object moves more than 1 meter in one simulation frame
	//rigidBody.setCcdSweptSphereRadius(20);

	if (g_physUseCCD.integer)
	{
		body->setCcdMotionThreshold(maxs[0]);
		body->setCcdSweptSphereRadius(6);
	}

	body->setUserPointer(ent);
	//ent->physicsCollisionShape = (plCollisionShapeHandle*) boxShape;
	ent->physicsRigidBody = (plRigidBodyHandle*) body;
}

static void G_ShootBox(const vec3_t start, const vec3_t dir)
{
	gentity_t      *bolt;
	vec3_t          mins = { -8, -8, -8 };
	vec3_t          maxs = { 8, 8, 8 };

	//VectorNormalize(forward);

	bolt = G_Spawn();
	bolt->classname = "physicsbox";
	bolt->nextthink = level.time + 15000;
	bolt->think = G_PhysicsBox_Think;
	bolt->physics = G_PhysicsBox_Physics;
	bolt->s.eType = ET_PHYSICS_BOX;
	bolt->r.svFlags = SVF_BROADCAST;
	//bolt->s.weapon = WP_ROCKET_LAUNCHER;
	//bolt->r.ownerNum = ent->s.number;
	//bolt->parent = ent;
	//bolt->damage = 100;
	//bolt->splashDamage = 100;
	//bolt->splashRadius = 120;
	//bolt->methodOfDeath = MOD_ROCKET;
	//bolt->splashMethodOfDeath = MOD_ROCKET_SPLASH;
	//bolt->clipmask = MASK_SHOT;
	//bolt->target_ent = NULL;

	VectorCopy(start, bolt->s.origin);

	// make the rocket shootable
	bolt->r.contents = CONTENTS_SOLID;
	VectorCopy(mins, bolt->r.mins);
	VectorCopy(maxs, bolt->r.maxs);
	//bolt->takedamage = qtrue;
	//bolt->health = 50;
	//bolt->die = G_Missile_Die;

	bolt->s.pos.trType = TR_LINEAR;
	VectorScale(dir, 30, bolt->s.pos.trDelta);

	bolt->s.pos.trTime = level.time;// - 50;	// move a bit on the very first frame
	VectorCopy(start, bolt->s.pos.trBase);

	//SnapVector(bolt->s.pos.trDelta);	// save net bandwidth
	VectorCopy(start, bolt->r.currentOrigin);

	G_PhysicsBox_InitPhysics(bolt, start, dir);

	//G_SetOrigin(bolt, start);

	trap_LinkEntity(bolt);
}

void Cmd_PhysicsTest_ShootBox_f(gentity_t * ent)
{
	vec3_t			forward, right, up;
	vec3_t			start, start2;

	//AngleVectors(ent->s.angles2, forward, right, up);
	VectorCopy(ent->s.pos.trBase, start);

	// set aiming directions
	CalcMuzzlePoint(ent, forward, right, up, start, ent->s.weapon, qfalse);
	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	VectorMA(ent->client->ps.origin, 50, forward, start);

	for(int i = -48; i < 48; i += 12)
	{
		VectorMA(start, i, right, start2);
		VectorMA(start2, playerMins[1] + 5, forward, start2);

		G_ShootBox(start2, forward);
	}
}

} // extern "C"