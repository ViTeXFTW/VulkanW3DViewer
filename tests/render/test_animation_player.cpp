#include "render/animation_player.hpp"
#include "w3d/types.hpp"

#include <gtest/gtest.h>

using namespace w3d;

class AnimationPlayerTest : public ::testing::Test {
protected:
  // Create a simple W3D file with animations
  static W3DFile createFileWithAnimation(const std::string &name, uint32_t numFrames,
                                         uint32_t frameRate = 30) {
    W3DFile file;
    Animation anim;
    anim.version = 1;
    anim.name = name;
    anim.hierarchyName = "TestSkeleton";
    anim.numFrames = numFrames;
    anim.frameRate = frameRate;
    file.animations.push_back(anim);
    return file;
  }

  // Create a W3D file with a compressed animation
  static W3DFile createFileWithCompressedAnimation(const std::string &name, uint32_t numFrames,
                                                   uint32_t frameRate = 30) {
    W3DFile file;
    CompressedAnimation anim;
    anim.version = 1;
    anim.name = name;
    anim.hierarchyName = "TestSkeleton";
    anim.numFrames = numFrames;
    anim.frameRate = frameRate;
    anim.flavor = 0;
    file.compressedAnimations.push_back(anim);
    return file;
  }

  // Create a W3D file with multiple animations
  static W3DFile createFileWithMultipleAnimations() {
    W3DFile file;

    Animation idle;
    idle.name = "Idle";
    idle.hierarchyName = "Skeleton";
    idle.numFrames = 30;
    idle.frameRate = 30;
    file.animations.push_back(idle);

    Animation walk;
    walk.name = "Walk";
    walk.hierarchyName = "Skeleton";
    walk.numFrames = 60;
    walk.frameRate = 30;
    file.animations.push_back(walk);

    CompressedAnimation attack;
    attack.name = "Attack";
    attack.hierarchyName = "Skeleton";
    attack.numFrames = 45;
    attack.frameRate = 15;
    file.compressedAnimations.push_back(attack);

    return file;
  }
};

// =============================================================================
// Loading Tests
// =============================================================================

TEST_F(AnimationPlayerTest, LoadEmptyFileHasNoAnimations) {
  W3DFile file;

  AnimationPlayer player;
  player.load(file);

  EXPECT_EQ(player.animationCount(), 0);
}

TEST_F(AnimationPlayerTest, LoadSingleAnimation) {
  auto file = createFileWithAnimation("TestAnim", 30, 15);

  AnimationPlayer player;
  player.load(file);

  EXPECT_EQ(player.animationCount(), 1);
  EXPECT_EQ(player.animationName(0), "TestAnim");
  EXPECT_EQ(player.numFrames(), 30);
  EXPECT_EQ(player.frameRate(), 15);
}

TEST_F(AnimationPlayerTest, LoadCompressedAnimation) {
  auto file = createFileWithCompressedAnimation("CompressedAnim", 60, 30);

  AnimationPlayer player;
  player.load(file);

  EXPECT_EQ(player.animationCount(), 1);
  EXPECT_EQ(player.animationName(0), "CompressedAnim");
  EXPECT_EQ(player.numFrames(), 60);
  EXPECT_EQ(player.frameRate(), 30);
}

TEST_F(AnimationPlayerTest, LoadMultipleAnimations) {
  auto file = createFileWithMultipleAnimations();

  AnimationPlayer player;
  player.load(file);

  EXPECT_EQ(player.animationCount(), 3);
  EXPECT_EQ(player.animationName(0), "Idle");
  EXPECT_EQ(player.animationName(1), "Walk");
  EXPECT_EQ(player.animationName(2), "Attack");
}

TEST_F(AnimationPlayerTest, LoadDefaultsToFirstAnimation) {
  auto file = createFileWithMultipleAnimations();

  AnimationPlayer player;
  player.load(file);

  EXPECT_EQ(player.currentAnimationIndex(), 0);
  EXPECT_FLOAT_EQ(player.currentFrame(), 0.0f);
}

TEST_F(AnimationPlayerTest, ClearRemovesAllAnimations) {
  auto file = createFileWithMultipleAnimations();

  AnimationPlayer player;
  player.load(file);
  EXPECT_EQ(player.animationCount(), 3);

  player.clear();
  EXPECT_EQ(player.animationCount(), 0);
}

TEST_F(AnimationPlayerTest, AnimationNameOutOfRangeReturnsEmpty) {
  auto file = createFileWithAnimation("Test", 10);

  AnimationPlayer player;
  player.load(file);

  EXPECT_EQ(player.animationName(100), "");
}

