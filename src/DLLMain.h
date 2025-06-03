#pragma once
#pragma once

#include "MargretePlugin.h"

#define DLLEXPORT extern "C" __declspec(dllexport)

DLLEXPORT void WINAPI MargretePluginGetInfo(MP_PLUGININFO *info);
DLLEXPORT MpBoolean WINAPI MargretePluginCommandCreate(IMargretePluginCommand **ppobj);

class PluginCommand final : public IMargretePluginCommand {
public:
    MpInteger addRef() override {
        return ++m_refCount;
    }

    MpInteger release() override {
        --m_refCount;
        if (m_refCount <= 0) {
            delete this;
            return 0;
        }
        return m_refCount;
    }

    MpBoolean queryInterface(const MpGuid &iid, void **ppobj) override {
        if (!ppobj) {
            return MP_FALSE;
        }

        if (iid == IID_IMargretePluginBase || iid == IID_IMargretePluginCommand) {
            *ppobj = this;
            addRef();
            return MP_TRUE;
        }
        return MP_FALSE;
    }

    MpBoolean getCommandName(wchar_t *text, MpInteger textLength) const override;

    MpBoolean invoke(IMargretePluginContext *ctx) override;

private:
    ~PluginCommand() {
        PluginCommand::release();
    }

    MpInteger m_refCount = 0;
};
