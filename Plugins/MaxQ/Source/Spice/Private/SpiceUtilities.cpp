// Copyright 2021 Gamergenic.  See full copyright notice in Spice.h.
// Author: chucknoble@gamergenic.com | https://www.gamergenic.com
// 
// Project page:   https://www.gamergenic.com/project/maxq/
// Documentation:  https://maxq.gamergenic.com/
// GitHub:         https://github.com/Gamergenic1/MaxQ/ 

#include "SpiceUtilities.h"
#include "CoreMinimal.h"
#include "SpicePlatformDefs.h"

// Local #defines
// UE has build acceleration that concatenates multiple source files.
// A historical problem with that is #defines leaking from one cpp to the next.
// If these were moved to a .h file they couldn't be #undefined at the end.
// May need a little rewrite for any platforms that don't support stack allocations.
#define LONG_MESSAGE_MAX_LENGTH 1841

namespace SpiceUtilities
{
    const double pi     = (double)pi_c();
    const double twopi  = (double)twopi_c();
    const double dpr    = (double)dpr_c();
    const double rpd    = (double)rpd_c();
}

FString toPath(const FString& file)
{
    // BaseGame.ini [/Script/UnrealEd.ProjectPackagingSettings] DirectoriesToAlwaysStageAsNonUFS
    // is relative to the project's Content directory...  Copying kernel files, then, means the directory must be relative to the content directory
#if WITH_EDITOR
    if (file.StartsWith(TEXT("Content")))
    {
        UE_LOG(LogSpice, Error, TEXT("BREAKING SPICE BEHAVIOR CHANGE!!  Spice data must be in project /Content directory.  All paths are now relative to /Content"));
        UE_LOG(LogSpice, Error, TEXT("Is relative to Content directory: %s"), *file);
    }
#endif

    if(FPaths::IsRelative(file))
    {
        auto gameDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
        return FPaths::Combine(gameDir, file);
    }

    return file;
}

void CopyFrom(const SpicePlane& _plane, FSPlane& dest)
{
    SpiceDouble _planeNormal[3] = { 0, 0, 0 }, _planeConstant = 0;

    pl2nvc_c(&_plane, _planeNormal, &_planeConstant);

    dest.normal = FSDimensionlessVector(_planeNormal);
    dest.constant = _planeConstant;
}


void CopyTo(const FSPlane& src, SpicePlane& _plane)
{
    SpiceDouble _planeNormal[3], _planeConstant = (SpiceDouble)(src.constant.AsSpiceDouble());
    src.normal.CopyTo(_planeNormal);

    memset(&_plane, 0, sizeof(_plane));

    nvc2pl_c(_planeNormal, _planeConstant, &_plane);

    // Ensure a failure in nvc2pl is logged
    // ...but let the caller pick up the signal and determine what to do
    UnexpectedErrorCheck(false);
}


void CopyFrom(const SpiceEllipse& _ellipse, FSEllipse& dest)
{
    SpiceDouble _center[3] = { 0, 0, 0 }, _v_major[3] = { 0, 0, 0 }, _v_minor[3] = { 0, 0, 0 };

    el2cgv_c(&_ellipse, _center, _v_major, _v_minor);

    dest.center = FSDistanceVector(_center);
    dest.v_major = FSDistanceVector(_v_major);
    dest.v_minor = FSDistanceVector(_v_minor);
}


void CopyTo(const FSEllipse& src, SpiceEllipse& _ellipse)
{
    SpiceDouble _center[3], _v_major[3], _v_minor[3];

    src.center.CopyTo(_center);
    src.v_major.CopyTo(_v_major);
    src.v_minor.CopyTo(_v_minor);

    memset(&_ellipse, 0, sizeof(_ellipse));

    cgv2el_c(_center, _v_major, _v_minor, &_ellipse);

    // Ensure a failure in cgv2el is logged
    // ...but let the caller pick up the signal and determine what to do
    UnexpectedErrorCheck(false);
}


uint8 ErrorCheck(ES_ResultCode& ResultCode, FString& ErrorMessage, bool BeQuiet)
{
    uint8 failed = failed_c();

    if (!failed)
    {
        ResultCode = ES_ResultCode::Success;
        ErrorMessage.Empty();
    }
    else
    {
        ResultCode = ES_ResultCode::Error;
        char szBuffer[LONG_MESSAGE_MAX_LENGTH];

        szBuffer[0] = '\0';
        getmsg_c("LONG", sizeof(szBuffer), szBuffer);

        if (!SpiceStringLengthN(szBuffer, sizeof(szBuffer)))
        {
            szBuffer[0] = '\0';
            getmsg_c("SHORT", sizeof(szBuffer), szBuffer);
        }

        ErrorMessage = szBuffer;

        if (!BeQuiet)
        {
            UE_LOG(LogSpice, Warning, TEXT("USpice Runtime Error: %s"), *ErrorMessage);
        }

        reset_c();
    }

    return failed;
}


uint8 UnexpectedErrorCheck(bool bReset)
{
    uint8 failed = failed_c();

    if (failed)
    {
        char szBuffer[LONG_MESSAGE_MAX_LENGTH];

        szBuffer[0] = '\0';
        getmsg_c("LONG", sizeof(szBuffer), szBuffer);

        if (!SpiceStringLengthN(szBuffer, sizeof(szBuffer)))
        {
            szBuffer[0] = '\0';
            getmsg_c("SHORT", sizeof(szBuffer), szBuffer);
        }

        FString ErrorMessage = szBuffer;
        UE_LOG(LogSpice, Warning, TEXT("USpice Runtime Unexpected Error: %s"), *ErrorMessage);

        if (bReset)
        {
            reset_c();
        }
    }

    return failed;
}

// Don't leak #define's
// UE has Unity-build-acceleration which concatenates multiple source files
#undef LONG_MESSAGE_MAX_LENGTH