TEST_F(AnimationPlayerTest, DefaultFrameRateWhenZero) {
  W3DFile file;
  Animation anim;
  anim.name = "NoFrameRate";
  anim.numFrames = 10;
  anim.frameRate = 0; // Invalid
  file.animations.push_back(anim);

  AnimationPlayer player;
  player.load(file);

  EXPECT_EQ(player.frameRate(), 15); // Default
}

// =============================================================================
// Animation Selection Tests
// =============================================================================

TEST_F(AnimationPlayerTest, SelectValidAnimation) {
  auto file = createFileWithMultipleAnimations();

  AnimationPlayer player;
  player.load(file);

  EXPECT_TRUE(player.selectAnimation(1));
  EXPECT_EQ(player.currentAnimationIndex(), 1);
  EXPECT_EQ(player.animationName(player.currentAnimationIndex()), "Walk");
}

TEST_F(AnimationPlayerTest, SelectInvalidAnimationReturnsFalse) {
  auto file = createFileWithAnimation("Test", 10);

  AnimationPlayer player;
  player.load(file);

  EXPECT_FALSE(player.selectAnimation(100));
  EXPECT_EQ(player.currentAnimationIndex(), 0);
}

TEST_F(AnimationPlayerTest, SelectAnimationResetsFrame) {
  auto file = createFileWithMultipleAnimations();

  AnimationPlayer player;
  player.load(file);
  player.setFrame(15.0f);

  player.selectAnimation(1);
  EXPECT_FLOAT_EQ(player.currentFrame(), 0.0f);
}

// =============================================================================
// Playback Control Tests
// =============================================================================

TEST_F(AnimationPlayerTest, InitialStateNotPlaying) {
  auto file = createFileWithAnimation("Test", 30);

  AnimationPlayer player;
  player.load(file);

  EXPECT_FALSE(player.isPlaying());
}

TEST_F(AnimationPlayerTest, PlayStartsPlayback) {
  auto file = createFileWithAnimation("Test", 30);

  AnimationPlayer player;
  player.load(file);
  player.play();

  EXPECT_TRUE(player.isPlaying());
}

TEST_F(AnimationPlayerTest, PauseStopsPlayback) {
  auto file = createFileWithAnimation("Test", 30);

  AnimationPlayer player;
  player.load(file);
  player.play();
  player.pause();

  EXPECT_FALSE(player.isPlaying());
}

TEST_F(AnimationPlayerTest, StopResetsToBeginning) {
  auto file = createFileWithAnimation("Test", 30);

  AnimationPlayer player;
  player.load(file);
  player.setFrame(15.0f);
  player.play();
  player.stop();

  EXPECT_FALSE(player.isPlaying());
  EXPECT_FLOAT_EQ(player.currentFrame(), 0.0f);
}

TEST_F(AnimationPlayerTest, SetFrameClampsToBounds) {
  auto file = createFileWithAnimation("Test", 30);

  AnimationPlayer player;
  player.load(file);

  player.setFrame(-5.0f);
  EXPECT_FLOAT_EQ(player.currentFrame(), 0.0f);

  player.setFrame(100.0f);
  EXPECT_FLOAT_EQ(player.currentFrame(), 29.0f); // maxFrame = numFrames - 1
}

TEST_F(AnimationPlayerTest, MaxFrameIsNumFramesMinusOne) {
  auto file = createFileWithAnimation("Test", 30);

  AnimationPlayer player;
  player.load(file);

  EXPECT_FLOAT_EQ(player.maxFrame(), 29.0f);
}

TEST_F(AnimationPlayerTest, MaxFrameForSingleFrameAnimation) {
  auto file = createFileWithAnimation("Single", 1);

  AnimationPlayer player;
  player.load(file);

  EXPECT_FLOAT_EQ(player.maxFrame(), 0.0f);
}

// =============================================================================
// Playback Mode Tests
// =============================================================================

TEST_F(AnimationPlayerTest, DefaultPlaybackModeIsLoop) {
  AnimationPlayer player;
  EXPECT_EQ(player.playbackMode(), PlaybackMode::Loop);
}

TEST_F(AnimationPlayerTest, SetPlaybackMode) {
  AnimationPlayer player;

  player.setPlaybackMode(PlaybackMode::Once);
  EXPECT_EQ(player.playbackMode(), PlaybackMode::Once);

  player.setPlaybackMode(PlaybackMode::PingPong);
  EXPECT_EQ(player.playbackMode(), PlaybackMode::PingPong);
}

// =============================================================================
// Update Tests
// =============================================================================

