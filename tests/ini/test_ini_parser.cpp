#include "../../src/lib/formats/ini/ini_parser.hpp"

#include <gtest/gtest.h>

using namespace ini;

class IniParserTest : public ::testing::Test {
protected:
  IniParser parser;
};

TEST_F(IniParserTest, ParsesEmptyString) {
  auto error = parser.parse("");
  EXPECT_FALSE(error.has_value());
}

TEST_F(IniParserTest, ParsesCommentsOnly) {
  auto error = parser.parse("; This is a comment\n; Another comment\n");
  EXPECT_FALSE(error.has_value());
}

TEST_F(IniParserTest, ParsesWhitespaceOnly) {
  auto error = parser.parse("   \n\t\n  \n");
  EXPECT_FALSE(error.has_value());
}

TEST_F(IniParserTest, SkipsUnknownBlocks) {
  auto error = parser.parse("UnknownBlock SomeName\n  Field = Value\nEnd\n");
  EXPECT_FALSE(error.has_value());
}

TEST_F(IniParserTest, InvokesRegisteredBlockHandler) {
  bool called = false;
  std::string capturedName;

  parser.registerBlock("TestBlock", [&](IniParser &p, const std::string &name) {
    called = true;
    capturedName = name;
    p.parseBlock({}, nullptr);
  });

  auto error = parser.parse("TestBlock MyInstance\nEnd\n");
  EXPECT_FALSE(error.has_value());
  EXPECT_TRUE(called);
  EXPECT_EQ(capturedName, "MyInstance");
}

TEST_F(IniParserTest, ParsesMultipleBlocks) {
  int count = 0;

  parser.registerBlock("Block", [&](IniParser &p, const std::string &) {
    ++count;
    p.parseBlock({}, nullptr);
  });

  auto error = parser.parse("Block First\nEnd\nBlock Second\nEnd\n");
  EXPECT_FALSE(error.has_value());
  EXPECT_EQ(count, 2);
}

TEST_F(IniParserTest, GetNextTokenSkipsEqualsSign) {
  parser.registerBlock("Test", [](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::string captured;
    std::vector<FieldEntry> fields = {
        {"Field", [&captured](IniParser &fp, void *) { captured = fp.parseAsciiString(); }},
    };
    p.parseBlock(fields, nullptr);
    EXPECT_EQ(captured, "Value");
  });

  parser.parse("Test Foo\n  Field = Value\nEnd\n");
}

TEST_F(IniParserTest, ParsesBoolYes) {
  bool result = false;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Flag", [&result](IniParser &fp, void *) { result = fp.parseBool(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Flag = Yes\nEnd\n");
  EXPECT_TRUE(result);
}

TEST_F(IniParserTest, ParsesBoolNo) {
  bool result = true;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Flag", [&result](IniParser &fp, void *) { result = fp.parseBool(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Flag = No\nEnd\n");
  EXPECT_FALSE(result);
}

TEST_F(IniParserTest, ParsesInt) {
  int32_t result = 0;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Count", [&result](IniParser &fp, void *) { result = fp.parseInt(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Count = 42\nEnd\n");
  EXPECT_EQ(result, 42);
}

TEST_F(IniParserTest, ParsesReal) {
  float result = 0.0f;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Value", [&result](IniParser &fp, void *) { result = fp.parseReal(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Value = 3.14\nEnd\n");
  EXPECT_FLOAT_EQ(result, 3.14f);
}

TEST_F(IniParserTest, ParsesRGBColor) {
  RGBColor result;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Color", [&result](IniParser &fp, void *) { result = fp.parseRGBColor(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Color = R:0.5 G:0.75 B:1.0\nEnd\n");
  EXPECT_FLOAT_EQ(result.r, 0.5f);
  EXPECT_FLOAT_EQ(result.g, 0.75f);
  EXPECT_FLOAT_EQ(result.b, 1.0f);
}

