//
//  CrossPlatform.cpp
//  quadro
//
//  Created by EAA van Poeijer on 24-01-13.
//
//

#include "CrossPlatform.h"
#include "UIDevice+IdentifierAddition.h"

namespace CrossPlatform
{
    const char* Identifiers::uniqueID()
{
        UIDevice* uniqueId = [UIDevice alloc];
        return [[uniqueId uniqueGlobalDeviceIdentifier] cStringUsingEncoding:NSUTF8StringEncoding];
}
    
}