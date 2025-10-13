/**********************************************************\
* File: test_QwPMT_Channel.cpp                            *
*                                                          *
* Unit tests for QwPMT_Channel class                      *
\**********************************************************/

#include <gtest/gtest.h>
#include "QwPMT_Channel.h"

class QwPMTChannelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test objects
        pmt1.InitializeChannel("TestPMT1");
        pmt2.InitializeChannel("TestPMT2");
    }

    void TearDown() override {
        // Clean up after tests
    }

    QwPMT_Channel pmt1;
    QwPMT_Channel pmt2;
    static constexpr double EPSILON = 1e-6;
};

// Test basic construction and initialization
TEST_F(QwPMTChannelTest, BasicConstruction) {
    QwPMT_Channel pmt;
    EXPECT_STREQ(pmt.GetElementName().Data(), "");
    
    QwPMT_Channel pmt_named("TestChannel");
    EXPECT_STREQ(pmt_named.GetElementName().Data(), "TestChannel");
}

// Test value setting and getting
TEST_F(QwPMTChannelTest, ValueOperations) {
    pmt1.SetValue(123.45);
    EXPECT_NEAR(pmt1.GetValue(), 123.45, EPSILON);
    
    pmt1.SetValue(-67.89);
    EXPECT_NEAR(pmt1.GetValue(), -67.89, EPSILON);
}

// Test module and subbank ID operations
TEST_F(QwPMTChannelTest, ModuleOperations) {
    pmt1.SetModule(5);
    EXPECT_EQ(pmt1.GetModule(), 5);
    
    pmt1.SetSubbankID(12);
    EXPECT_EQ(pmt1.GetSubbankID(), 12);
}

// Test copy constructor
TEST_F(QwPMTChannelTest, CopyConstructor) {
    pmt1.SetValue(314.159);
    pmt1.SetModule(7);
    pmt1.SetSubbankID(3);
    
    QwPMT_Channel pmt_copy(pmt1);
    EXPECT_NEAR(pmt_copy.GetValue(), 314.159, EPSILON);
    EXPECT_EQ(pmt_copy.GetModule(), 7);
    EXPECT_EQ(pmt_copy.GetSubbankID(), 3);
}

// Test assignment operator
TEST_F(QwPMTChannelTest, AssignmentOperator) {
    pmt1.SetValue(100.0);
    pmt2.SetValue(200.0);
    
    pmt2 = pmt1;
    EXPECT_NEAR(pmt2.GetValue(), 100.0, EPSILON);
}

// Test addition operator
TEST_F(QwPMTChannelTest, AdditionOperator) {
    pmt1.SetValue(100.0);
    pmt2.SetValue(50.0);
    
    pmt1 += pmt2;
    EXPECT_NEAR(pmt1.GetValue(), 150.0, EPSILON);
}

// Test subtraction operator
TEST_F(QwPMTChannelTest, SubtractionOperator) {
    pmt1.SetValue(100.0);
    pmt2.SetValue(30.0);
    
    pmt1 -= pmt2;
    EXPECT_NEAR(pmt1.GetValue(), 70.0, EPSILON);
}

// Test Sum method
TEST_F(QwPMTChannelTest, SumMethod) {
    QwPMT_Channel pmt3;
    pmt1.SetValue(25.0);
    pmt2.SetValue(75.0);
    
    pmt3.Sum(pmt1, pmt2);
    EXPECT_NEAR(pmt3.GetValue(), 100.0, EPSILON);
}

// Test Difference method
TEST_F(QwPMTChannelTest, DifferenceMethod) {
    QwPMT_Channel pmt3;
    pmt1.SetValue(80.0);
    pmt2.SetValue(30.0);
    
    pmt3.Difference(pmt1, pmt2);
    EXPECT_NEAR(pmt3.GetValue(), 50.0, EPSILON);
}

// Test ClearEventData
TEST_F(QwPMTChannelTest, ClearEventData) {
    pmt1.SetValue(123.45);
    pmt1.ClearEventData();
    // Value should be reset after clearing
    EXPECT_NEAR(pmt1.GetValue(), 0.0, EPSILON);
}

// Test with zero values
TEST_F(QwPMTChannelTest, ZeroValue) {
    pmt1.SetValue(0.0);
    EXPECT_NEAR(pmt1.GetValue(), 0.0, EPSILON);
    
    pmt2.SetValue(42.0);
    pmt1 += pmt2;
    EXPECT_NEAR(pmt1.GetValue(), 42.0, EPSILON);
}

// Test with negative values
TEST_F(QwPMTChannelTest, NegativeValues) {
    pmt1.SetValue(-123.45);
    pmt2.SetValue(-67.89);
    
    pmt1 += pmt2;
    EXPECT_NEAR(pmt1.GetValue(), -191.34, EPSILON);
}

// Test large values
TEST_F(QwPMTChannelTest, LargeValues) {
    pmt1.SetValue(1e6);
    pmt2.SetValue(2e6);
    
    pmt1 += pmt2;
    EXPECT_NEAR(pmt1.GetValue(), 3e6, 1e-3);
}