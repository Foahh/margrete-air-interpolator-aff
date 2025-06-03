#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "Dialog.h"
#include "Interpolator.h"

TEST_CASE("Dialog") {
    ConfigContext cctx;
    const auto p = std::make_unique<Dialog>(nullptr, cctx);
    REQUIRE(p->ShowDialog() == S_OK);
}
