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
    Int_t run_num = buffer->GetRunNumber();
    EXPECT_GE(run_num, 0);
}
