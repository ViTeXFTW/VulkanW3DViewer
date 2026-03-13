#include "../../src/lib/formats/ini/water_settings.hpp"

#include <gtest/gtest.h>

using namespace ini;

class WaterSettingsTest : public ::testing::Test {
protected:
  WaterSettings settings;
};

TEST_F(WaterSettingsTest, DefaultTransparencyValues) {
  EXPECT_FLOAT_EQ(settings.transparency.transparentWaterDepth, 3.0f);
  EXPECT_FLOAT_EQ(settings.transparency.minWaterOpacity, 1.0f);
  EXPECT_FLOAT_EQ(settings.transparency.standingWaterColor.r, 1.0f);
  EXPECT_FLOAT_EQ(settings.transparency.standingWaterColor.g, 1.0f);
  EXPECT_FLOAT_EQ(settings.transparency.standingWaterColor.b, 1.0f);
  EXPECT_FLOAT_EQ(settings.transparency.radarColor.r, 0.55f);
  EXPECT_FLOAT_EQ(settings.transparency.radarColor.g, 0.55f);
  EXPECT_FLOAT_EQ(settings.transparency.radarColor.b, 1.0f);
  EXPECT_FALSE(settings.transparency.additiveBlend);
  EXPECT_EQ(settings.transparency.standingWaterTexture, "TWWater01.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureN, "TSMorningN.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureE, "TSMorningE.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureS, "TSMorningS.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureW, "TSMorningW.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureT, "TSMorningT.tga");
}

TEST_F(WaterSettingsTest, ParsesWaterSetMorning) {
  const char *ini = R"(
WaterSet Morning
  SkyTexture = TSSkyMorning.tga
  WaterTexture = TWWater01.tga
  Vertex00Color = R:255 G:200 B:180 A:255
  Vertex10Color = R:255 G:200 B:180 A:255
  Vertex01Color = R:255 G:200 B:180 A:255
  Vertex11Color = R:255 G:200 B:180 A:255
  DiffuseColor = R:128 G:128 B:128 A:255
  TransparentDiffuseColor = R:64 G:64 B:64 A:128
  UScrollPerMS = 0.001
  VScrollPerMS = 0.002
  SkyTexelsPerUnit = 0.01
  WaterRepeatCount = 10
End
)";

  settings.loadFromINI(ini);

  const auto &morning = settings.perTimeOfDay[static_cast<int32_t>(TimeOfDay::Morning)];
  EXPECT_EQ(morning.skyTextureFile, "TSSkyMorning.tga");
  EXPECT_EQ(morning.waterTextureFile, "TWWater01.tga");
  EXPECT_EQ(morning.vertex00Diffuse.r, 255);
  EXPECT_EQ(morning.vertex00Diffuse.g, 200);
  EXPECT_EQ(morning.vertex00Diffuse.b, 180);
  EXPECT_EQ(morning.vertex00Diffuse.a, 255);
  EXPECT_EQ(morning.waterDiffuseColor.r, 128);
  EXPECT_EQ(morning.transparentWaterDiffuse.a, 128);
  EXPECT_FLOAT_EQ(morning.uScrollPerMs, 0.001f);
  EXPECT_FLOAT_EQ(morning.vScrollPerMs, 0.002f);
  EXPECT_FLOAT_EQ(morning.skyTexelsPerUnit, 0.01f);
  EXPECT_EQ(morning.waterRepeatCount, 10);
}

TEST_F(WaterSettingsTest, ParsesMultipleTimeOfDay) {
  const char *ini = R"(
WaterSet Morning
  SkyTexture = TSSkyMorning.tga
  WaterTexture = TWWater01.tga
  UScrollPerMS = 0.001
  VScrollPerMS = 0.002
End

WaterSet Afternoon
  SkyTexture = TSSkyAfternoon.tga
  WaterTexture = TWWater02.tga
  UScrollPerMS = 0.003
  VScrollPerMS = 0.004
End

WaterSet Evening
  SkyTexture = TSSkyEvening.tga
  WaterTexture = TWWater03.tga
  UScrollPerMS = 0.005
  VScrollPerMS = 0.006
End

WaterSet Night
  SkyTexture = TSSkyNight.tga
  WaterTexture = TWWater04.tga
  UScrollPerMS = 0.007
  VScrollPerMS = 0.008
End
)";

  settings.loadFromINI(ini);

  const auto &morning = settings.getForTimeOfDay(TimeOfDay::Morning);
  EXPECT_EQ(morning.skyTextureFile, "TSSkyMorning.tga");
  EXPECT_FLOAT_EQ(morning.uScrollPerMs, 0.001f);

  const auto &afternoon = settings.getForTimeOfDay(TimeOfDay::Afternoon);
  EXPECT_EQ(afternoon.skyTextureFile, "TSSkyAfternoon.tga");
  EXPECT_FLOAT_EQ(afternoon.uScrollPerMs, 0.003f);

  const auto &evening = settings.getForTimeOfDay(TimeOfDay::Evening);
  EXPECT_EQ(evening.skyTextureFile, "TSSkyEvening.tga");
  EXPECT_FLOAT_EQ(evening.uScrollPerMs, 0.005f);

  const auto &night = settings.getForTimeOfDay(TimeOfDay::Night);
  EXPECT_EQ(night.skyTextureFile, "TSSkyNight.tga");
  EXPECT_FLOAT_EQ(night.uScrollPerMs, 0.007f);
}

