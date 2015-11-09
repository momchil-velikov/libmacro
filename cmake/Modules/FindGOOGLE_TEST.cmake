find_path(GOOGLE_TEST_DIR
  NAMES
    include/gtest/gtest.h
  HINTS
    $ENV{HOME}/local/gtest $ENV{HOME}/opt/gtest /usr/local/gtest /opt/gtest
)
