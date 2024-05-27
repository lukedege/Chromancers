#pragma once

#include "../transform.h"
#include "../model.h"
#include "../utils.h"

namespace
{
	using namespace utils::math;
	using namespace engine::resources;
}

namespace engine::scene
{
	// Class abstracting a complex mesh/shape to perform easier calculations on scene entities (like frustum culling)
	struct BoundingVolume
	{
		virtual bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const = 0;

		virtual bool isOnOrForwardPlane(const Plane& plane) const = 0;

		bool isOnFrustum(const Frustum& camFrustum) const
		{
			return (isOnOrForwardPlane(camFrustum.leftFace) &&
				isOnOrForwardPlane(camFrustum.rightFace) &&
				isOnOrForwardPlane(camFrustum.topFace) &&
				isOnOrForwardPlane(camFrustum.bottomFace) &&
				isOnOrForwardPlane(camFrustum.nearFace) &&
				isOnOrForwardPlane(camFrustum.farFace));
		};
	};

	struct BoundingSphere : public BoundingVolume
	{
		glm::vec3 center{ 0.f, 0.f, 0.f };
		float radius{ 0.f };

		BoundingSphere(const glm::vec3& center, float radius) : 
			BoundingVolume(),
			center{ center }, radius{ radius }
		{}

		BoundingSphere(const Model& model) : 
			BoundingVolume()
		{
			glm::vec3 minAABB = glm::vec3(std::numeric_limits<float>::max());
			glm::vec3 maxAABB = glm::vec3(std::numeric_limits<float>::min());
			for (auto&& mesh_entry : model.meshes)
			{
				for (auto&& vertex : mesh_entry.mesh.vertices)
				{
					minAABB.x = std::min(minAABB.x, vertex.position.x);
					minAABB.y = std::min(minAABB.y, vertex.position.y);
					minAABB.z = std::min(minAABB.z, vertex.position.z);

					maxAABB.x = std::max(maxAABB.x, vertex.position.x);
					maxAABB.y = std::max(maxAABB.y, vertex.position.y);
					maxAABB.z = std::max(maxAABB.z, vertex.position.z);
				}
			}

			center = (maxAABB + minAABB) * 0.5f;
			radius = glm::length(minAABB - maxAABB);
		}

		bool isOnOrForwardPlane(const Plane& plane) const final
		{
			return plane.getSignedDistanceToPlane(center) > -radius;
		}

		bool isOnFrustum(const Frustum& camFrustum, const Transform& transform) const final
		{
			//Get global scale thanks to our transform
			const glm::vec3 globalScale = transform.size();

			//Get our global center with process it with the global model matrix of our transform
			const glm::vec3 globalCenter{ transform.matrix() * glm::vec4(center, 1.f) };

			//To wrap correctly our shape, we need the maximum scale scalar.
			const float maxScale = std::max(std::max(globalScale.x, globalScale.y), globalScale.z);

			//Max scale is assuming for the diameter. So, we need the half to apply it to our radius
			BoundingSphere globalSphere(globalCenter, radius * (maxScale * 0.5f));

			//Check Firstly the result that have the most chance to failure to avoid to call all functions.
			return (globalSphere.isOnOrForwardPlane(camFrustum.leftFace) &&
				globalSphere.isOnOrForwardPlane(camFrustum.rightFace) &&
				globalSphere.isOnOrForwardPlane(camFrustum.farFace) &&
				globalSphere.isOnOrForwardPlane(camFrustum.nearFace) &&
				globalSphere.isOnOrForwardPlane(camFrustum.topFace) &&
				globalSphere.isOnOrForwardPlane(camFrustum.bottomFace));
		};
	};

}