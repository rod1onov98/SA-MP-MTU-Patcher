#include <iostream>
#include "minhook/minhook.h"
#include <string>
#include <cstdint>
#include <cstdio>

namespace Samp {
    const DWORD SAMP_DLL = (DWORD)GetModuleHandleA("samp.dll");

    namespace RakPeer {
        const DWORD SetMTUSize = 0x16E0;
        const DWORD GetMTUSize = 0x16B0;
    }
}

typedef void(__thiscall* CRakPeer_SetMTUSize_t)(void* pThis, int mtuSize);
typedef int(__thiscall* CRakPeer_GetMTUSize_t)(void* pThis);

CRakPeer_SetMTUSize_t Original_CRakPeer_SetMTUSize = nullptr;
CRakPeer_GetMTUSize_t Original_CRakPeer_GetMTUSize = nullptr;

int g_currentMTU = 0;
const int TARGET_MTU = 590; // i use 590, you can change it

void __fastcall Hooked_CRakPeer_SetMTUSize(void* pThis, void* edx, int mtuSize) {

    g_currentMTU = TARGET_MTU;

    Original_CRakPeer_SetMTUSize(pThis, TARGET_MTU);
}

int __fastcall Hooked_CRakPeer_GetMTUSize(void* pThis, void* edx) {

    int originalMTU = Original_CRakPeer_GetMTUSize(pThis);

    if (g_currentMTU > 0) {
        return g_currentMTU;
    }

    return originalMTU;
}

void* rakpeerptr() {
    DWORD* samp_info = (DWORD*)(Samp::SAMP_DLL + 0x21A0E8);
    if (!samp_info || !*samp_info) {
        return nullptr;
    }

    void* rak_peer = *(void**)(*samp_info + 0x3C5);
    return rak_peer;
}

void forcemtupatch()
{

    void* rak_peer = rakpeerptr();
    if (!rak_peer) {
        return;
    }

    int current_mtu = Original_CRakPeer_GetMTUSize(rak_peer);

    g_currentMTU = TARGET_MTU;
    Original_CRakPeer_SetMTUSize(rak_peer, TARGET_MTU);

    int new_mtu = Original_CRakPeer_GetMTUSize(rak_peer);

    if (new_mtu == TARGET_MTU) {
    }
    else {
    }
}

bool InstallHooks() {

    if (!Samp::SAMP_DLL) {
        return false;
    }

    if (MH_Initialize() != MH_OK) {
        return false;
    }

    DWORD set_mtu_addr = Samp::SAMP_DLL + Samp::RakPeer::SetMTUSize;

    BYTE* code = (BYTE*)set_mtu_addr;

    if (MH_CreateHook((LPVOID)set_mtu_addr, &Hooked_CRakPeer_SetMTUSize, (LPVOID*)&Original_CRakPeer_SetMTUSize) != MH_OK) {
        return false;
    }

    DWORD get_mtu_addr = Samp::SAMP_DLL + Samp::RakPeer::GetMTUSize;

    code = (BYTE*)get_mtu_addr;

    if (MH_CreateHook((LPVOID)get_mtu_addr, &Hooked_CRakPeer_GetMTUSize, (LPVOID*)&Original_CRakPeer_GetMTUSize) != MH_OK) {
        return false;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        return false;
    }

    return true;
}

DWORD WINAPI MonitorThread(LPVOID lpParam) {

    Sleep(5000);

    void* rak_peer = rakpeerptr();
    if (rak_peer) {
        int current_mtu = Original_CRakPeer_GetMTUSize(rak_peer);

        if (current_mtu != TARGET_MTU) {
            forcemtupatch();
        }
    }

    for (int i = 0; i < 10; i++) {
        Sleep(1000);
        if (rak_peer) {
            int current_mtu = Original_CRakPeer_GetMTUSize(rak_peer);
            if (current_mtu != TARGET_MTU && g_currentMTU == TARGET_MTU) {
            }
        }
    }
    return 0;
}

DWORD WINAPI InitThread(LPVOID lpParam) {

    Sleep(10000);

    if (InstallHooks()) {

        CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);

        Sleep(2000);
        forcemtupatch();
    }
    else {
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    return TRUE;

}
