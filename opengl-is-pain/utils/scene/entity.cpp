#include "entity.h"

namespace
{
	using namespace engine::components;
}

namespace engine::scene
{
	// Object in scene
	
	Entity::Entity(std::string display_name, Model& drawable, Material& material) :
		display_name { display_name }, model{ &drawable }, material{ &material }
	{}

	Entity::~Entity()
	{
		//utils::io::log(utils::io::INFO, "Deleting " + display_name);
		components.clear();
	}

	// draws using the provided shader instead of the material
	void Entity::custom_draw(Shader& shader) const noexcept
	{
		shader.bind();

		shader.setMat4("modelMatrix", _transform.world_matrix());
		model->draw();
			
		shader.unbind();
	}

	void Entity::init() noexcept
	{
		for (auto& c : components)
		{
			c->init();
		}
	}

	void Entity::draw() const noexcept
	{
		prepare_draw();

		material->bind();

		material->shader->setMat4("modelMatrix", _transform.world_matrix());

		model->draw();
		material->unbind();
	}

	void Entity::update(float delta_time) noexcept
	{
		child_update(delta_time);

		// TODO check order of updates (components or child first?)
		for (auto& c : components)
		{
			c->update(delta_time);
		}
	}

	void Entity::on_collision(Entity& other, glm::vec3 contact_point, glm::vec3 norm, glm::vec3 impulse)
	{
		for (auto& c : components)
		{
			c->on_collision(other, contact_point, norm, impulse);
		}
	}

	const Entity::SceneState& Entity::scene_state() const
	{
		return _scene_state;
	}

#pragma region transform_stuff
	const Transform& Entity::transform() const noexcept { return _transform; }

	void Entity::set_position  (const glm::vec3& new_position)    noexcept { _transform.set_position(new_position);    on_transform_update(); }
	void Entity::set_rotation  (const glm::vec3& new_orientation) noexcept { _transform.set_rotation(new_orientation); on_transform_update(); }
	void Entity::set_size      (const glm::vec3& new_size)        noexcept { _transform.set_size(new_size);            on_transform_update(); }

	void Entity::translate     (const glm::vec3& translation)     noexcept { _transform.translate(translation); on_transform_update(); }
	void Entity::rotate        (const glm::vec3& rotation)        noexcept { _transform.rotate(rotation);       on_transform_update(); }
	void Entity::scale         (const glm::vec3& scale)           noexcept { _transform.scale(scale);           on_transform_update(); }

	void Entity::set_transform (const glm::mat4& matrix, bool trigger_update) noexcept { _transform.set(matrix); if(trigger_update) on_transform_update(); }
#pragma endregion transform_stuff

	void Entity::prepare_draw() const noexcept {}
	void Entity::child_update(float delta_time) noexcept {}

	void Entity::on_transform_update()
	{
		for (auto& c : components)
		{
			c->on_transform_update();
		}
	}
}