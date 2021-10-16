#include "ClientHook.h"

#include "../../Features/Misc/Misc.h"
#include "../../Features/Visuals/Visuals.h"
#include "../../Features/Menu/Menu.h"
#include "../../Features/AttributeChanger/AttributeChanger.h"

void __stdcall ClientHook::PreEntity::Hook(char const *szMapName)
{
	Table.Original<fn>(index)(g_Interfaces.Client, szMapName);
}

void __stdcall ClientHook::PostEntity::Hook()
{
	Table.Original<fn>(index)(g_Interfaces.Client);
	g_Interfaces.Engine->ClientCmd_Unrestricted(_("r_maxdlights 69420"));
	g_Interfaces.Engine->ClientCmd_Unrestricted(_("r_dynamic 1"));
	g_Visuals.ModulateWorld();
}

void __stdcall ClientHook::ShutDown::Hook()
{
	Table.Original<fn>(index)(g_Interfaces.Client);
	g_EntityCache.Clear();
	std::fill_n(g_GlobalInfo.ignoredPlayers, 128, 0); // We wouldn't want some wog to get ignored randomly!
}

void __stdcall ClientHook::FrameStageNotify::Hook(EClientFrameStage FrameStage)
{
	switch (FrameStage)
	{
		case EClientFrameStage::FRAME_RENDER_START:
		{
			g_GlobalInfo.m_vPunchAngles = Vec3();

			if (Vars::Visuals::RemovePunch.m_Var) {
				if (const auto& pLocal = g_EntityCache.m_pLocal) {
					g_GlobalInfo.m_vPunchAngles = pLocal->GetPunchAngles();	//Store punch angles to be compesnsated for in aim
					pLocal->ClearPunchAngle();								//Clear punch angles for visual no-recoil
				}
			}
			for (auto i = 1; i <= g_Interfaces.Engine->GetMaxClients(); i++)
			{
				CBaseEntity* entity = nullptr;
				PlayerInfo_t temp;

				if (!(entity = g_Interfaces.EntityList->GetClientEntity(i)))
					continue;

				if (entity->GetDormant())
					continue;

				if (!g_Interfaces.Engine->GetPlayerInfo(i, &temp))
					continue;

				if (!entity->GetLifeState() == LIFE_ALIVE)
					continue;

				Vector vX = entity->GetEyeAngles();
				auto* m_angEyeAnglesX = reinterpret_cast<float*>(reinterpret_cast<DWORD>(entity) + g_NetVars.get_offset("DT_TFPlayer", "tfnonlocaldata", "m_angEyeAngles[0]"));
				if (Vars::AntiHack::Resolver::PitchResolver.m_Var)
				{
					//pitch resolver 
					if (vX.x == 90) //Fake Up resolver
					{
						*m_angEyeAnglesX = -89;
					}

					if (vX.x == -90) //Fake Down resolver
					{
						*m_angEyeAnglesX = 89;
					}
				}
			}
			g_Visuals.ThirdPerson();
			g_Visuals.SkyboxChanger();

			break;
		}

		default: break;
	}
	
	Table.Original<fn>(index)(g_Interfaces.Client, FrameStage);

	switch (FrameStage)
	{
		case EClientFrameStage::FRAME_NET_UPDATE_START: {
			g_EntityCache.Clear();
			break;
		}

#ifdef DEVELOPER_BUILD
		case EClientFrameStage::FRAME_NET_UPDATE_POSTDATAUPDATE_START: {
			g_AttributeChanger.Run();
			break;
		}
#endif

		case EClientFrameStage::FRAME_NET_UPDATE_END: 
		{
			g_EntityCache.Fill();

			g_GlobalInfo.m_bLocalSpectated = false;

			if (const auto &pLocal = g_EntityCache.m_pLocal)
			{
				for (const auto &Teammate : g_EntityCache.GetGroup(EGroupType::PLAYERS_TEAMMATES))
				{
					if (Teammate->IsAlive() || g_EntityCache.Friends[Teammate->GetIndex()])
						continue;

					CBaseEntity *pObservedPlayer = g_Interfaces.EntityList->GetClientEntityFromHandle(Teammate->GetObserverTarget());

					if (pObservedPlayer == pLocal)
					{
						switch (Teammate->GetObserverMode()) {
							case OBS_MODE_FIRSTPERSON: break;
							case OBS_MODE_THIRDPERSON: break;
							default: continue;
						}

						g_GlobalInfo.m_bLocalSpectated = true;
						break;
					}
				}
			}

			break;
		}

		case EClientFrameStage::FRAME_RENDER_START: {
			if (!g_GlobalInfo.unloadWndProcHook) {
				g_Visuals.UpdateWorldModulation();
				g_Visuals.UpdateSkyModulation();
			}
			break;
		}

		default: break;
	}
}