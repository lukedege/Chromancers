#include "entity.h"

namespace
{
	using namespace engine::components;
}

namespace engine::scene
{
	// Object in scene
	EntityBase::EntityBase(std::string display_name) :
			display_name{ display_name }
		{}

	EntityBase::~EntityBase()
	{
		//utils::io::log(utils::io::INFO, "Deleting " + display_name);
		components.clear();
	}

	void EntityBase::init() noexcept
	{
		for (auto& c : components)
		{
			c->init();
		}
	}

	void EntityBase::update(float delta_time) noexcept
	{
		update_world_transform();
		for (auto& c : components)
		{
			c->update(delta_time);
		}
	}

	void EntityBase::on_collision(Entity& other, glm::vec3 contact_point, glm::vec3 norm, glm::vec3 impulse)
	{
		// Invoke the on_collision event for each component
		for (auto& c : components)
		{
			c->on_collision(other, contact_point, norm, impulse);
		}
	}

	const EntityBase::SceneState& EntityBase::scene_state() const
	{
		return _scene_state;
	}

#pragma region transform_stuff
	const Transform& EntityBase::local_transform() const noexcept { return _local_transform; }
	const Transform& EntityBase::world_transform() const noexcept { return _world_transform; }

	void EntityBase::set_position    (const glm::vec3& new_position)    noexcept { _local_transform.set_position(new_position);       on_transform_update(); }
	void EntityBase::set_orientation (const glm::vec3& new_orientation) noexcept { _local_transform.set_orientation(new_orientation); on_transform_update(); }
	void EntityBase::set_size        (const glm::vec3& new_size)        noexcept { _local_transform.set_size(new_size);               on_transform_update(); }

	void EntityBase::translate       (const glm::vec3& translation)     noexcept { _local_transform.translate(translation); on_transform_update(); }
	void EntityBase::rotate          (const glm::vec3& rotation)        noexcept { _local_transform.rotate(rotation);       on_transform_update(); }
	void EntityBase::scale           (const glm::vec3& scale)           noexcept { _local_transform.scale(scale);           on_transform_update(); }

	void EntityBase::set_transform (const glm::mat4& matrix, bool trigger_component_update) noexcept 
	{ 
		_local_transform.set(matrix); 
		update_world_transform();

		if(trigger_component_update) 
			on_transform_update(); 
	}
#pragma endregion transform_stuff

	void EntityBase::update_world_transform()
	{
		// Compute world transform from local and parent
		if (parent)
		{
			_world_transform = parent->world_transform() * _local_transform;
		}
		else
		{
			_world_transform = _local_transform;
		}
	}

	void EntityBase::on_transform_update()
	{
		update_world_transform();

		for (auto& c : components)
		{
			c->on_transform_update();
		}
	}

	Entity::Entity(std::string display_name, Model& drawable, Material& material) :
		EntityBase(display_name), 
		model{ &drawable }, material{ &material }
	{
		bounding_volume = std::make_unique<BoundingSphere>(drawable);
	}

	void Entity::draw() const noexcept
	{
		if (!(material))         { utils::io::error("ENTITY - Entity ", display_name, " has no material"); return; }
		if (!(model   ))         { utils::io::error("ENTITY - Entity ", display_name, " has no model")   ; return; }

		if (!(material->shader)) { utils::io::error("ENTITY - Entity's ", display_name, " material has no shader"); return; }

		Shader& current_shader = *material->shader;

		current_shader.bind();
		current_shader.setMat4("modelMatrix", _world_transform.matrix());

		// If the model has materials of its own, use them
		if (model->has_material())
		{
			model->draw(current_shader);
		}
		// otherwise use entity's material
		else
		{
			material->bind();
			model->draw();
			material->unbind();
		}

	}

	void Entity::custom_draw(const Shader& shader) const noexcept
	{
		if (!(model)) { utils::io::error("ENTITY - Entity ", display_name, " has no model"); return; }

		shader.bind();

		shader.setMat4("modelMatrix", _world_transform.matrix());
		model->draw();
			
		shader.unbind();
	}
}	