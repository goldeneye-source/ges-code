///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ent_hat.h
//   Description :
//      [TODO: Write the purpose of ent_hat.h.]
//
//   Created On: 12/28/2008 5:10:19 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_ENT_HAT_H
#define MC_ENT_HAT_H
#ifdef _WIN32
#pragma once
#endif

CBaseEntity *CreateNewHat( const Vector &position, const QAngle &angles, const char* hatModel);

#endif //MC_ENT_HAT_H