TEST_F(AnimationPlayerTest, UpdateAdvancesFrame) {
  auto file = createFileWithAnimation("Test", 30, 30); // 30 fps

  AnimationPlayer player;
  player.load(file);
  player.play();

  // 0.5 seconds at 30 fps = 15 frames
  player.update(0.5f);

  EXPECT_NEAR(player.currentFrame(), 15.0f, 0.01f);
}

TEST_F(AnimationPlayerTest, UpdateWhenNotPlayingDoesNothing) {
  auto file = createFileWithAnimation("Test", 30, 30);

  AnimationPlayer player;
  player.load(file);
  // Not playing

  player.update(1.0f);

  EXPECT_FLOAT_EQ(player.currentFrame(), 0.0f);
}

TEST_F(AnimationPlayerTest, UpdateLoopModeWrapsAround) {
  auto file = createFileWithAnimation("Test", 30, 30); // 30 frames at 30 fps

  AnimationPlayer player;
  player.load(file);
  player.setPlaybackMode(PlaybackMode::Loop);
  player.play();

  // 2 seconds at 30 fps = 60 frames, should wrap to ~0 (modulo 30)
  player.update(2.0f);

  // Frame should have wrapped (60 % 30 = 0)
  EXPECT_LT(player.currentFrame(), 30.0f);
}

TEST_F(AnimationPlayerTest, UpdateOnceModeStopsAtEnd) {
  auto file = createFileWithAnimation("Test", 30, 30);

  AnimationPlayer player;
  player.load(file);
  player.setPlaybackMode(PlaybackMode::Once);
  player.play();

  // 2 seconds should go past end
  player.update(2.0f);

  EXPECT_FLOAT_EQ(player.currentFrame(), 29.0f);
  EXPECT_FALSE(player.isPlaying());
}

TEST_F(AnimationPlayerTest, UpdatePingPongModeReversesAtEnd) {
  auto file = createFileWithAnimation("Test", 30, 30);

  AnimationPlayer player;
  player.load(file);
  player.setPlaybackMode(PlaybackMode::PingPong);
  player.play();

  // Go past end
  player.update(1.5f); // 45 frames at 30 fps

  // Should be somewhere between start and end, reversing direction
  EXPECT_LE(player.currentFrame(), 29.0f);
  EXPECT_GE(player.currentFrame(), 0.0f);
}

TEST_F(AnimationPlayerTest, UpdatePingPongModeReversesAtStart) {
  auto file = createFileWithAnimation("Test", 30, 30);

  AnimationPlayer player;
  player.load(file);
  player.setPlaybackMode(PlaybackMode::PingPong);
  player.setFrame(29.0f);
  player.play();

  // Go to end, reverse, go back to start, reverse again
  player.update(0.1f); // Small step first
  player.update(2.0f); // Should go back and forth

  // Frame should be valid
  EXPECT_GE(player.currentFrame(), 0.0f);
  EXPECT_LE(player.currentFrame(), 29.0f);
}

TEST_F(AnimationPlayerTest, UpdateWithEmptyAnimationsDoesNotCrash) {
  AnimationPlayer player;
  // No animations loaded

  // Should not crash
  player.update(1.0f);

  EXPECT_EQ(player.animationCount(), 0);
}

// =============================================================================
// Frame Rate Tests
// =============================================================================

TEST_F(AnimationPlayerTest, FrameRateWithNoAnimations) {
  AnimationPlayer player;
  EXPECT_EQ(player.frameRate(), 15); // Default
}

TEST_F(AnimationPlayerTest, NumFramesWithNoAnimations) {
  AnimationPlayer player;
  EXPECT_EQ(player.numFrames(), 0);
}

TEST_F(AnimationPlayerTest, MaxFrameWithNoAnimations) {
  AnimationPlayer player;
  EXPECT_FLOAT_EQ(player.maxFrame(), 0.0f);
}

// =============================================================================
// Reload Tests
// =============================================================================

TEST_F(AnimationPlayerTest, LoadReplacesExistingAnimations) {
  auto file1 = createFileWithAnimation("First", 10);
  auto file2 = createFileWithAnimation("Second", 20);

  AnimationPlayer player;
  player.load(file1);
  EXPECT_EQ(player.animationName(0), "First");

  player.load(file2);
  EXPECT_EQ(player.animationCount(), 1);
  EXPECT_EQ(player.animationName(0), "Second");
}

TEST_F(AnimationPlayerTest, LoadResetsPlaybackState) {
  auto file1 = createFileWithAnimation("First", 30);
  auto file2 = createFileWithAnimation("Second", 20);

  AnimationPlayer player;
  player.load(file1);
  player.setFrame(15.0f);
  player.play();

  player.load(file2);

  EXPECT_FLOAT_EQ(player.currentFrame(), 0.0f);
  EXPECT_FALSE(player.isPlaying());
}
