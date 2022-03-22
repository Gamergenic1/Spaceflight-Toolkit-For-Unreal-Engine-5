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

TEST(FSDimensionlessVectorTest, DefaultConstruction_IsInitialized) {
    FSDimensionlessVector dimensionlessVector;

    EXPECT_EQ(dimensionlessVector.x, 0.);
    EXPECT_EQ(dimensionlessVector.y, 0.);
    EXPECT_EQ(dimensionlessVector.z, 0.);
}

