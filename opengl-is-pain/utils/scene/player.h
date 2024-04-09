#pragma once

#include "camera.h"
#include "entity.h"
#include "utils/physics.h"

#include "utils/components/rigidbody_component.h"
#include "utils/components/paintball_component.h"

#include <glm/gtx/vector_angle.hpp>

namespace
{
	using namespace engine::physics;
	using namespace engine::components;
}

namespace engine::scene
{
	class Player
	{
	public:
		EntityBase player_entity;
		Entity* gun_entity;
		Model* bullet_model;
		Material* bullet_material;
		PhysicsEngine<Entity>* physics_engine;

		Camera first_person_camera;		
		float lerp_speed = 10.f;
		
		
		glm::vec3 viewmodel_offset{ 0 };
		float paintball_size{ 0.1f };
		glm::vec4 paint_color{ 1.f, 1.f, 0.f, 1.f };

		Player() :
			player_entity{"PlayerObject"},
			gun_entity{nullptr},
			first_person_camera{ glm::vec3{0,0,5} }
		{

		}

		void init()
		{
			if (gun_entity)
			{
				gun_entity->parent = &player_entity;
				gun_entity->rotate(glm::vec3{0, 90.f, 0});
			}
			if (physics_engine)
			{
				cs_bullet = create_projectile_collision_shape(*physics_engine);
			}
			
			player_entity.set_position(first_person_camera.position());
			player_entity.set_orientation({ 0, -first_person_camera.rotation().y, first_person_camera.rotation().z});
		}

		void update(float delta_time)
		{	
			sync_to_cam(delta_time);
		}

		// Creates and launches a paintball, if precise is true then there will be no spread among fired bullets 
		void shoot(bool precise, PhysicsEngine<Entity>& physics_engine) // TODO temp parameters
		{
			if (gun_entity)
			{
				Scene* current_scene = gun_entity->scene_state().current_scene;
				auto& rng = current_scene->rng; // TODO temp

				if (current_scene && bullet_model && bullet_material) // TODO temp, make a better check
				{
					Entity* bullet = current_scene->emplace_instanced_entity("bullets", "bullet", "This is a paintball", *bullet_model, *bullet_material);

					bullet->set_position(gun_muzzle_world_position() + gun_entity->world_transform().forward() * paintball_size * 2.f);
					glm::vec3 ori = player_entity.world_transform().orientation();
					glm::vec3 bullet_orientation = glm::vec3{ ori.z, ori.y, ori.x } - glm::vec3{0, 90.f, 0};
					bullet->set_orientation(bullet_orientation);
					bullet->set_size(glm::vec3(paintball_size));

					// setting up collision mask for paintball rigidbodies (to avoid colliding with themselves)
					CollisionFilter paintball_cf{ 1 << 7, ~(1 << 7) }; // this means "Paintball group is 1 << 7 (128), mask is everything but paintball group (inverse of the group bit)"

					// create compound collision shape for bullet
					//bullet->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1.0f, 0.1f, 0.1f, {ColliderShape::GIVEN}, cs_bullet }, paintball_cf, false);
					//bullet->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1.0f, 0.1f, 0.1f, {ColliderShape::SPHERE, glm::vec3{paintball_size} } }, paintball_cf, false);
					bullet->emplace_component<RigidBodyComponent>(physics_engine, RigidBodyCreateInfo{ 1.0f, 0.1f, 0.1f, {ColliderShape::BOX, glm::vec3{paintball_size, paintball_size, paintball_size*2}} }, paintball_cf, false);
					bullet->emplace_component<PaintballComponent>(paint_color);

					bullet->init();

					float speed_modifier = 20.f;
					glm::vec3 spread_modifier {0};

					if (!precise)
					{
						float speed_bias = 2.0f;
						float spread_bias = 1.5f;
						float speed_spread = rng.get_float(-1, 1) * speed_bias; // get float between -bias and +bias
						speed_modifier = 20.f + speed_spread;
						spread_modifier = normalize(glm::vec3{rng.get_float(-1, 1), rng.get_float(-1, 1), rng.get_float(-1, 1)})* spread_bias;
					}

					glm::vec3 shoot_dir = gun_entity->world_transform().forward() * speed_modifier + spread_modifier;
					btVector3 impulse = btVector3(shoot_dir.x, shoot_dir.y, shoot_dir.z);
					bullet->get_component<RigidBodyComponent>()->rigid_body->applyCentralImpulse(impulse);
				}
			}
		}

