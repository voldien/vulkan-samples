#ifndef _VKS_PROCEDURAL_GEOMETRY_H_
#define _VKS_PROCEDURAL_GEOMETRY_H_ 1
#include <vector>

class ProceduralGeometry {
  public:
	typedef struct _vertex_t {
		float vertex[3];
		float uv[2];
		float normal[3];
		float tangent[3];
	} Vertex;

	void generateCube(float scale, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices);
};

#endif
