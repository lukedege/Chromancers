#pragma once

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <bullet/btBulletDynamicsCommon.h>

namespace utils::graphics::opengl
{
    ///////////////////  Physics class ///////////////////////
    class Physics
    {
    public:
        //enum to identify the 2 considered Collision Shapes
        enum ColliderShape { BOX, SPHERE };

        struct RigidBodyCreateInfo
        {
            ColliderShape type{ ColliderShape::BOX };
            float mass{ 1.0f };
            float friction{ 0.1f };
            float restitution{ 0.1f };
        };

        btDiscreteDynamicsWorld* dynamicsWorld; // the main physical simulation class
        btAlignedObjectArray<btCollisionShape*> collisionShapes; // a vector for all the Collision Shapes of the scene
        btDefaultCollisionConfiguration* collisionConfiguration; // setup for the collision manager
        btCollisionDispatcher* dispatcher; // collision manager
        btBroadphaseInterface* overlappingPairCache; // method for the broadphase collision detection
        btSequentialImpulseConstraintSolver* solver; // constraints solver


        //////////////////////////////////////////
        // constructor
        // we set all the classes needed for the physical simulation
        Physics()
        {
            // Collision configuration, to be used by the collision detection class
            // collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
            this->collisionConfiguration = new btDefaultCollisionConfiguration();

            // default collision dispatcher (=collision detection method). For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
            this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);

            // btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
            this->overlappingPairCache = new btDbvtBroadphase();

            // we set a ODE solver, which considers forces, constraints, collisions etc., to calculate positions and rotations of the rigid bodies.
            // the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
            this->solver = new btSequentialImpulseConstraintSolver();

            //  DynamicsWorld is the main class for the physical simulation
            this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher,this->overlappingPairCache,this->solver,this->collisionConfiguration);

            // we set the gravity force
            this->dynamicsWorld->setGravity(btVector3(0.0f,-9.82f,0.0f));
        }

        //////////////////////////////////////////
        // Method for the creation of a rigid body, based on a Box or Sphere Collision Shape
        // The Collision Shape is a reference solid that approximates the shape of the actual object of the scene. The Physical simulation is applied to these solids, and the rotations and positions of these solids are used on the real models.
        btRigidBody* createRigidBody(glm::vec3 pos, glm::vec3 size, glm::vec3 rot, RigidBodyCreateInfo rb_info)
        {
            btCollisionShape* collision_shape = NULL;

            // we convert the glm vector to a Bullet vector
            btVector3 position = btVector3(pos.x,pos.y,pos.z);

            // we set a quaternion from the Euler angles passed as parameters
            btQuaternion rotation;
            rotation.setEuler(rot.x,rot.y,rot.z);

            // Box Collision shape
            if (rb_info.type == BOX)
            {
                // we convert the glm vector to a Bullet vector
                btVector3 dim = btVector3(size.x,size.y,size.z);
                // BoxShape
                collision_shape = new btBoxShape(dim);
            }
            // Sphere Collision Shape (in this case we consider only the first component)
            else if (rb_info.type == SPHERE)
                collision_shape = new btSphereShape(size.x);

            // we add this Collision Shape to the vector
            this->collisionShapes.push_back(collision_shape);

            // We set the initial transformations
            btTransform objTransform;
            objTransform.setIdentity();
            objTransform.setRotation(rotation);
            // we set the initial position (it must be equal to the position of the corresponding model of the scene)
            objTransform.setOrigin(position);

            // if objects has mass = 0 -> then it is static (it does not move and it is not subject to forces)
            btScalar mass = rb_info.mass;
            bool isDynamic = (mass != 0.0f);

            // if it is dynamic (mass > 0) then we calculates local inertia
            btVector3 localInertia(0.0f,0.0f,0.0f);
            if (isDynamic)
                collision_shape->calculateLocalInertia(mass,localInertia);

            // we initialize the Motion State of the object on the basis of the transformations
            // using the Motion State, the physical simulation will calculate the positions and rotations of the rigid body
            btDefaultMotionState* motionState = new btDefaultMotionState(objTransform);

            // we set the data structure for the rigid body
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,motionState,collision_shape,localInertia);
            // we set friction and restitution
            rbInfo.m_friction = rb_info.friction;
            rbInfo.m_restitution = rb_info.restitution;

            // if the Collision Shape is a sphere
            if (rb_info.type == SPHERE){
                // the sphere touches the plane on the plane on a single point, and thus the friction between sphere and the plane does not work -> the sphere does not stop
                // to avoid the problem, we apply the rolling friction together with an angular damping (which applies a resistence during the rolling movement), in order to make the sphere to stop after a while
                rbInfo.m_angularDamping = 0.3f;
                rbInfo.m_rollingFriction = 0.3f;
            }

            // we create the rigid body
            btRigidBody* body = new btRigidBody(rbInfo);

            //add the body to the dynamics world
            this->dynamicsWorld->addRigidBody(body);

            // the function returns a pointer to the created rigid body
            // in a standard simulation (e.g., only objects falling), it is not needed to have a reference to a single rigid body, but in some cases (e.g., the application of an impulse), it is needed.
            return body;
        }

        void deleteRigidBody(btRigidBody* body)
        {
            // as of https://github.com/bulletphysics/bullet3/blob/master/examples/CommonInterfaces/CommonRigidBodyBase.h
            if (body)
            {
                dynamicsWorld->removeRigidBody(body);
                btMotionState* ms = body->getMotionState();
                delete body;
                delete ms;
            }
        }

        //////////////////////////////////////////
        // We delete the data of the physical simulation when the program ends
        void Clear()
        {
            //we remove the rigid bodies from the dynamics world and delete them
            for (int i=this->dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
            {
                // we remove all the Motion States
                btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[i];
                // we upcast in order to use the methods of the main class RigidBody
                btRigidBody* body = btRigidBody::upcast(obj);
                if (body && body->getMotionState())
                {
                    delete body->getMotionState();
                }
                this->dynamicsWorld->removeCollisionObject( obj );
                delete obj;
            }

            //delete dynamics world
            delete this->dynamicsWorld;

            //delete solver
            delete this->solver;

            //delete broadphase
            delete this->overlappingPairCache;

            //delete dispatcher
            delete this->dispatcher;

            delete this->collisionConfiguration;

            this->collisionShapes.clear();
        }
    };

}