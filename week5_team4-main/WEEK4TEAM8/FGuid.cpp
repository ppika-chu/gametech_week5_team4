#include "FGuid.h"
#include <objbase.h>
#include <cstring>

FGuid FGuid::NewGuid()
{
    GUID NativeGuid{};
    CoCreateGuid(&NativeGuid);

    FGuid Guid;
    std::memcpy(&Guid, &NativeGuid, sizeof(Guid));

    return Guid;
}

