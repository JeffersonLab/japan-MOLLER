/**
 * @file test_QwScaler_Channel.cpp
 * @brief Unit tests for QwScaler_Channel class
 * 
 * Tests the scaler channel implementation including counting operations,
 * rate calculations, and dual operator pattern compliance.
 */

#include <gtest/gtest.h>
#include <cmath>

#include "QwScaler_Channel.h"
#include "VQwHardwareChannel.h"

class QwScalerChannelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test channels using typedef'd type
        scaler1.InitializeChannel("test_scaler_1");
        scaler2.InitializeChannel("test_scaler_2");
    }
    
    // Use the typedef'd scaler channel type
    QwSIS3801_Channel scaler1, scaler2, result;
    static constexpr double EPSILON = 1e-10;
};

//==============================================================================
// Construction and Basic Properties Tests
//==============================================================================

TEST_F(QwScalerChannelTest, DefaultConstruction) {
    QwSIS3801_Channel scaler;
    EXPECT_STREQ(scaler.GetElementName().Data(), "");
}

TEST_F(QwScalerChannelTest, NamedConstruction) {
    QwSIS3801_Channel scaler("TestScaler");
    EXPECT_STREQ(scaler.GetElementName().Data(), "TestScaler");
}

TEST_F(QwScalerChannelTest, CopyConstruction) {
    scaler1.SetEventData(12345.0);
    QwSIS3801_Channel scaler_copy(scaler1);
    EXPECT_STREQ(scaler_copy.GetElementName().Data(), "test_scaler_1");
    EXPECT_NEAR(scaler_copy.GetValue(), 12345.0, EPSILON);
}

//==============================================================================
// Basic Value Operations Tests
//==============================================================================

TEST_F(QwScalerChannelTest, SetAndGetEventData) {
    scaler1.SetEventData(98765.0);
    EXPECT_NEAR(scaler1.GetValue(), 98765.0, EPSILON);
    
    scaler1.SetEventData(-12345.0);
    EXPECT_NEAR(scaler1.GetValue(), -12345.0, EPSILON);
}

TEST_F(QwScalerChannelTest, ClearEventData) {
    scaler1.SetEventData(12345.0);
    scaler1.ClearEventData();
    EXPECT_NEAR(scaler1.GetValue(), 0.0, EPSILON);
}

//==============================================================================
// Assignment and Arithmetic Operations Tests
//==============================================================================

TEST_F(QwScalerChannelTest, AssignmentOperator) {
    scaler1.SetEventData(100.0);
    scaler2.SetEventData(200.0);
    
    scaler2 = scaler1;
    EXPECT_NEAR(scaler2.GetValue(), 100.0, EPSILON);
}

TEST_F(QwScalerChannelTest, AdditionOperator) {
    scaler1.SetEventData(100.0);
    scaler2.SetEventData(50.0);
    
    scaler1 += scaler2;
    EXPECT_NEAR(scaler1.GetValue(), 150.0, EPSILON);
}

TEST_F(QwScalerChannelTest, SubtractionOperator) {
    scaler1.SetEventData(100.0);
    scaler2.SetEventData(30.0);
    
    scaler1 -= scaler2;
    EXPECT_NEAR(scaler1.GetValue(), 70.0, EPSILON);
}

TEST_F(QwScalerChannelTest, MultiplicationOperator) {
    scaler1.SetEventData(5.0);
    scaler2.SetEventData(3.0);
    
    scaler1 *= scaler2;
    EXPECT_NEAR(scaler1.GetValue(), 15.0, EPSILON);
}

//==============================================================================
// Sum and Difference Methods Tests
//==============================================================================

TEST_F(QwScalerChannelTest, SumMethod) {
    scaler1.SetEventData(25.0);
    scaler2.SetEventData(75.0);
    
    result.Sum(scaler1, scaler2);
    EXPECT_NEAR(result.GetValue(), 100.0, EPSILON);
}

TEST_F(QwScalerChannelTest, DifferenceMethod) {
    scaler1.SetEventData(80.0);
    scaler2.SetEventData(30.0);
    
    result.Difference(scaler1, scaler2);
    EXPECT_NEAR(result.GetValue(), 50.0, EPSILON);
}

//==============================================================================
// Scaling Operations Tests
//==============================================================================

TEST_F(QwScalerChannelTest, ScaleOperation) {
    scaler1.SetEventData(100.0);
    scaler1.Scale(1.5);
    EXPECT_NEAR(scaler1.GetValue(), 150.0, EPSILON);
    
    scaler1.Scale(0.5);
    EXPECT_NEAR(scaler1.GetValue(), 75.0, EPSILON);
}

TEST_F(QwScalerChannelTest, AddChannelOffset) {
    scaler1.SetEventData(100.0);
    scaler1.AddChannelOffset(50.0);
    EXPECT_NEAR(scaler1.GetValue(), 150.0, EPSILON);
    
    scaler1.AddChannelOffset(-25.0);
    EXPECT_NEAR(scaler1.GetValue(), 125.0, EPSILON);
}

//==============================================================================
// Division Operations Tests
//==============================================================================

TEST_F(QwScalerChannelTest, DivideByMethod) {
    scaler1.SetEventData(150.0);
    scaler2.SetEventData(3.0);
    
    scaler1.DivideBy(scaler2);
    EXPECT_NEAR(scaler1.GetValue(), 50.0, EPSILON);
}

TEST_F(QwScalerChannelTest, RatioMethod) {
    scaler1.SetEventData(120.0);
    scaler2.SetEventData(4.0);
    
    result.Ratio(scaler1, scaler2);
    EXPECT_NEAR(result.GetValue(), 30.0, EPSILON);
}

//==============================================================================
// Zero and Edge Cases Tests
//==============================================================================

TEST_F(QwScalerChannelTest, ZeroValueOperations) {
    scaler1.SetEventData(0.0);
    scaler2.SetEventData(42.0);
    
    result.Sum(scaler1, scaler2);
    EXPECT_NEAR(result.GetValue(), 42.0, EPSILON);
    
    result.Difference(scaler2, scaler1);
    EXPECT_NEAR(result.GetValue(), 42.0, EPSILON);
}

TEST_F(QwScalerChannelTest, NegativeValueOperations) {
    scaler1.SetEventData(-100.0);
    scaler2.SetEventData(50.0);
    
    result.Sum(scaler1, scaler2);
    EXPECT_NEAR(result.GetValue(), -50.0, EPSILON);
    
    result.Difference(scaler1, scaler2);
    EXPECT_NEAR(result.GetValue(), -150.0, EPSILON);
}

TEST_F(QwScalerChannelTest, LargeValueOperations) {
    scaler1.SetEventData(1e6);
    scaler2.SetEventData(2e6);
    
    result.Sum(scaler1, scaler2);
    EXPECT_NEAR(result.GetValue(), 3e6, 1e-3);
}