#pragma once
#include <assimp/Importer.hpp>
#include <assimp/anim.h>
#include <assimp/camera.h>
#include <assimp/light.h>
#include <assimp/material.h>
#include <assimp/matrix4x4.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <assimp/texture.h>
#include <assimp/types.h>
#include <assimp/vector3.h>
#include <cassert>

using namespace Assimp;
class ModelImporter {

	void loadContent(const std::string &path, unsigned long int supportFlag) {
		Importer importer;
		const aiScene *pScene = importer.ReadFile(path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals
												  //| aiProcess_SplitLargeMeshes
		);

		/**/
		this->sceneRef = (aiScene *)pScene;

		if (pScene) {
			aiMatrix4x4 m_GlobalInverseTransform = pScene->mRootNode->mTransformation;
			m_GlobalInverseTransform.Inverse();

			this->initScene(pScene);
			importer.FreeScene();
		} else {
		}
		// return importSuccess;
	}

	void initScene(const aiScene *scene) {
		int x;

		// /*	 */
		// if (scene->HasMeshes() ) {
		// 	for (unsigned int x = 0; x < scene->mNumMeshes; x++) {
		// 		this->initMesh(scene->mMeshes[x], x);
		// 	}

		// }

		// /*	*/
		// if (scene->HasAnimations() ) {
		// 	for (unsigned int x = 0; x < scene->mNumAnimations; x++) {
		// 		this->initAnimation(scene->mAnimations[x], x);
		// 	}
		// }

		// /*	*/
		// if (scene->HasTextures()) {
		// 	for (unsigned int x = 0; x < scene->mNumTextures; x++) {
		// 		this->initTexture(scene->mTextures[x], x);
		// 	}
		// }

		// /*	*/
		// if (scene->HasMaterials() ) {
		// 	for (unsigned int x = 0; x < scene->mNumMaterials; x++) {
		// 		this->initMaterial(scene->mMaterials[x]);
		// 	}
		// }

		// /*	*/
		// if (scene->HasLights()) {
		// 	for (unsigned int x = 0; x < scene->mNumLights; x++) {
		// 		this->initLight(scene->mLights[x], x);
		// 	}
		// }

		// /*	*/
		// if (scene->HasCameras() ) {
		// 	for (unsigned int x = 0; x < scene->mNumCameras; x++) {
		// 		this->initCamera(scene->mCameras[x], x);
		// 	}
		// }

		// /*	*/
		// //if (this->getFlag() & IMPORT_HIERARCHY) {
		// 	this->initNoodeRoot(scene->mRootNode);
		// //}
	}

	void initMesh(const aiMesh *aimesh, unsigned int index) {
		unsigned int z;

		unsigned int vertexSize = 0;
		unsigned int indicesSize = 0;

		unsigned int VertexIndex = 0, IndicesIndex = 0, bonecount = 0, initilzebone = 0;

		indicesSize = 4; /*TODO remove later, if not it cased the geometry notbe rendered proparly*/
		float *vertices = (float *)malloc(aimesh->mNumVertices * vertexSize);
		unsigned char *Indice = (unsigned char *)malloc(indicesSize * aimesh->mNumFaces * 3);

		float *temp = vertices;
		unsigned char *Itemp = Indice;

		const aiVector3D Zero = aiVector3D(0, 0, 0);

		for (unsigned int x = 0; x < aimesh->mNumVertices; x++) {
			const aiVector3D *Pos = &(aimesh->mVertices[x]);
			aiVector3D *pNormal = &(aimesh->mNormals[x]);
			const aiVector3D *pTexCoord = aimesh->HasTextureCoords(0) ? &(aimesh->mTextureCoords[0][x]) : &Zero;
			// VDVector3 mtangent;

			*vertices++ = Pos->x;
			*vertices++ = Pos->y;
			*vertices++ = Pos->z;

			*vertices++ = pTexCoord->x;
			*vertices++ = pTexCoord->y;

			pNormal->Normalize();
			*vertices++ = pNormal->x;
			*vertices++ = pNormal->y;
			*vertices++ = pNormal->z;

			// mtangent = tangent(VDVector3(pNormal->x, pNormal->y, pNormal->z));
			// mtangent.makeUnitVector();

			*vertices++ = mtangent.x();
			*vertices++ = mtangent.y();
			*vertices++ = mtangent.z();

		} /**/

		vertices = temp;

		/*	some issues with this I thing? */
		for (unsigned int x = 0; x < aimesh->mNumFaces; x++) {
			const aiFace &face = aimesh->mFaces[x];
			assert(face.mNumIndices == 3); // Check if Indices Count is 3 other case error
			memcpy(Indice, &face.mIndices[0], indicesSize);
			Indice += indicesSize;
			memcpy(Indice, &face.mIndices[1], indicesSize);
			Indice += indicesSize;
			memcpy(Indice, &face.mIndices[2], indicesSize);
			Indice += indicesSize;
		}

		Indice = Itemp;

		/**/
		free(vertices);
		free(Indice);
	}

  private:
	Assimp::aiScene *sceneRef;
};
