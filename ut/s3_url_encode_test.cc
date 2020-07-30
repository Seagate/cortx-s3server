#include "s3_url_encode.h"
#include "gtest/gtest.h"

TEST(S3urlencodeTest, EscapeChar) {
  std::string destination;
  escape_char(' ', destination);
  EXPECT_EQ("%20", destination);
}

TEST(S3urlencodeTest, IsEncodingNeeded) {
  EXPECT_TRUE(char_needs_url_encoding(' '));
  EXPECT_TRUE(char_needs_url_encoding('/'));
  EXPECT_FALSE(char_needs_url_encoding('A'));
}

TEST(S3urlencodeTest, urlEncode) {
  EXPECT_EQ("http%3A%2F%2Ftest%20this", url_encode("http://test this"));
  EXPECT_EQ("abcd", url_encode("abcd"));
}

TEST(S3urlencodeTest, InvalidUrlEncode) {
  EXPECT_EQ("", url_encode(NULL));
  EXPECT_EQ("", url_encode(""));
}
