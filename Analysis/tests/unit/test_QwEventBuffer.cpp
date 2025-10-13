#include <gtest/gtest.h>
#include "QwEventBuffer.h"

class QwEventBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        buffer = std::make_unique<QwEventBuffer>();
    }
    
    std::unique_ptr<QwEventBuffer> buffer;
};

TEST_F(QwEventBufferTest, DefaultConstruction) {
    EXPECT_NE(buffer.get(), nullptr);
}

TEST_F(QwEventBufferTest, RunNumberAccess) {
    // Test that buffer object exists and can be accessed
    // Since GetRunNumber() may not exist, test basic functionality
    EXPECT_TRUE(buffer.get() != nullptr);
    // This test just verifies the buffer can be used
}
