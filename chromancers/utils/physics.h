#pragma once

#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/LinearMath/btIDebugDraw.h>
#include <bullet/BulletCollision/CollisionShapes/btShapeHull.h>

#include <glad.h>

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    // Simple debug drawer for drawing physics engine bodies and colliders
    class GLDebugDrawer : public btIDebugDraw
    {
        int m_debugMode;
        Camera* camera;
        Shader* shader;

		GLuint vao, vbo, ebo;

    public:
        GLDebugDrawer(Camera& camera, Shader& shader) :
            m_debugMode(0),
            camera(&camera),
            shader(&shader)
        {
			glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);
		}

		~GLDebugDrawer()
		{
            //delete buffers
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
		}

        virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
        {
            GLfloat vertices[6]{};
            vertices[0] = from.x(); vertices[3] = to.x();
            vertices[1] = from.y(); vertices[4] = to.y();
            vertices[2] = from.z(); vertices[5] = to.z();

            glm::vec3 colors = { 1, 0, 0 };
            GLuint indices[] = { 0,1 };

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
			
            shader->unbind();
        }

        virtual void   drawSphere(const btVector3& p, btScalar radius, const btVector3& color) {}

        virtual void   drawTriangle(const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& color, btScalar alpha) {}

        virtual void   drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) {}

        virtual void   reportErrorWarning(const char* warningString) { utils::io::error(warningString); }

        virtual void   draw3dText(const btVector3& location, const char* textString) {}

        virtual void   setDebugMode(int debugMode) { m_debugMode = debugMode; }

        virtual int    getDebugMode() const { return m_debugMode; }

    };

    // Enum to identify the various considered collision shapes
    enum ColliderShape { SPHERE, BOX, HULL };

    struct ColliderShapeCreateInfo
    {
        ColliderShape                 type          { ColliderShape::BOX };
        glm::vec3                     size          { 1.0f, 1.0f, 1.0f }; // Used whole if ColliderShape::BOX, only .x if ColliderShape::SPHERE
        const std::vector<glm::vec3>* hull_vertices {nullptr};            // Used if ColliderShape::HULL
    };

    struct RigidBodyCreateInfo
    {
        float mass         { 1.0f };
        float friction     { 0.1f };
        float restitution  { 0.1f };
        ColliderShapeCreateInfo cs_info;
    };

	// Struct used to store how a rigidbody behaves against other rigidbodies
    struct CollisionFilter
    {
        int group; // which group  the rigidbody is part of
        int mask ; // which groups the rigidbody should collide with
    };

    template<typename UserObject>
    class PhysicsEngine
    {
    public:
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

        // Creates an approximate convex hull given a set of vertices (e.g. a mesh). The "hi_res" parameter, if true, builds a more accurate hull.
        btConvexHullShape* createConvexHull(const std::vector<glm::vec3>& vertices, bool hi_res = false) const
        {
            // We create a hull from the provided vertices
            btConvexHullShape initial_convexHullShape;
            for (glm::vec3 v : vertices)
            {
                btVector3 bt_v{ v.x, v.y, v.z };
                initial_convexHullShape.addPoint(bt_v);
            }

            //Create a hull approximation
            initial_convexHullShape.setMargin(0);  // this is to compensate for a bug in bullet
            btShapeHull simpler_hull{ &initial_convexHullShape };
            simpler_hull.buildHull(0, hi_res);    

            // Build a convex hull shape from the approximate one
            btConvexHullShape* convex_hull_shape = new btConvexHullShape(
                (const btScalar*)simpler_hull.getVertexPointer(), simpler_hull.numVertices(), sizeof(btVector3));
            return convex_hull_shape;
        }

		// Creates a collision shape given the construction info
        btCollisionShape* createCollisionShape(ColliderShapeCreateInfo cs_info)
        {
            btCollisionShape* collision_shape{ nullptr };

            // Box Collision shape
            if (cs_info.type == BOX)
            {
                // we convert the glm vector to a Bullet vector
                btVector3 dim = btVector3(cs_info.size.x, cs_info.size.y, cs_info.size.z);
                // BoxShape
                collision_shape = new btBoxShape(dim);
            }
            // Sphere Collision Shape (in this case we consider only the first component)
            else if (cs_info.type == SPHERE)
            {
                collision_shape = new btSphereShape(cs_info.size.x);
            }
            else if (cs_info.type == HULL)
            {
                if (cs_info.hull_vertices)
                    collision_shape = createConvexHull(*cs_info.hull_vertices); 
                else
                    utils::io::error("PHYSICS - no vertices provided for convex hull construction!");
            }
            else
                collision_shape = new btSphereShape(cs_info.size.x); // If nothing, use sphere collider

            // we add this Collision Shape to the vector
            collisionShapes.push_back(collision_shape);

            return collision_shape;
        }

		// Creates and add a rigidbody to the dynamic world
        btRigidBody* addRigidBody(const glm::vec3 pos, const glm::vec3 rot, const RigidBodyCreateInfo rb_info)
        {
            btRigidBody* body = createRigidBody(pos, rot, rb_info);

            //add the body to the dynamics world
            dynamicsWorld->addRigidBody(body);

            // the function returns a pointer to the created rigid body
            // in a standard simulation (e.g., only objects falling), it is not needed to have a reference to a single rigid body, but in some cases (e.g., the application of an impulse), it is needed.
            return body;
        }

		// Creates and add a rigidbody to the dynamic world providing a collision filter for it
        btRigidBody* addRigidBody(const glm::vec3 pos, const glm::vec3 rot, const RigidBodyCreateInfo rb_info, const CollisionFilter cf)
        {
            btRigidBody* body = createRigidBody(pos, rot, rb_info);

            //add the body to the dynamics world
            dynamicsWorld->addRigidBody(body, cf.group, cf.mask);

            // the function returns a pointer to the created rigid body
            // in a standard simulation (e.g., only objects falling), it is not needed to have a reference to a single rigid body, but in some cases (e.g., the application of an impulse), it is needed.
            return body;
        }

		// Removes and deletes a rigidbody from the dynamic world
        void deleteRigidBody(btRigidBody* body)
        {
            // as of https://github.com/bulletphysics/bullet3/blob/master/examples/CommonInterfaces/CommonRigidBodyBase.h
            if (dynamicsWorld && body) // we need to check if dynamic world wasnt already destroyed (destroying all the rigid bodies itself with it)
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

		// Progress the dynamic world simulation by one step
        void step(float delta_time)
        {
            dynamicsWorld->stepSimulation(delta_time);
            if (debugDrawer && debugDrawer->getDebugMode())
            {
                dynamicsWorld->debugDrawWorld();
            }
        }

		// Detect and process rigidbody collisions, making user's object aware of eventual collisions happening
        void detect_collisions()
        {
            // Iterate through all manifolds in the dispatcher
            int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
            for (int i = 0; i < numManifolds; i++)
            {
                // A contact manifold is a cache that contains all contact points between pairs of collision objects.
                btPersistentManifold* contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
                const btCollisionObject* obA = contactManifold->getBody0();
                const btCollisionObject* obB = contactManifold->getBody1();

				// Retrieve the user's objects that have collided
                UserObject* ent_a = static_cast<UserObject*>(obA->getUserPointer());
                UserObject* ent_b = static_cast<UserObject*>(obB->getUserPointer());
                
                int numContacts = contactManifold->getNumContacts();
                for (int j = 0; j < numContacts; j++)
                {
                    btManifoldPoint& pt = contactManifold->getContactPoint(j);
                    if (pt.getDistance() < 0.1f) // if a bullet bounces instead of exploding, remove this check or increase the distance
                    {
						// We compute various informations about the collision
                        const btVector3& ptA = pt.getPositionWorldOnA();
                        const btVector3& ptB = pt.getPositionWorldOnB();
                        btVector3 normalOnB = pt.m_normalWorldOnB;
                        btVector3 impulse{ pt.m_appliedImpulse, pt.m_appliedImpulseLateral1, pt.m_appliedImpulseLateral2 };

                        glm::vec3 glm_ptA   { ptA.x(), ptA.y(), ptA.z() };
                        glm::vec3 glm_ptB   { ptB.x(), ptB.y(), ptB.z() };
                        glm::vec3 glm_norm  { normalOnB.x(), normalOnB.y(), normalOnB.z() };
                        glm::vec3 glm_impls { impulse.x(), impulse.y(), impulse.z()};

                        // Check that user pointers are not null
                        if (ent_a && ent_b)
                        {
							// We forward the computed collision informations to the objects
							// so that they can react to the collision and resolve it as they see fit
                            ent_a->on_collision(*ent_b, glm_ptA, glm_norm, glm_impls);
                            ent_b->on_collision(*ent_a, glm_ptB, glm_norm, glm_impls);
                        }
                    }
                    
                }
                
            }
        }
    private:
        btIDebugDraw* debugDrawer;

        // Method for the creation of a rigid body, based on a Box or Sphere Collision Shape
        // The Collision Shape is a reference solid that approximates the shape of the actual object of the scene. The Physical simulation is applied to these solids, and the rotations and positions of these solids are used on the real models.
        btRigidBody* createRigidBody(const glm::vec3 pos, const glm::vec3 rot, const RigidBodyCreateInfo rb_info)
        {
            // we convert the glm vector to a Bullet vector
            btVector3 position = btVector3(pos.x, pos.y, pos.z);

            // we convert from degrees to radians because in setEuler function BULLET USES RADIANS BUT DOESNT SAY 
            glm::vec3 rot_radians {glm::radians(rot)};

            // we set a quaternion from the Euler angles passed as parameters
            btQuaternion rotation;
            
            rotation.setEuler(rot_radians.y, rot_radians.x, rot_radians.z);

            // We set the initial transformations
            btTransform objTransform;
            objTransform.setIdentity();
            objTransform.setRotation(rotation);
            // we set the initial position (it must be equal to the position of the corresponding model of the scene)
            objTransform.setOrigin(position);

            // If a collision shape is provided we will adopt it and ignore cs creation info
			btCollisionShape* collision_shape = createCollisionShape(rb_info.cs_info);

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
            if (rb_info.cs_info.type == SPHERE) {
                // the sphere touches the plane on the plane on a single point, and thus the friction between sphere and the plane does not work -> the sphere does not stop
                // to avoid the problem, we apply the rolling friction together with an angular damping (which applies a resistence during the rolling movement), in order to make the sphere to stop after a while
                rbInfo.m_angularDamping = 0.3f;
                rbInfo.m_rollingFriction = 0.3f;
            }

            // we create the rigid body
            btRigidBody* body = new btRigidBody(rbInfo);

            // the function returns a pointer to the created rigid body
            // in a standard simulation (e.g., only objects falling), it is not needed to have a reference to a single rigid body, but in some cases (e.g., the application of an impulse), it is needed.
            return body;
        }

        void clear()
        {
            //we remove the rigid bodies from the dynamics world and delete them
            for (int i=dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
            {
                // we remove all the Motion States
                btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[i];
                // we upcast in order to use the methods of the main class RigidBody
                btRigidBody* body = btRigidBody::upcast(obj);
                if (body && body->getMotionState())
                {
                    delete body->getMotionState();
                }
                dynamicsWorld->removeCollisionObject( obj );
                delete obj;
            }

            //delete dynamics world
            delete dynamicsWorld;
            dynamicsWorld = nullptr;

            //delete solver
            delete solver;
            solver = nullptr;

            //delete broadphase
            delete overlappingPairCache;
            overlappingPairCache = nullptr;

            //delete dispatcher
            delete dispatcher;
            dispatcher = nullptr;

            delete collisionConfiguration;
            collisionConfiguration = nullptr;

            collisionShapes.clear();
        }
    };

	// Utility functions to convert bt vectors to glm
    glm::vec3 to_glm_vec3(const btVector3& bt_vec)
    {
        return { bt_vec.x(), bt_vec.y(), bt_vec.z()};
    }

    glm::vec4 to_glm_vec4(const btVector4& bt_vec)
    {
        return { bt_vec.x(), bt_vec.y(), bt_vec.z(), bt_vec.w() };
    }
}