		glm::vec3 gun_muzzle_world_position()
		{
			glm::mat4 gun_world_transform = gun_entity ? gun_entity->world_transform().matrix() : glm::mat4{1.f};
			return glm::vec3 { gun_world_transform * glm::vec4 { local_muzzle_position, 1.f } };
		}

	private:
		glm::vec3 local_muzzle_position { 0.0f, 0.1f, 0.67f };
		btCollisionShape* cs_bullet;
		
		btCollisionShape* create_projectile_collision_shape(PhysicsEngine<Entity>& physics_engine)
		{
			//create a few dynamic rigidbodies
			// Re-using the same collision is better for memory usage and performance
			btBoxShape* cube = new btBoxShape(btVector3(paintball_size,paintball_size,paintball_size));
			physics_engine.collisionShapes.push_back(cube);
			btBoxShape* cube2 = new btBoxShape(btVector3(paintball_size*0.5, paintball_size*0.5, paintball_size));
			physics_engine.collisionShapes.push_back(cube2);
		
			// create a new compound shape for making an L-beam from `cube`s
			btCompoundShape* compoundShape = new btCompoundShape();
		
			btTransform transform;
		
			// add cubes in an L-beam fashion to the compound shape
			transform.setIdentity();
			transform.setOrigin(btVector3(0, 0, -paintball_size*0.5));
			compoundShape->addChildShape(transform, cube);
		
			transform.setIdentity();
			transform.setOrigin(btVector3(0, 0, paintball_size*0.5));
			compoundShape->addChildShape(transform, cube2);
		
			btScalar masses[2] = {1000, 1};
			btTransform principal;
			btVector3 inertia;
			compoundShape->calculatePrincipalAxisTransform(masses, principal, inertia);

			// new compund shape to store
			btCompoundShape* compound2 = new btCompoundShape();
			physics_engine.collisionShapes.push_back(compound2);
		
			// recompute the shift to make sure the compound shape is re-aligned
			for (int i = 0; i < compoundShape->getNumChildShapes(); i++)
				compound2->addChildShape(compoundShape->getChildTransform(i) * principal.inverse(),
										 compoundShape->getChildShape(i));
			delete compoundShape;
		
			return compound2;
		}

		void sync_to_cam(float delta_time)
		{
			
			//float dist = glm::distance(player_entity.world_transform().position(), first_person_camera.position());
			//float f_pos = std::clamp(((dist+0.1f) + (1/(dist+0.1f)))  * 8 * delta_time, 0.f, 1.f);
			//player_entity.set_position(glm::mix(player_entity.world_transform().position(), first_person_camera.position(), f_pos));
			
			//glm::vec3 norm_p_orient = glm::normalize(player_entity.world_transform().orientation());
			//glm::vec3 norm_c_orient = glm::normalize(glm::vec3{ 0, -first_person_camera.rotation().y, first_person_camera.rotation().z });
			//float angle_dist = glm::angle(norm_p_orient, norm_c_orient);

			float f_ang = std::clamp(lerp_speed * delta_time, 0.f, 1.f);

			player_entity.set_position(first_person_camera.position());
			player_entity.set_orientation(glm::mix(player_entity.world_transform().orientation(), { first_person_camera.rotation().x, -first_person_camera.rotation().y, first_person_camera.rotation().z}, f_ang));

			if (gun_entity)
			{
				gun_entity->set_position(viewmodel_offset);
			}

			//utils::io::info("player pos", glm::to_string(player_entity.local_transform().position()), " ", glm::to_string(player_entity.world_transform().position()));
			//utils::io::info("gun pos", glm::to_string(gun->local_transform().position()), " ", glm::to_string(gun->world_transform().position()));
			//utils::io::info("cam pos", glm::to_string(first_person_camera.position()), " ", glm::to_string(first_person_camera.position()));
		}
	};
}