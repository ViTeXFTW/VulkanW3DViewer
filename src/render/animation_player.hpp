#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>

#include "skeleton.hpp"
#include "lib/formats/w3d/types.hpp"

namespace w3d {

// Playback modes
enum class PlaybackMode {
  Once,    // Play once and stop at end
  Loop,    // Loop continuously
  PingPong // Play forward then backward
};

// Animation player - manages animation playback and applies to skeleton
class AnimationPlayer {
public:
  AnimationPlayer() = default;

  // Load animations from a W3D file
  void load(const W3DFile &file);

  // Clear all animations
  void clear();

  // Animation selection
  size_t animationCount() const { return animationNames_.size(); }
  std::string animationName(size_t index) const;
  size_t currentAnimationIndex() const { return currentAnimationIndex_; }
  bool selectAnimation(size_t index);

  // Playback state
  bool isPlaying() const { return isPlaying_; }
  float currentFrame() const { return currentFrame_; }
  float maxFrame() const; // numFrames - 1
  uint32_t frameRate() const;
  uint32_t numFrames() const;

  // Playback control
  void setFrame(float frame); // For slider
  void play();
  void pause();
  void stop(); // Reset to frame 0
  void setPlaybackMode(PlaybackMode mode) { playbackMode_ = mode; }
  PlaybackMode playbackMode() const { return playbackMode_; }

  // Update (call each frame with delta time)
  void update(float deltaSeconds);

  // Apply current animation frame to skeleton pose
  bool applyToPose(SkeletonPose &pose, const Hierarchy &hierarchy) const;

private:
  // Internal animation representation
  struct AnimationData {
    std::string name;
    std::string hierarchyName;
    uint32_t numFrames = 0;
    uint32_t frameRate = 15;
    bool isCompressed = false;
    size_t fileIndex = 0; // Index in animations or compressedAnimations vector
  };

  // Channel evaluation for standard animations
  glm::vec3 evaluateTranslation(const Animation &anim, size_t pivotIndex, float frame) const;
  glm::quat evaluateRotation(const Animation &anim, size_t pivotIndex, float frame) const;

  // Channel evaluation for compressed animations
  glm::vec3 evaluateTranslationCompressed(const CompressedAnimation &anim, size_t pivotIndex,
                                          float frame) const;
  glm::quat evaluateRotationCompressed(const CompressedAnimation &anim, size_t pivotIndex,
                                       float frame) const;

  // Helper: find keyframes for compressed animation channel
  std::pair<size_t, size_t> findKeyframes(const CompressedAnimChannel &channel, float frame) const;

  // Helper: interpolate between two frames
  float interpolate(float value0, float value1, float ratio) const;

  // Loaded animations
  std::vector<AnimationData> animations_;
  std::vector<std::string> animationNames_;

  // Reference to source W3D file (not owned)
  const W3DFile *sourceFile_ = nullptr;

  // Current playback state
  size_t currentAnimationIndex_ = 0;
  float currentFrame_ = 0.0f;
  bool isPlaying_ = false;
  PlaybackMode playbackMode_ = PlaybackMode::Loop;
  float playbackDirection_ = 1.0f; // 1.0f for forward, -1.0f for backward (pingpong)
};

} // namespace w3d
