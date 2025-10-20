#include <gtest/gtest.h>
#include <memory>

// Include the QwBPMStripline class header
#include "QwBPMStripline.h"
#include "QwVQWK_Channel.h"

// Type alias for convenience
using QwBPMStriplineVQWK = QwBPMStripline<QwVQWK_Channel>;

//==============================================================================
// Test Fixture for QwBPMStripline tests
//==============================================================================

class QwBPMStriplineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create BPM instances for testing
        bpm1 = std::make_unique<QwBPMStriplineVQWK>("BPM1");
        bpm2 = std::make_unique<QwBPMStriplineVQWK>("BPM2");
        
        // Basic initialization - no complex setup needed for API compilation tests
    }

    std::unique_ptr<QwBPMStriplineVQWK> bpm1;
    std::unique_ptr<QwBPMStriplineVQWK> bpm2;
    
    static constexpr double EPSILON = 1e-9;
};

//==============================================================================
// Basic API Tests
//==============================================================================

TEST_F(QwBPMStriplineTest, BasicInstantiation) {
    // Test that QwBPMStripline can be instantiated
    EXPECT_TRUE(bpm1 != nullptr);
    EXPECT_TRUE(bpm2 != nullptr);
}

TEST_F(QwBPMStriplineTest, GetEffectiveCharge) {
    // Test basic effective charge access
    auto charge = bpm1->GetEffectiveCharge();
    EXPECT_TRUE(charge != nullptr);
}

TEST_F(QwBPMStriplineTest, ClearEventData) {
    // Test clearing event data
    EXPECT_NO_THROW(bpm1->ClearEventData());
    EXPECT_NO_THROW(bpm2->ClearEventData());
}

TEST_F(QwBPMStriplineTest, ProcessEvent) {
    // Test event processing
    EXPECT_NO_THROW(bpm1->ProcessEvent());
}

TEST_F(QwBPMStriplineTest, BPMArithmetic) {
    // Test basic arithmetic operations
    QwBPMStriplineVQWK result;
    
    // Test assignment
    EXPECT_NO_THROW(result = *bpm1);
    
    // Test addition
    EXPECT_NO_THROW(result += *bpm1);
    
    // Test subtraction  
    EXPECT_NO_THROW(result -= *bpm2);
}

TEST_F(QwBPMStriplineTest, CopyConstruction) {
    // Test copy construction
    EXPECT_NO_THROW({
        QwBPMStriplineVQWK copy(*bpm1);
        auto charge1 = copy.GetEffectiveCharge();
        auto charge2 = bpm1->GetEffectiveCharge();
        EXPECT_TRUE(charge1 != nullptr);
        EXPECT_TRUE(charge2 != nullptr);
    });
}

TEST_F(QwBPMStriplineTest, NameOperations) {
    // Test name-related operations
    EXPECT_NO_THROW(bpm1->GetElementName());
}

//==============================================================================
// Hardware Channel Interface Tests  
//==============================================================================

TEST_F(QwBPMStriplineTest, SetDefaultSampleSize) {
    // Test setting sample size
    EXPECT_NO_THROW(bpm1->SetDefaultSampleSize(10));
}

TEST_F(QwBPMStriplineTest, RandomEventMethods) {
    // Test random event parameter methods - BPMs need X and Y parameters
    EXPECT_NO_THROW(bpm1->SetRandomEventParameters(100.0, 5.0, 100.0, 5.0));
    EXPECT_NO_THROW(bpm1->SetRandomEventAsymmetry(0.1));
}

TEST_F(QwBPMStriplineTest, MockDataMethods) {
    // Test mock data related methods
    EXPECT_NO_THROW(bpm1->FillRawEventData());
    EXPECT_NO_THROW(bpm1->RandomizeEventData());
}

//==============================================================================
// BPM-Specific Operations Tests
//==============================================================================

TEST_F(QwBPMStriplineTest, EventCutMode) {
    // Test event cut mode setting
    EXPECT_NO_THROW(bpm1->SetEventCutMode(1));
}

TEST_F(QwBPMStriplineTest, SingleEventCuts) {
    // Test setting single event cuts - needs channel name as first parameter
    EXPECT_NO_THROW(bpm1->SetSingleEventCuts("XP", 0, 0.0, 1000.0, 0.1, 10.0));
}

//==============================================================================
// Complex Workflow Tests
//==============================================================================

TEST_F(QwBPMStriplineTest, FullProcessingWorkflow) {
    // Test a complete processing workflow
    bpm1->ClearEventData();
    bpm1->SetRandomEventParameters(100.0, 10.0, 100.0, 10.0);
    bpm1->FillRawEventData();
    bpm1->ProcessEvent();
    
    auto final_charge = bpm1->GetEffectiveCharge();
    EXPECT_TRUE(final_charge != nullptr);
}

TEST_F(QwBPMStriplineTest, ArithmeticWorkflow) {
    // Test arithmetic operations workflow
    QwBPMStriplineVQWK sum, diff;
    
    // Set up test data
    bpm1->SetRandomEventParameters(100.0, 5.0, 100.0, 5.0);
    bpm2->SetRandomEventParameters(200.0, 10.0, 200.0, 10.0);
    
    bpm1->FillRawEventData();
    bpm2->FillRawEventData();
    
    // Perform arithmetic
    sum = *bpm1;
    sum += *bpm2;
    
    diff = *bpm1;
    diff -= *bpm2;
    
    // Get results (should not crash)
    auto sum_charge = sum.GetEffectiveCharge();
    auto diff_charge = diff.GetEffectiveCharge();
    EXPECT_TRUE(sum_charge != nullptr);
    EXPECT_TRUE(diff_charge != nullptr);
}

//==============================================================================
// Stability and Edge Cases
//==============================================================================

TEST_F(QwBPMStriplineTest, MultipleEventProcessing) {
    // Test processing multiple events in sequence
    for (int i = 0; i < 10; ++i) {
        bpm1->ClearEventData();
        bpm1->SetRandomEventParameters(100.0 + i, 10.0, 100.0 + i, 10.0);
        bpm1->FillRawEventData();
        bpm1->ProcessEvent();
        
        auto charge = bpm1->GetEffectiveCharge();
        EXPECT_TRUE(charge != nullptr);
    }
}

TEST_F(QwBPMStriplineTest, ErrorHandling) {
    // Test that basic operations don't crash with default initialization
    auto charge = bpm1->GetEffectiveCharge();
    EXPECT_TRUE(charge != nullptr);
    
    // Test arithmetic doesn't crash
    QwBPMStriplineVQWK result;
    result = *bpm1;
    result += *bpm2;
    
    auto result_charge = result.GetEffectiveCharge();
    EXPECT_TRUE(result_charge != nullptr);
}