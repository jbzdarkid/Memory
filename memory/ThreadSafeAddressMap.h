#pragma once

#include "HazardPointer.h"
#include <atomic>
#include <map>

// Thread-safe map (from Andrei Alexandrescu and Maged Michael)
// http://erdani.com/publications/cuj-2004-12.pdf

class ThreadSafeAddressMap {
public:
    void Set(uintptr_t key, uintptr_t value);
    uintptr_t Find(uintptr_t key) const;
    size_t Size() const;
    void Clear();
    
private:
    // Note: The HazardPointer must be retained until we're done acting on it.
    // Thus, we return a HazardPointer rather than a Map*, to force the
    // enclosing scope to keep the object alive. That said, this isn't really
    // a guarantee. Callers should still be careful.
    HazardPointer LoadMap() const;
};