TEST_F(IniParserTest, ParsesRGBAColorInt) {
  RGBAColorInt result;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Color", [&result](IniParser &fp, void *) { result = fp.parseRGBAColorInt(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Color = R:128 G:64 B:32 A:200\nEnd\n");
  EXPECT_EQ(result.r, 128);
  EXPECT_EQ(result.g, 64);
  EXPECT_EQ(result.b, 32);
  EXPECT_EQ(result.a, 200);
}

TEST_F(IniParserTest, ParsesIndexList) {
  int32_t result = -1;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Type",
         [&result](IniParser &fp, void *) {
           result = fp.parseIndexList({"NONE", "FIRST", "SECOND", "THIRD"});
         }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Type = SECOND\nEnd\n");
  EXPECT_EQ(result, 2);
}

TEST_F(IniParserTest, HandlesWindowsLineEndings) {
  bool called = false;
  parser.registerBlock("Test", [&called](IniParser &p, const std::string &) {
    called = true;
    p.parseBlock({}, nullptr);
  });

  parser.parse("Test Foo\r\nEnd\r\n");
  EXPECT_TRUE(called);
}

TEST_F(IniParserTest, HandlesMixedLineEndings) {
  int32_t result = 0;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Val", [&result](IniParser &fp, void *) { result = fp.parseInt(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\r\n  Val = 99\r\nEnd\r\n");
  EXPECT_EQ(result, 99);
}

TEST_F(IniParserTest, IgnoresInlineComments) {
  int32_t result = 0;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Val", [&result](IniParser &fp, void *) { result = fp.parseInt(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Val = 50 ; this is a comment\nEnd\n");
  EXPECT_EQ(result, 50);
}

TEST_F(IniParserTest, SkipsUnknownFieldsInsideBlock) {
  int32_t result = 0;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Known", [&result](IniParser &fp, void *) { result = fp.parseInt(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Unknown = blah\n  Known = 7\n  AlsoUnknown = stuff\nEnd\n");
  EXPECT_EQ(result, 7);
}

TEST_F(IniParserTest, EndIsCaseInsensitive) {
  bool called = false;
  parser.registerBlock("Test", [&called](IniParser &p, const std::string &) {
    called = true;
    p.parseBlock({}, nullptr);
  });

  parser.parse("Test Foo\nEND\n");
  EXPECT_TRUE(called);
}

TEST_F(IniParserTest, EndMixedCase) {
  bool called = false;
  parser.registerBlock("Test", [&called](IniParser &p, const std::string &) {
    called = true;
    p.parseBlock({}, nullptr);
  });

  parser.parse("Test Foo\neNd\n");
  EXPECT_TRUE(called);
}

TEST_F(IniParserTest, ParsesBlockWithNoName) {
  bool called = false;
  std::string capturedName;
  parser.registerBlock("Singleton", [&](IniParser &p, const std::string &name) {
    called = true;
    capturedName = name;
    p.parseBlock({}, nullptr);
  });

  parser.parse("Singleton\n  ; no name on block line\nEnd\n");
  EXPECT_TRUE(called);
  EXPECT_TRUE(capturedName.empty());
}

TEST_F(IniParserTest, ParsesNegativeInt) {
  int32_t result = 0;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Val", [&result](IniParser &fp, void *) { result = fp.parseInt(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Val = -10\nEnd\n");
  EXPECT_EQ(result, -10);
}

TEST_F(IniParserTest, ParsesNegativeReal) {
  float result = 0.0f;
  parser.registerBlock("Test", [&result](IniParser &p, const std::string &) {
    using FieldEntry = std::pair<std::string, std::function<void(IniParser &, void *)>>;
    std::vector<FieldEntry> fields = {
        {"Val", [&result](IniParser &fp, void *) { result = fp.parseReal(); }},
    };
    p.parseBlock(fields, nullptr);
  });

  parser.parse("Test X\n  Val = -2.5\nEnd\n");
  EXPECT_FLOAT_EQ(result, -2.5f);
}
