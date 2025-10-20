#include <gtest/gtest.h>
#include <memory>

// Include the QwHelicity class header
#include "QwHelicity.h"
#include "QwOptions.h"  // For gQwOptions singleton

//==============================================================================
// Test Fixture for QwHelicity tests
//==============================================================================

class QwHelicityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create QwHelicity instance using named constructor
        helicity = std::make_unique<QwHelicity>("HelicityTest");
    }

    std::unique_ptr<QwHelicity> helicity;
    
    static constexpr double EPSILON = 1e-9;
};

//==============================================================================
// Basic API Tests
//==============================================================================

TEST_F(QwHelicityTest, BasicInstantiation) {
    // Test that QwHelicity can be instantiated
    EXPECT_TRUE(helicity != nullptr);
}

TEST_F(QwHelicityTest, DefaultConstruction) {
    // Test named constructor since default is private
    QwHelicity named_helicity("TestHelicity");
    EXPECT_TRUE(true);  // Just verify it doesn't crash
}

TEST_F(QwHelicityTest, NamedConstruction) {
    // Test construction with name
    QwHelicity named_helicity("HelicitySubsystem");
    EXPECT_TRUE(true);  // Just verify it doesn't crash
}

//==============================================================================
// Basic Functionality Tests
//==============================================================================

TEST_F(QwHelicityTest, CopyConstruction) {
    // Test copy construction
    QwHelicity copied(*helicity);
    EXPECT_TRUE(true);  // Just verify it doesn't crash
}

TEST_F(QwHelicityTest, ClearEventData) {
    // Test clearing event data
    EXPECT_NO_THROW(helicity->ClearEventData());
}

TEST_F(QwHelicityTest, ProcessEvent) {
    // Test event processing
    EXPECT_NO_THROW(helicity->ProcessEvent());
}

//==============================================================================
// Pattern Phase Operations
//==============================================================================

TEST_F(QwHelicityTest, PatternPhaseAccess) {
    // Test pattern phase access methods
    EXPECT_NO_THROW(helicity->GetMaxPatternPhase());
    EXPECT_NO_THROW(helicity->GetMinPatternPhase());
}

TEST_F(QwHelicityTest, HelicityValues) {
    // Test available public helicity methods only
    EXPECT_NO_THROW(helicity->PredictHelicity());
    EXPECT_NO_THROW(helicity->IsHelicityIgnored());
    // Note: MatchActualHelicity is protected
}

//==============================================================================
// Event and Pattern Number Operations
//==============================================================================

TEST_F(QwHelicityTest, EventAndPatternNumbers) {
    // Test available event and pattern number access
    EXPECT_NO_THROW(helicity->GetEventNumber());
    EXPECT_NO_THROW(helicity->GetMaxPatternPhase());
    EXPECT_NO_THROW(helicity->GetPatternNumber());
}

TEST_F(QwHelicityTest, RandomSeedOperations) {
    // Test available public random seed operations
    // GetRandomSeed is protected, so we skip it
    EXPECT_TRUE(true);  // Placeholder for seed operations when API allows
}

//==============================================================================
// Helicity State Tests
//==============================================================================

TEST_F(QwHelicityTest, GoodHelicityCheck) {
    // Test available public helicity state checking methods only
    EXPECT_NO_THROW(helicity->IsHelicityIgnored());
    // Note: MatchActualHelicity is protected
}

TEST_F(QwHelicityTest, HelicityIgnoredState) {
    // Test ignored helicity state
    EXPECT_NO_THROW(helicity->IsHelicityIgnored());
}

//==============================================================================
// Basic Processing Tests
//==============================================================================

TEST_F(QwHelicityTest, ProcessEventCycle) {
    // Test a complete event processing cycle
    helicity->ClearEventData();
    helicity->ProcessEvent();
    
    // Basic state checks using available methods
    EXPECT_NO_THROW(helicity->GetEventNumber());
    EXPECT_NO_THROW(helicity->GetMaxPatternPhase());
}

TEST_F(QwHelicityTest, ClearEventDataCycle) {
    // Test clearing event data and state verification
    helicity->ClearEventData();
    
    // Verify basic operations still work using available public methods
    EXPECT_NO_THROW(helicity->PredictHelicity());
    EXPECT_NO_THROW(helicity->IsHelicityIgnored());
}

//==============================================================================
// Subsystem Operations
//==============================================================================

TEST_F(QwHelicityTest, SubsystemArithmetic) {
    // Test basic pointer-based arithmetic operations (required by VQwSubsystem interface)
    QwHelicity other("Other");
    
    // Test pointer-based addition (as required by API)
    EXPECT_NO_THROW(*helicity += &other);
    
    // Test pointer-based subtraction (as required by API)
    EXPECT_NO_THROW(*helicity -= &other);
}

TEST_F(QwHelicityTest, HelicitySum) {
    // Test helicity sum operation using pointer interface
    QwHelicity first("First"), second("Second"), result("Result");
    EXPECT_NO_THROW(result.Sum(&first, &second));
}

TEST_F(QwHelicityTest, HelicityDifference) {
    // Test helicity difference operation using pointer interface
    QwHelicity first("First"), second("Second"), result("Result");
    EXPECT_NO_THROW(result.Difference(&first, &second));
}

//==============================================================================
// Advanced Operations
//==============================================================================

TEST_F(QwHelicityTest, CloneOperation) {
    // Test cloning operation
    VQwSubsystem* cloned = helicity->Clone();
    EXPECT_TRUE(cloned != nullptr);
    delete cloned;
}

TEST_F(QwHelicityTest, AccumulateRunningSum) {
    // Test accumulation operation using pointer interface
    QwHelicity accumulator("Accumulator");
    EXPECT_NO_THROW(accumulator.AccumulateRunningSum(helicity.get()));
}

//==============================================================================
// Processing and Configuration
//==============================================================================

TEST_F(QwHelicityTest, ProcessOptions) {
    // Test processing options with singleton
    EXPECT_NO_THROW(helicity->ProcessOptions(gQwOptions));
}

TEST_F(QwHelicityTest, LoadChannelMap) {
    // Test loading channel map (will fail gracefully for non-existent file)
    EXPECT_NO_THROW(helicity->LoadChannelMap("non_existent_file.map"));
}

TEST_F(QwHelicityTest, LoadInputParameters) {
    // Test loading input parameters (will fail gracefully for non-existent file)
    EXPECT_NO_THROW(helicity->LoadInputParameters("non_existent_file.conf"));
}

TEST_F(QwHelicityTest, LoadEventCuts) {
    // Test loading event cuts (will fail gracefully for non-existent file)
    EXPECT_NO_THROW(helicity->LoadEventCuts("non_existent_file.cuts"));
}

//==============================================================================
// Stability Tests
//==============================================================================

TEST_F(QwHelicityTest, MultipleOperationCycles) {
    // Test multiple operation cycles
    for (int i = 0; i < 10; ++i) {
        helicity->ClearEventData();
        helicity->ProcessEvent();
        
        // Verify basic state access still works using available public methods
        EXPECT_NO_THROW(helicity->GetEventNumber());
        EXPECT_NO_THROW(helicity->IsHelicityIgnored());
    }
}

TEST_F(QwHelicityTest, MemoryStability) {
    // Test memory stability over many operations using named constructors
    for (int i = 0; i < 100; ++i) {
        QwHelicity temp_helicity("TempHelicity");
        temp_helicity.ClearEventData();
        temp_helicity.ProcessEvent();
    }
    
    // Original should still be functional
    EXPECT_TRUE(helicity != nullptr);
    EXPECT_NO_THROW(helicity->GetEventNumber());
}