TEST_F(WaterSettingsTest, ParsesWaterTransparency) {
  const char *ini = R"(
WaterTransparency
  TransparentWaterDepth = 5.0
  TransparentWaterMinOpacity = 0.5
  StandingWaterColor = R:0.2 G:0.3 B:0.8
  StandingWaterTexture = TWCustomWater.tga
  AdditiveBlending = Yes
  RadarWaterColor = R:0.0 G:0.0 B:1.0
  SkyboxTextureN = TSCustomN.tga
  SkyboxTextureE = TSCustomE.tga
  SkyboxTextureS = TSCustomS.tga
  SkyboxTextureW = TSCustomW.tga
  SkyboxTextureT = TSCustomT.tga
End
)";

  settings.loadFromINI(ini);

  EXPECT_FLOAT_EQ(settings.transparency.transparentWaterDepth, 5.0f);
  EXPECT_FLOAT_EQ(settings.transparency.minWaterOpacity, 0.5f);
  EXPECT_FLOAT_EQ(settings.transparency.standingWaterColor.r, 0.2f);
  EXPECT_FLOAT_EQ(settings.transparency.standingWaterColor.g, 0.3f);
  EXPECT_FLOAT_EQ(settings.transparency.standingWaterColor.b, 0.8f);
  EXPECT_TRUE(settings.transparency.additiveBlend);
  EXPECT_FLOAT_EQ(settings.transparency.radarColor.r, 0.0f);
  EXPECT_FLOAT_EQ(settings.transparency.radarColor.g, 0.0f);
  EXPECT_FLOAT_EQ(settings.transparency.radarColor.b, 1.0f);
  EXPECT_EQ(settings.transparency.standingWaterTexture, "TWCustomWater.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureN, "TSCustomN.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureE, "TSCustomE.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureS, "TSCustomS.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureW, "TSCustomW.tga");
  EXPECT_EQ(settings.transparency.skyboxTextureT, "TSCustomT.tga");
}

TEST_F(WaterSettingsTest, ParsesMixedWaterSetAndTransparency) {
  const char *ini = R"(
WaterSet Morning
  SkyTexture = Sky.tga
  WaterTexture = Water.tga
  UScrollPerMS = 0.01
  VScrollPerMS = 0.02
End

WaterTransparency
  TransparentWaterDepth = 4.0
  StandingWaterTexture = Custom.tga
End

WaterSet Night
  SkyTexture = NightSky.tga
  WaterTexture = NightWater.tga
End
)";

  settings.loadFromINI(ini);

  const auto &morning = settings.getForTimeOfDay(TimeOfDay::Morning);
  EXPECT_EQ(morning.skyTextureFile, "Sky.tga");
  EXPECT_FLOAT_EQ(morning.uScrollPerMs, 0.01f);

  const auto &night = settings.getForTimeOfDay(TimeOfDay::Night);
  EXPECT_EQ(night.skyTextureFile, "NightSky.tga");

  EXPECT_FLOAT_EQ(settings.transparency.transparentWaterDepth, 4.0f);
  EXPECT_EQ(settings.transparency.standingWaterTexture, "Custom.tga");
}

TEST_F(WaterSettingsTest, GetForTimeOfDayInvalidReturnsFallback) {
  const auto &fallback = settings.getForTimeOfDay(TimeOfDay::Invalid);
  EXPECT_TRUE(fallback.skyTextureFile.empty());
}

TEST_F(WaterSettingsTest, HandlesCommentsInWaterINI) {
  const char *ini = R"(
; Water configuration
WaterSet Morning
  ; Sky texture for morning
  SkyTexture = Sky.tga  ; This is the sky
  WaterTexture = Water.tga
End
)";

  settings.loadFromINI(ini);

  const auto &morning = settings.getForTimeOfDay(TimeOfDay::Morning);
  EXPECT_EQ(morning.skyTextureFile, "Sky.tga");
  EXPECT_EQ(morning.waterTextureFile, "Water.tga");
}

TEST_F(WaterSettingsTest, VertexColorsDefaultToZero) {
  const auto &morning = settings.getForTimeOfDay(TimeOfDay::Morning);
  EXPECT_EQ(morning.vertex00Diffuse.r, 0);
  EXPECT_EQ(morning.vertex00Diffuse.g, 0);
  EXPECT_EQ(morning.vertex00Diffuse.b, 0);
  EXPECT_EQ(morning.vertex00Diffuse.a, 255);
}

TEST_F(WaterSettingsTest, ParsesWindowsLineEndings) {
  std::string ini = "WaterSet Morning\r\n  SkyTexture = Sky.tga\r\nEnd\r\n";

  settings.loadFromINI(ini);

  const auto &morning = settings.getForTimeOfDay(TimeOfDay::Morning);
  EXPECT_EQ(morning.skyTextureFile, "Sky.tga");
}
