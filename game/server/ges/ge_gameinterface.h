/////////////  Copyright © 2006, Scott Loyd. All rights reserved.  /////////////
// 
// ge_gameinterface.h
//
// Description:
//      Exposes our accessor to MapEntityFilter()
//
// Created On: 8/21/2006 1:39:04 PM
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_GAMEINTERFACE_H
#define GE_GAMEINTERFACE_H
#ifdef _WIN32
#pragma once
#endif

#include "mapentities.h"
#include "gameinterface.h"

//Accessor
IMapEntityFilter *GEMapEntityFilter();

#endif //GE_GAMEINTERFACE_H
