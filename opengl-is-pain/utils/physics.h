#pragma once

#include <glad.h>

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/LinearMath/btIDebugDraw.h>

#include "utils.h"
#include "shader.h"
#include "scene/camera.h"

namespace
{
    using namespace engine::scene;
    using namespace engine::resources;
}

namespace engine::physics
{
    // Debug drawer for Physics engine
    class GLDebugDrawer : public btIDebugDraw
    {
        int m_debugMode;
        Camera* camera;
        Shader* shader;

    public:
        GLDebugDrawer(Camera& camera, Shader& shader) :
            m_debugMode(0),
            camera(&camera),
            shader(&shader)
        {}

        virtual void   drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
        {
            GLfloat vertices[6];
            vertices[0] = from.x(); vertices[3] = to.x();
            vertices[1] = from.y(); vertices[4] = to.y();
            vertices[2] = from.z(); vertices[5] = to.z();

            glm::vec3 colors = { 1, 0, 0 };
            GLuint indices[] = { 0,1 };

            GLuint vao, vbo, ebo;

            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);

            //UPLOADING VERTEX
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER,
                sizeof(GLfloat) * 6, vertices, GL_STATIC_DRAW);
            //UPLOADING INDEXES
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                sizeof(GLuint) * 2,
                indices,
                GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                sizeof(GLfloat) * 3, (GLvoid*)0);
            glBindVertexArray(0);


            //use program
            shader->bind();
            shader->setVec3("colorIn", colors);
            shader->setMat4("projectionMatrix", camera->projectionMatrix());
            shader->setMat4("viewMatrix", camera->viewMatrix());

            //use geometry
            glBindVertexArray(vao);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glDrawElements(GL_LINES, 2, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            //delete buffers
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }

        virtual void   drawSphere(const btVector3& p, btScalar radius, const btVector3& color) {}

        virtual void   drawTriangle(const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& color, btScalar alpha) {}

        virtual void   drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {}

        virtual void   reportErrorWarning(const char* warningString) { utils::io::log(warningString); }

        virtual void   draw3dText(const btVector3& location, const char* textString) {}

        virtual void   setDebugMode(int debugMode) { m_debugMode = debugMode; }

        virtual int    getDebugMode() const { return m_debugMode; }

    };

    class PhysicsEngine
    {
    public:
        //enum to identify the 2 considered Collision Shapes
        enum ColliderShape { BOX, SPHERE };

        struct RigidBodyCreateInfo
        {
            ColliderShape type{ ColliderShape::BOX };
            glm::vec3 size {1.0f, 1.0f, 1.0f};
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
        PhysicsEngine()
        {
            // Collision configuration, to be used by the collision detection class
            // collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
            collisionConfiguration = new btDefaultCollisionConfiguration();

            // default collision dispatcher (=collision detection method). For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
            dispatcher = new btCollisionDispatcher(collisionConfiguration);

            // btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
            overlappingPairCache = new btDbvtBroadphase();

            // we set a ODE solver, which considers forces, constraints, collisions etc., to calculate positions and rotations of the rigid bodies.
            // the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
            solver = new btSequentialImpulseConstraintSolver();

            //  DynamicsWorld is the main class for the physical simulation
            dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);

            // we set the gravity force
            dynamicsWorld->setGravity(btVector3(0.0f, -9.82f, 0.0f));

            debugDrawer = nullptr;
        }

        ~PhysicsEngine()
        {
            clear();
        }

        //////////////////////////////////////////
        // Method for the creation of a rigid body, based on a Box or Sphere Collision Shape
        // The Collision Shape is a reference solid that approximates the shape of the actual object of the scene. The Physical simulation is applied to these solids, and the rotations and positions of these solids are used on the real models.
        btRigidBody* createRigidBody(glm::vec3 pos, glm::vec3 rot, RigidBodyCreateInfo rb_info)
        {
            btCollisionShape* collision_shape = NULL;

            // we convert the glm vector to a Bullet vector
            btVector3 position = btVector3(pos.x, pos.y, pos.z);

            // we convert from degrees to radians because in setEuler function BULLET USES RADIANS BUT DOESNT SAY 
            glm::vec3 rot_radians {glm::radians(rot)};

            // we set a quaternion from the Euler angles passed as parameters
            btQuaternion rotation;
            
            rotation.setEuler(rot_radians.y, rot_radians.x, rot_radians.z);

            // Box Collision shape
            if (rb_info.type == BOX)
            {
                // we convert the glm vector to a Bullet vector
                btVector3 dim = btVector3(rb_info.size.x, rb_info.size.y, rb_info.size.z);
                // BoxShape
                collision_shape = new btBoxShape(dim);
            }
            // Sphere Collision Shape (in this case we consider only the first component)
            else if (rb_info.type == SPHERE)
                collision_shape = new btSphereShape(rb_info.size.x);
            else
                collision_shape = new btSphereShape(rb_info.size.x); // If nothing, use sphere collider

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
            btVector3 localInertia(0.0f, 0.0f, 0.0f);
            if (isDynamic)
                collision_shape->calculateLocalInertia(mass, localInertia);

            // we initialize the Motion State of the object on the basis of the transformations
            // using the Motion State, the physical simulation will calculate the positions and rotations of the rigid body
            btDefaultMotionState* motionState = new btDefaultMotionState(objTransform);

            // we set the data structure for the rigid body
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, collision_shape, localInertia);
            // we set friction and restitution
            rbInfo.m_friction = rb_info.friction;
            rbInfo.m_restitution = rb_info.restitution;

            // if the Collision Shape is a sphere
            if (rb_info.type == SPHERE) {
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

        void addDebugDrawer(btIDebugDraw* newDebugDrawer)
        {
            debugDrawer = newDebugDrawer;
            dynamicsWorld->setDebugDrawer(newDebugDrawer);
        }

        void set_debug_mode(int isDebug)
        {
            debugDrawer->setDebugMode(isDebug);
        }

        void step(float delta_time)
        {
            dynamicsWorld->stepSimulation(delta_time, 10);
            if (debugDrawer && debugDrawer->getDebugMode())
            {
                dynamicsWorld->debugDrawWorld();
            }
        }
    private:
        btIDebugDraw* debugDrawer;

        void clear()
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