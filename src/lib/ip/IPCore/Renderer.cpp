//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <IPCore/Renderer.h>
#include <algorithm>

namespace IPCore
{
    using namespace std;

    namespace
    {
        vector<RendererBackend*>& backends()
        {
            static vector<RendererBackend*> b;
            return b;
        }
    } // namespace

    void registerRendererBackend(RendererBackend* backend)
    {
        if (!findRendererBackend(backend->name()))
            backends().push_back(backend);
    }

    RendererBackend* findRendererBackend(const string& name)
    {
        if (name.empty())
            return defaultRendererBackend();

        for (RendererBackend* b : backends())
            if (b->name() == name)
                return b;

        return nullptr;
    }

    RendererBackend* defaultRendererBackend() { return backends().empty() ? nullptr : backends().front(); }

    bool selectRendererBackend(const string& name)
    {
        RendererBackend* backend = findRendererBackend(name);
        RendererBackend* current = defaultRendererBackend();

        if (!backend)
            return false;
        if (backend == current)
            return true;
        if (current->initialized())
            return false;

        vector<RendererBackend*>& b = backends();
        b.erase(find(b.begin(), b.end(), backend));
        b.insert(b.begin(), backend);
        return true;
    }

    vector<string> availableRendererBackends()
    {
        vector<string> names;
        for (RendererBackend* b : backends())
            names.push_back(b->name());
        return names;
    }

    bool setRendererDebugOption(const string& flag)
    {
        bool known = false;

        for (RendererBackend* b : backends())
            known = b->setDebugOption(flag) || known;

        return known;
    }

} // namespace IPCore
