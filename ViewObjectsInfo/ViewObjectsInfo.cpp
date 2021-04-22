#include "plugin.h"
#include "CWorld.h"
#include "CCoronas.h"
#include "CCamera.h"
#include "CSprite.h"
#include "CFont.h"
#include "CRenderer.h"
#include "CModelInfo.h"
#include "TestCheat.h"
#include "CVisibilityPlugins.h"
#include "..\injector\assembly.hpp"

using namespace plugin;
using namespace std;

bool show = false;

//fstream lg;

void FixAspectRatio(float* x, float* y)
{
	float resX = (float)RsGlobal.maximumWidth;
	float resY = (float)RsGlobal.maximumHeight;
	resY *= 1.33333333f;
	resX /= resY;

	*x /= resX;
	*y /= 1.07142857f;
}

void DrawString(string text, float posX, float posY, float sizeX, float sizeY)
{
    if (sizeX < 0.01f) return; // size limit

    float alpha = 0.0f;
    if (sizeX < 0.1f) {
        alpha += (sizeX * 10.0f);
        sizeX = 0.1f;
    }
    else
    {
        alpha = 1.0f;
    }
    int finalAlpha = (int)(alpha * 255.0f);
    if (sizeY < 0.2f) sizeY = 0.2f;

    CRGBA fontColor = { 255, 255, 255, 255 };
    fontColor.a = finalAlpha;
    CRGBA edgeColor = { 0, 0, 0, 255 };
    edgeColor.a = finalAlpha;

    sizeX *= 1.2f;
    sizeY *= 1.2f;
	FixAspectRatio(&sizeX, &sizeY);

	float magicResolutionWidth = RsGlobal.maximumWidth * 0.0015625f;
	float magicResolutionHeight = RsGlobal.maximumHeight * 0.002232143f;
    float finalSizeX = sizeX * magicResolutionWidth;
    float finalSizeY = sizeY * magicResolutionHeight;

	CFont::SetScale(finalSizeX, finalSizeY);
	CFont::SetFontStyle(1);
	CFont::SetProportional(true);
	CFont::SetJustify(true);
	CFont::SetOrientation(eFontAlignment::ALIGN_CENTER);

    if (finalAlpha > 128)
    {
        CFont::SetEdge(1);
        CFont::SetDropColor(edgeColor);
    }
    else
    {
        CFont::SetEdge(0);
    }

    CFont::SetBackground(false, false);
    CFont::SetColor(fontColor);

	CFont::SetCentreSize(640.0f * magicResolutionWidth);

	CFont::PrintString(posX * magicResolutionWidth, posY * magicResolutionHeight, &text[0]);
}

list<CEntity*> entityList;

void ShowForEntity(CEntity* entity)
{
    entityList.push_back(entity);
}

void _fastcall hookPreRenderEntiy(CEntity* entity)
{
    if (show) ShowForEntity(entity);
    CBaseModelInfo *modelInfo = CModelInfo::GetModelInfo(entity->m_nModelIndex);
    if (modelInfo->m_nNum2dEffects > 0)
    {
        entity->ProcessLightsForEntity();
    }
}

ThiscallEvent <AddressList<0x535FCD, H_CALL>, PRIORITY_AFTER, ArgPickN<CEntity*, 0>, void(CEntity*)> preRenderEntity;

class ViewObjectsInfo
{
public:
    ViewObjectsInfo()
    {
        //lg.open("ViewObjectsInfo.log", fstream::out | fstream::trunc);

        Events::processScriptsEvent += []
        {
            if (TestCheat("MODELSINFO"))
            {
                show = !show;
            }
            if (show)
            {
                CPed* playa = FindPlayerPed();
                if (playa && playa->m_pIntelligence->GetTaskUseGun())
                {
                    CVector startPos = playa->GetPosition();
                    CVector outCamPos;
                    CVector outPointPos;
                    TheCamera.Find3rdPersonCamTargetVector(200.0f, startPos, &outCamPos, &outPointPos);

                    CColPoint outColPoint;
                    CEntity* outEntity;
                    if (CWorld::ProcessLineOfSight(outCamPos, outPointPos, outColPoint, outEntity, true, true, true, true, false, false, false, false))
                    {
                        CCoronas::RegisterCorona(outEntity->m_nModelIndex + 6969, nullptr, 255, 100, 50, 255, outColPoint.m_vecPoint, 1.0f, 500.0f, eCoronaType::CORONATYPE_SHINYSTAR, eCoronaFlareType::FLARETYPE_NONE, false, false, 0, 0.0f, false, 0.1f, 0, 50.0f, false, false);
                        //lg << outColPoint.m_vecPoint.x << " " << outColPoint.m_vecPoint.y << " " << outColPoint.m_vecPoint.z << endl;

                        RwV3d screenPos2D;
                        float sizeX, sizeY;

                        if (CSprite::CalcScreenCoors(outColPoint.m_vecPoint.ToRwV3d(), &screenPos2D, &sizeX, &sizeY, true, true))
                        {
                            float x = (screenPos2D.x / RsGlobal.maximumWidth) * 640.0f;
                            float y = (screenPos2D.y / RsGlobal.maximumHeight) * 448.0f;
                            sizeX = (sizeX / RsGlobal.maximumWidth) * 15.0f;
                            sizeY = (sizeY / RsGlobal.maximumHeight) * 15.0f;

                            string text = to_string(outEntity->m_nModelIndex);
                            DrawString(text, x, y, sizeX, sizeY);
                        }

                    }
                }
            }
        };

        Events::drawingEvent.after += []
        {
            if (show)
            {
                for (auto const& entity : entityList)
                {
                    RwV3d screenPos2D;
                    float sizeX, sizeY;
                    if (CSprite::CalcScreenCoors(entity->GetPosition().ToRwV3d(), &screenPos2D, &sizeX, &sizeY, true, true))
                    {
                        float x = (screenPos2D.x / RsGlobal.maximumWidth) * 640.0f;
                        float y = (screenPos2D.y / RsGlobal.maximumHeight) * 448.0f;
                        sizeX = (sizeX / RsGlobal.maximumWidth) * 15.0f;
                        sizeY = (sizeY / RsGlobal.maximumHeight) * 15.0f;

                        string text = to_string(entity->m_nModelIndex);
                        DrawString(text, x, y, sizeX, sizeY);
                    }
                }
                entityList.clear();
            }
        };

        Events::initPoolsEvent += []
        {
            // Make ProcessLightsForEntity always be called
            injector::MakeNOP(0x535FC3, 2);
            injector::MakeNOP(0x535FC9, 2);
            patch::RedirectCall(0x535FCD, hookPreRenderEntiy);
        };
    }
} viewObjectsInfo;
