#include "UWave.h"
#include "UUIImage.h"

UWave::UWave()
{
}

void UWave::Create(ID3D11Device* device, ID3D11DeviceContext* context)
{
	float offset = 2.f / 27.f; // 27개의 파도가 화면 전체를 덮도록 간격 계산
	for (int i = 0; i < 27; ++i)
	{
		Waves[i] = new UUIImage();
		Waves[i]->Create(device, context);
		Waves[i]->SetTexture(L"Resource/Image/objects/wave.png");
		Waves[i]->SetPosition(FVector3( -1 + offset * i+ offset/2, -1.f, 0.f ));
		Waves[i]->SetScale(1.35f);
	}
}

void UWave::Release()
{

}



void UWave::Physics_Update(float tick)
{
}

void UWave::Update(float tick)
{
	ElaspedTime += tick;

    int frameStep = static_cast<int>(ElaspedTime / FrameDuration);
    
    if (frameStep != prevFrameStep)
    {
        for (int i = 0; i < 27; ++i) {
            int moves = rand() % 3 - 1;   // -1,0,1
            FVector3 pos = Waves[i]->GetPosition();

            int count = 35;
            int cycle = count * 2 - 2;

            int moveCycle = frameStep % cycle;
            pos.y = -1.15f + (count - abs(moveCycle - (count - 1))) * 0.005f;

            // 개별 운동
            pos.y += moves * 0.005f;

            Waves[i]->SetPosition(pos);
        }
        prevFrameStep = frameStep;
    }
    
    for (int i = 0; i < 27; ++i)
    {
        if (Waves[i])
        {
            
        }
    }
}

void UWave::Render(ID3D11DeviceContext* context, ID3D11Device* device)
{
	for (int i = 0; i < 27; ++i)
	{
		Waves[i]->Render(context, device);
	}

}

