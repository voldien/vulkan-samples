#pragma once
#include "../Scene/Skybox.h"
#include "VKDataStructure.h"

namespace vksample {

	class VKSampleSessionBase;

	class FVDECLSPEC MeshProcedural {
	  public:
		MeshProcedural(VKSampleSessionBase &vkBase);

		void loadPlan(MeshObject &planMesh, const float scale, const int segmentX = 1, const int segmentY = 1);
		void loadCube(MeshObject &cubeMesh, const float scale, const int segmentX = 1, const int segmentY = 1);
		void loadSphere(MeshObject &sphereMesh, const float radius = 1, const int slices = 8, const int segements = 8);



	  private:
		VKSampleSessionBase &vkBase;
	};
} // namespace vksample
