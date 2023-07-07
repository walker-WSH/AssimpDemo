#ifndef MESH_H
#define MESH_H

#include <Windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <d3d11_1.h>
#include <DirectXMath.h>

using namespace DirectX;

#include "SafeRelease.hpp"

struct VERTEX {
	FLOAT X, Y, Z;
	XMFLOAT2 texcoord;
};

struct Texture {
	std::string type;
	std::string path;
	ID3D11ShaderResourceView *texture;

	void Release() { SafeRelease(texture); }
};

class Mesh {
	std::vector<Texture> textures_;

	std::vector<VERTEX> vertices_;
	std::vector<UINT> indices_;

	// Render data
	ID3D11Buffer *VertexBuffer_ = nullptr;
	ID3D11Buffer *IndexBuffer_ = nullptr;

public:
	Mesh(ID3D11Device *dev, const std::vector<VERTEX> &vertices,
	     const std::vector<UINT> &indices, const std::vector<Texture> &textures)
		: vertices_(vertices),
		  indices_(indices),
		  textures_(textures),
		  VertexBuffer_(nullptr),
		  IndexBuffer_(nullptr)
	{
		assert(!vertices.empty() && !textures.empty());
		this->setupMesh(dev);
	}

	void Draw(ID3D11DeviceContext *devcon)
	{
		UINT stride = sizeof(VERTEX);
		UINT offset = 0;

		devcon->IASetVertexBuffers(0, 1, &VertexBuffer_, &stride, &offset);
		devcon->IASetIndexBuffer(IndexBuffer_, DXGI_FORMAT_R32_UINT, 0);
		devcon->PSSetShaderResources(0, 1, &textures_[0].texture);

		devcon->DrawIndexed(static_cast<UINT>(indices_.size()), 0, 0);
	}

	void Close()
	{
		SafeRelease(VertexBuffer_);
		SafeRelease(IndexBuffer_);
	}

private:
	// Functions
	// Initializes all the buffer objects/arrays
	void setupMesh(ID3D11Device *dev)
	{
		HRESULT hr;

		D3D11_BUFFER_DESC vbd;
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.ByteWidth = static_cast<UINT>(sizeof(VERTEX) * vertices_.size());
		vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbd.CPUAccessFlags = 0;
		vbd.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA initData;
		initData.pSysMem = &vertices_[0];

		hr = dev->CreateBuffer(&vbd, &initData, &VertexBuffer_);
		if (FAILED(hr)) {
			Close();
			throw std::runtime_error("Failed to create vertex buffer.");
		}

		D3D11_BUFFER_DESC ibd;
		ibd.Usage = D3D11_USAGE_IMMUTABLE;
		ibd.ByteWidth = static_cast<UINT>(sizeof(UINT) * indices_.size());
		ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ibd.CPUAccessFlags = 0;
		ibd.MiscFlags = 0;

		initData.pSysMem = &indices_[0];

		hr = dev->CreateBuffer(&ibd, &initData, &IndexBuffer_);
		if (FAILED(hr)) {
			Close();
			throw std::runtime_error("Failed to create index buffer.");
		}
	}
};

#endif
