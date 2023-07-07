#include "ModelLoader.h"

ModelLoader::ModelLoader()
	: dev_(nullptr),
	  devcon_(nullptr),
	  meshes_(),
	  directory_(),
	  textures_loaded_(),
	  hwnd_(nullptr)
{
	// empty
}

ModelLoader::~ModelLoader()
{
	// empty
}

bool ModelLoader::Load(HWND hwnd, ID3D11Device *dev, ID3D11DeviceContext *devcon,
		       std::string filename)
{
	Assimp::Importer importer;

	const aiScene *pScene =
		importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (pScene == nullptr)
		return false;

	this->directory_ = filename.substr(0, filename.find_last_of("/\\"));

	this->dev_ = dev;
	this->devcon_ = devcon;
	this->hwnd_ = hwnd;

	processNode(pScene->mRootNode, pScene);

	return true;
}

void ModelLoader::Draw(ID3D11DeviceContext *devcon)
{
	for (size_t i = 0; i < meshes_.size(); ++i) {
		meshes_[i].Draw(devcon);
	}
}

Mesh ModelLoader::processMesh(aiMesh *mesh, const aiScene *scene)
{
	// Data to fill
	std::vector<VERTEX> vertices;
	std::vector<UINT> indices;
	std::vector<Texture> textures;

	// Walk through each of the mesh's vertices
	for (UINT i = 0; i < mesh->mNumVertices; i++) {
		VERTEX vertex;

		vertex.X = mesh->mVertices[i].x;
		vertex.Y = mesh->mVertices[i].y;
		vertex.Z = mesh->mVertices[i].z;

		if (mesh->mTextureCoords[0]) {
			vertex.texcoord.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.texcoord.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.push_back(vertex);

		ATLTRACE("---------vertex [%d/%d] x:%f  y:%f   z:%f,  %f, %f    \n", i,
			 mesh->mNumVertices, vertex.X, vertex.Y, vertex.Z, vertex.texcoord.x,
			 vertex.texcoord.y);
	}

	for (UINT i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];

		std::string points = "";
		for (UINT j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
			points += std::to_string(face.mIndices[j]);
			points += "  ";
		}

		ATLTRACE("index [%d/%d]  %s   \n", i + 1, mesh->mNumFaces, points.c_str());
	}

	if (mesh->mMaterialIndex >= 0) {
		aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

		std::vector<Texture> diffuseMaps =
			this->loadMaterialTextures(material, aiTextureType_DIFFUSE, scene);
		assert(!diffuseMaps.empty());
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	}

	return Mesh(dev_, vertices, indices, textures);
}

std::vector<Texture> ModelLoader::loadMaterialTextures(aiMaterial *mat, aiTextureType type,
						       const aiScene *scene)
{
	std::vector<Texture> textures;
	auto count = mat->GetTextureCount(type);
	for (UINT i = 0; i < count; i++) {
		aiString str;
		mat->GetTexture(type, i, &str);

		// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		for (UINT j = 0; j < textures_loaded_.size(); j++) {
			if (std::strcmp(textures_loaded_[j].path.c_str(), str.C_Str()) == 0) {
				textures.push_back(textures_loaded_[j]);
				skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}

		if (!skip) { // If texture hasn't been loaded already, load it
			Texture texture;
			texture.path = str.C_Str();

			const aiTexture *embeddedTexture = scene->GetEmbeddedTexture(str.C_Str());
			if (embeddedTexture != nullptr) {
				texture.texture = loadEmbeddedTexture(embeddedTexture);
			} else {
				auto path = directory_ + "\\" + std::string(str.C_Str());
				auto wpath = std::wstring(path.begin(), path.end());
				HRESULT hr = CreateWICTextureFromFile(dev_, devcon_, wpath.c_str(),
								      nullptr, &texture.texture);
				if (FAILED(hr)) {
					assert(false);
					MessageBox(hwnd_, "Texture couldn't be loaded", "Error!",
						   MB_ICONERROR | MB_OK);
				}
			}

			textures.push_back(texture);
			this->textures_loaded_.push_back(
				texture); // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
		}
	}
	return textures;
}

void ModelLoader::Close()
{
	for (auto &t : textures_loaded_)
		t.Release();

	for (size_t i = 0; i < meshes_.size(); i++) {
		meshes_[i].Close();
	}
}

void ModelLoader::processNode(aiNode *node, const aiScene *scene)
{
	for (UINT i = 0; i < node->mNumMeshes; i++) {
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		meshes_.push_back(this->processMesh(mesh, scene));
	}

	for (UINT i = 0; i < node->mNumChildren; i++) {
		this->processNode(node->mChildren[i], scene);
	}
}

ID3D11ShaderResourceView *ModelLoader::loadEmbeddedTexture(const aiTexture *embeddedTexture)
{
	HRESULT hr;
	ID3D11ShaderResourceView *texture = nullptr;

	if (embeddedTexture->mHeight != 0) {
		// Load an uncompressed ARGB8888 embedded texture
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = embeddedTexture->mWidth;
		desc.Height = embeddedTexture->mHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA subresourceData;
		subresourceData.pSysMem = embeddedTexture->pcData;
		subresourceData.SysMemPitch = embeddedTexture->mWidth * 4;
		subresourceData.SysMemSlicePitch =
			embeddedTexture->mWidth * embeddedTexture->mHeight * 4;

		ID3D11Texture2D *texture2D = nullptr;
		hr = dev_->CreateTexture2D(&desc, &subresourceData, &texture2D);
		if (FAILED(hr))
			MessageBox(hwnd_, "CreateTexture2D failed!", "Error!",
				   MB_ICONERROR | MB_OK);

		hr = dev_->CreateShaderResourceView(texture2D, nullptr, &texture);
		if (FAILED(hr))
			MessageBox(hwnd_, "CreateShaderResourceView failed!", "Error!",
				   MB_ICONERROR | MB_OK);

		return texture;
	}

	// mHeight is 0, so try to load a compressed texture of mWidth bytes
	const size_t size = embeddedTexture->mWidth;

	hr = CreateWICTextureFromMemory(
		dev_, devcon_, reinterpret_cast<const unsigned char *>(embeddedTexture->pcData),
		size, nullptr, &texture);
	if (FAILED(hr))
		MessageBox(hwnd_, "Texture couldn't be created from memory!", "Error!",
			   MB_ICONERROR | MB_OK);

	return texture;
}
