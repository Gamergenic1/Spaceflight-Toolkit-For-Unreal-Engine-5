// Copyright 2021 Gamergenic.  See full copyright notice in Spice.h.
// Author: chucknoble@gamergenic.com | https://www.gamergenic.com
// 
// Project page:   https://www.gamergenic.com/project/maxq/
// Documentation:  https://maxq.gamergenic.com/
// GitHub:         https://github.com/Gamergenic1/MaxQ/ 

#include "UE5HostDefs.h"
#include "SpiceHostDefs.h"

#include "Spice.h"
#include "SpiceTypes.h"

#include "gtest/gtest.h"

TEST(FSPlanetographicStateVectorTest, DefaultConstruction_IsInitialized) {
    FSPlanetographicStateVector planetographicStateVector;

    EXPECT_EQ(planetographicStateVector.r.lonlat.longitude.degrees, 0.);
    EXPECT_EQ(planetographicStateVector.r.lonlat.latitude.degrees, 0.);
    EXPECT_EQ(planetographicStateVector.r.alt.km, 0.);
    EXPECT_EQ(planetographicStateVector.dr.dlon.radiansPerSecond, 0.);
    EXPECT_EQ(planetographicStateVector.dr.dlat.radiansPerSecond, 0.);
    EXPECT_EQ(planetographicStateVector.dr.dalt.kmps, 0.);
}

