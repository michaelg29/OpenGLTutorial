#ifndef SPHERE_HPP
#define SPHERE_HPP

#include "../objects/model.h"

typedef struct {
	glm::vec3 pos;
	glm::vec3 norm;
	glm::vec2 texCoord;
} SphereVertex;

SphereVertex normalizeSphereVertex(glm::vec3 pos, float phi, float th) {
	glm::vec2 texCoord;
	texCoord.x = th / glm::two_pi<float>();
	texCoord.y = (phi + glm::half_pi<float>()) / glm::pi<float>();

	return { pos, pos, texCoord };
}

class Sphere : public Model {
public:
    Sphere(unsigned int maxNoInstances)
        : Model("sphere", maxNoInstances, NO_TEX | DYNAMIC) {}

    void init() {
		std::vector<SphereVertex> vertices;
		std::vector<GLuint> indices;

		// generate vertices
		unsigned int res = 100; // number of rows and columns
		float circleStep = glm::two_pi<float>() / (float)res; // angle step between cells
		float heightStep = glm::pi<float>() / (float)res; // height of row

		int row = 0;
		int noVertices = 0;
		float phi = -glm::half_pi<float>();
		float y = glm::sin(phi);
		float radius;

		for (; phi < glm::half_pi<float>() + heightStep; phi += heightStep, row++) {
			y = glm::sin(phi);
			radius = glm::cos(phi);
			int cell = 0;
			for (float th = 0; th < glm::two_pi<float>(); th += circleStep, cell++) {
				vertices.push_back(normalizeSphereVertex(
					glm::vec3(radius * glm::cos(th), y, radius * glm::sin(th)),
					phi, th
				));

				// add indices if not bottom row
				if (row)
				{
					int nextCell = (cell + 1) % res;
					indices.push_back(noVertices - res); // bottom left
					indices.push_back((row - 1) * res + nextCell); // bottom right
					indices.push_back(row * res + nextCell); // top right

					indices.push_back(noVertices - res); // bottom left
					indices.push_back(noVertices); // top left (this vertex)
					indices.push_back(row * res + nextCell); // top right
				}

				noVertices++;
			}
		}

		BoundingRegion br(glm::vec3(0.0f), 1.0f);

		Mesh ret = processMesh(br,
			(unsigned int)vertices.size(), &vertices[0].pos[0],
			(unsigned int)indices.size(), &indices[0],
			true);

		// colors
		ret.setupMaterial(Material::white_plastic);
		
		addMesh(&ret);
    }
};

#endif