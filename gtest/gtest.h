#pragma once

namespace testing
{
    typedef void (*TestFn)(void);
    struct TestRegistrable {
        explicit TestRegistrable(TestFn testFuntionPtr, const char* name, const char* name2);
    };

    void _check(bool, const char* expression, const char* function);
}

#define TEST(name, name2) \
    void TESTCASE_NAME(name,name2)(void); \
    ::testing::TestRegistrable TestRegistrable_##name##_##name2(&TESTCASE_NAME(name,name2), #name, #name2); \
    void TESTCASE_NAME(name,name2)(void) // function body

#define TEST_DISABLED(name, name2) TEST(name, DISABLED_##name2)

#define TESTCASE_NAME(name, name2) testFunction_##name##_##name2

#define EXPECT_TRUE(a) ::testing::_check(a, #a, __FUNCTION__);
#define EXPECT_EQ(a, b) EXPECT_TRUE( (a) == (b) );

#define EXPECT_THROW(a, exception) \
    try { \
        a; \
        ::testing::_check(false, ""); \
    } \
    catch( exception ) {}

#define EXPECT_NOTHROW(a, exception) \
    try { \
        a; \
    } catch( exception ) { \
        ::testing::_check(false, ""); \
    }
