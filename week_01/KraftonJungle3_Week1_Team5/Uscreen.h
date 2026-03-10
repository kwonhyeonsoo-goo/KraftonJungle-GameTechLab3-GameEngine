#pragma once
#include "UGameObject.h"

class UShader;
class UTextureMesh;

class Uscreen : public UGameObject
{
public:

    Uscreen(const wchar_t* flnm)
    {
        filename = flnm;

    }

    virtual ~Uscreen() override
    {

    }


	void Create(ID3D11Device* device, ID3D11DeviceContext* context);

	void Physics_Update(float tick) override;
	void Update(float tick) override;
	void Render(ID3D11DeviceContext* context, ID3D11Device* device) override;
	const char* GetEditorTypeName() const override { return "Uscreen"; }
	void Release() override;
	bool LoadTexture(ID3D11Device* device, ID3D11DeviceContext* context);
	void Init(ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
private:
	UTextureMesh* Mesh;
	UShader* Shader;
	const wchar_t* filename;
	ID3D11ShaderResourceView* gTexture = nullptr;

};



