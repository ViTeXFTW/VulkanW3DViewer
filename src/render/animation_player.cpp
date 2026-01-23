#include "animation_player.hpp"

#include <algorithm>
#include <cmath>

#include "w3d/chunk_types.hpp"

namespace w3d {

void AnimationPlayer::load(const W3DFile& file) {
  clear();
  sourceFile_ = &file;

  // Load standard animations
  for (size_t i = 0; i < file.animations.size(); ++i) {
    const Animation& anim = file.animations[i];
    AnimationData data;
    data.name = anim.name;
    data.hierarchyName = anim.hierarchyName;
    data.numFrames = anim.numFrames;
    data.frameRate = anim.frameRate > 0 ? anim.frameRate : 15;
    data.isCompressed = false;
    data.fileIndex = i;

    animations_.push_back(data);
    animationNames_.push_back(anim.name);
  }

  // Load compressed animations
  for (size_t i = 0; i < file.compressedAnimations.size(); ++i) {
    const CompressedAnimation& anim = file.compressedAnimations[i];
    AnimationData data;
    data.name = anim.name;
    data.hierarchyName = anim.hierarchyName;
    data.numFrames = anim.numFrames;
    data.frameRate = anim.frameRate > 0 ? anim.frameRate : 15;
    data.isCompressed = true;
    data.fileIndex = i;

    animations_.push_back(data);
    animationNames_.push_back(anim.name);
  }

  // Select first animation if available
  if (!animations_.empty()) {
    currentAnimationIndex_ = 0;
    currentFrame_ = 0.0f;
  }
}

void AnimationPlayer::clear() {
  animations_.clear();
  animationNames_.clear();
  sourceFile_ = nullptr;
  currentAnimationIndex_ = 0;
  currentFrame_ = 0.0f;
  isPlaying_ = false;
}

std::string AnimationPlayer::animationName(size_t index) const {
  if (index < animationNames_.size()) {
    return animationNames_[index];
  }
  return "";
}

bool AnimationPlayer::selectAnimation(size_t index) {
  if (index >= animations_.size()) {
    return false;
  }

  currentAnimationIndex_ = index;
  currentFrame_ = 0.0f;
  return true;
}

float AnimationPlayer::maxFrame() const {
  if (animations_.empty() || currentAnimationIndex_ >= animations_.size()) {
    return 0.0f;
  }

  const AnimationData& anim = animations_[currentAnimationIndex_];
  return static_cast<float>(anim.numFrames > 0 ? anim.numFrames - 1 : 0);
}

uint32_t AnimationPlayer::frameRate() const {
  if (animations_.empty() || currentAnimationIndex_ >= animations_.size()) {
    return 15;
  }

  return animations_[currentAnimationIndex_].frameRate;
}

uint32_t AnimationPlayer::numFrames() const {
  if (animations_.empty() || currentAnimationIndex_ >= animations_.size()) {
    return 0;
  }

  return animations_[currentAnimationIndex_].numFrames;
}

void AnimationPlayer::setFrame(float frame) {
  currentFrame_ = std::clamp(frame, 0.0f, maxFrame());
}

void AnimationPlayer::play() {
  isPlaying_ = true;
}

void AnimationPlayer::pause() {
  isPlaying_ = false;
}

void AnimationPlayer::stop() {
  isPlaying_ = false;
  currentFrame_ = 0.0f;
}

void AnimationPlayer::update(float deltaSeconds) {
  if (!isPlaying_ || animations_.empty() || currentAnimationIndex_ >= animations_.size()) {
    return;
  }

  const AnimationData& anim = animations_[currentAnimationIndex_];
  float framesPerSecond = static_cast<float>(anim.frameRate);
  float deltaFrames = deltaSeconds * framesPerSecond;

  currentFrame_ += deltaFrames;

  float max = maxFrame();

  // Handle playback modes
  switch (playbackMode_) {
    case PlaybackMode::Once:
      if (currentFrame_ > max) {
        currentFrame_ = max;
        isPlaying_ = false;
      }
      break;

    case PlaybackMode::Loop:
      if (currentFrame_ > max) {
        currentFrame_ = std::fmod(currentFrame_, max + 1.0f);
      }
      break;

    case PlaybackMode::PingPong:
      // TODO: Implement ping-pong mode
      // For now, just loop
      if (currentFrame_ > max) {
        currentFrame_ = std::fmod(currentFrame_, max + 1.0f);
      }
      break;
  }
}

bool AnimationPlayer::applyToPose(SkeletonPose& pose, const Hierarchy& hierarchy) const {
  if (!sourceFile_ || animations_.empty() || currentAnimationIndex_ >= animations_.size()) {
    return false;
  }

  const AnimationData& animData = animations_[currentAnimationIndex_];

  // Check if animation matches hierarchy
  if (!animData.hierarchyName.empty() &&
      animData.hierarchyName != hierarchy.name) {
    return false;
  }

  // Prepare translation and rotation arrays for all bones
  std::vector<glm::vec3> translations(hierarchy.pivots.size());
  std::vector<glm::quat> rotations(hierarchy.pivots.size());

  // Initialize with identity values
  for (size_t i = 0; i < hierarchy.pivots.size(); ++i) {
    translations[i] = glm::vec3(0.0f);
    rotations[i] = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
  }

  // Evaluate animation channels
  if (animData.isCompressed) {
    const CompressedAnimation& anim = sourceFile_->compressedAnimations[animData.fileIndex];
    for (size_t i = 0; i < hierarchy.pivots.size(); ++i) {
      translations[i] = evaluateTranslationCompressed(anim, i, currentFrame_);
      rotations[i] = evaluateRotationCompressed(anim, i, currentFrame_);
    }
  } else {
    const Animation& anim = sourceFile_->animations[animData.fileIndex];
    for (size_t i = 0; i < hierarchy.pivots.size(); ++i) {
      translations[i] = evaluateTranslation(anim, i, currentFrame_);
      rotations[i] = evaluateRotation(anim, i, currentFrame_);
    }
  }

  // Apply to pose
  pose.computeAnimatedPose(hierarchy, translations, rotations);

  return true;
}

glm::vec3 AnimationPlayer::evaluateTranslation(const Animation& anim, size_t pivotIndex, float frame) const {
  glm::vec3 translation(0.0f);

  // Find channels for this pivot
  for (const AnimChannel& channel : anim.channels) {
    if (channel.pivot != pivotIndex) {
      continue;
    }

    // Check channel type (X, Y, or Z)
    if (channel.flags == AnimChannelType::X) {
      // X translation
      if (channel.vectorLen == 1 && !channel.data.empty()) {
        int frame0 = static_cast<int>(std::floor(frame));
        int frame1 = frame0 + 1;
        float ratio = frame - static_cast<float>(frame0);

        // Clamp to valid frame range
        frame0 = std::clamp(frame0, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));
        frame1 = std::clamp(frame1, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));

        size_t idx0 = frame0 - channel.firstFrame;
        size_t idx1 = frame1 - channel.firstFrame;

        if (idx0 < channel.data.size() && idx1 < channel.data.size()) {
          translation.x = interpolate(channel.data[idx0], channel.data[idx1], ratio);
        }
      }
    } else if (channel.flags == AnimChannelType::Y) {
      // Y translation
      if (channel.vectorLen == 1 && !channel.data.empty()) {
        int frame0 = static_cast<int>(std::floor(frame));
        int frame1 = frame0 + 1;
        float ratio = frame - static_cast<float>(frame0);

        frame0 = std::clamp(frame0, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));
        frame1 = std::clamp(frame1, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));

        size_t idx0 = frame0 - channel.firstFrame;
        size_t idx1 = frame1 - channel.firstFrame;

        if (idx0 < channel.data.size() && idx1 < channel.data.size()) {
          translation.y = interpolate(channel.data[idx0], channel.data[idx1], ratio);
        }
      }
    } else if (channel.flags == AnimChannelType::Z) {
      // Z translation
      if (channel.vectorLen == 1 && !channel.data.empty()) {
        int frame0 = static_cast<int>(std::floor(frame));
        int frame1 = frame0 + 1;
        float ratio = frame - static_cast<float>(frame0);

        frame0 = std::clamp(frame0, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));
        frame1 = std::clamp(frame1, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));

        size_t idx0 = frame0 - channel.firstFrame;
        size_t idx1 = frame1 - channel.firstFrame;

        if (idx0 < channel.data.size() && idx1 < channel.data.size()) {
          translation.z = interpolate(channel.data[idx0], channel.data[idx1], ratio);
        }
      }
    }
  }

  return translation;
}

glm::quat AnimationPlayer::evaluateRotation(const Animation& anim, size_t pivotIndex, float frame) const {
  glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f); // Identity

  // Find quaternion rotation channel for this pivot
  for (const AnimChannel& channel : anim.channels) {
    if (channel.pivot != pivotIndex) {
      continue;
    }

    // Check for quaternion channel (Q)
    if (channel.flags == AnimChannelType::Q && channel.vectorLen == 4) {
      int frame0 = static_cast<int>(std::floor(frame));
      int frame1 = frame0 + 1;
      float ratio = frame - static_cast<float>(frame0);

      // Clamp to valid frame range
      frame0 = std::clamp(frame0, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));
      frame1 = std::clamp(frame1, static_cast<int>(channel.firstFrame), static_cast<int>(channel.lastFrame));

      size_t idx0 = (frame0 - channel.firstFrame) * 4;
      size_t idx1 = (frame1 - channel.firstFrame) * 4;

      if (idx0 + 3 < channel.data.size() && idx1 + 3 < channel.data.size()) {
        // Extract quaternions (x, y, z, w order in data)
        glm::quat q0(
          channel.data[idx0 + 3],  // w
          channel.data[idx0 + 0],  // x
          channel.data[idx0 + 1],  // y
          channel.data[idx0 + 2]   // z
        );

        glm::quat q1(
          channel.data[idx1 + 3],  // w
          channel.data[idx1 + 0],  // x
          channel.data[idx1 + 1],  // y
          channel.data[idx1 + 2]   // z
        );

        // SLERP between quaternions
        rotation = glm::slerp(q0, q1, ratio);
      }

      break; // Only one quaternion channel per pivot
    }
  }

  return rotation;
}

glm::vec3 AnimationPlayer::evaluateTranslationCompressed(const CompressedAnimation& anim, size_t pivotIndex, float frame) const {
  glm::vec3 translation(0.0f);

  // Find channels for this pivot
  for (const CompressedAnimChannel& channel : anim.channels) {
    if (channel.pivot != pivotIndex) {
      continue;
    }

    // Check channel type using TIMECODED flags
    if (channel.flags == AnimChannelType::TIMECODED_X) {
      // X translation
      if (channel.vectorLen == 1 && !channel.data.empty()) {
        auto [idx0, idx1] = findKeyframes(channel, frame);

        if (idx0 < channel.data.size() && idx1 < channel.data.size()) {
          float frame0 = static_cast<float>(channel.timeCodes[idx0]);
          float frame1 = static_cast<float>(channel.timeCodes[idx1]);
          float ratio = (frame1 > frame0) ? (frame - frame0) / (frame1 - frame0) : 0.0f;

          translation.x = interpolate(channel.data[idx0], channel.data[idx1], ratio);
        }
      }
    } else if (channel.flags == AnimChannelType::TIMECODED_Y) {
      // Y translation
      if (channel.vectorLen == 1 && !channel.data.empty()) {
        auto [idx0, idx1] = findKeyframes(channel, frame);

        if (idx0 < channel.data.size() && idx1 < channel.data.size()) {
          float frame0 = static_cast<float>(channel.timeCodes[idx0]);
          float frame1 = static_cast<float>(channel.timeCodes[idx1]);
          float ratio = (frame1 > frame0) ? (frame - frame0) / (frame1 - frame0) : 0.0f;

          translation.y = interpolate(channel.data[idx0], channel.data[idx1], ratio);
        }
      }
    } else if (channel.flags == AnimChannelType::TIMECODED_Z) {
      // Z translation
      if (channel.vectorLen == 1 && !channel.data.empty()) {
        auto [idx0, idx1] = findKeyframes(channel, frame);

        if (idx0 < channel.data.size() && idx1 < channel.data.size()) {
          float frame0 = static_cast<float>(channel.timeCodes[idx0]);
          float frame1 = static_cast<float>(channel.timeCodes[idx1]);
          float ratio = (frame1 > frame0) ? (frame - frame0) / (frame1 - frame0) : 0.0f;

          translation.z = interpolate(channel.data[idx0], channel.data[idx1], ratio);
        }
      }
    }
  }

  return translation;
}

glm::quat AnimationPlayer::evaluateRotationCompressed(const CompressedAnimation& anim, size_t pivotIndex, float frame) const {
  glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f); // Identity

  // Find quaternion rotation channel for this pivot
  for (const CompressedAnimChannel& channel : anim.channels) {
    if (channel.pivot != pivotIndex) {
      continue;
    }

    // Check for quaternion channel (TIMECODED_Q)
    if (channel.flags == AnimChannelType::TIMECODED_Q && channel.vectorLen == 4) {
      auto [idx0, idx1] = findKeyframes(channel, frame);

      size_t dataIdx0 = idx0 * 4;
      size_t dataIdx1 = idx1 * 4;

      if (dataIdx0 + 3 < channel.data.size() && dataIdx1 + 3 < channel.data.size()) {
        // Extract quaternions (x, y, z, w order in data)
        glm::quat q0(
          channel.data[dataIdx0 + 3],  // w
          channel.data[dataIdx0 + 0],  // x
          channel.data[dataIdx0 + 1],  // y
          channel.data[dataIdx0 + 2]   // z
        );

        glm::quat q1(
          channel.data[dataIdx1 + 3],  // w
          channel.data[dataIdx1 + 0],  // x
          channel.data[dataIdx1 + 1],  // y
          channel.data[dataIdx1 + 2]   // z
        );

        // Calculate interpolation ratio
        float frame0 = static_cast<float>(channel.timeCodes[idx0]);
        float frame1 = static_cast<float>(channel.timeCodes[idx1]);
        float ratio = (frame1 > frame0) ? (frame - frame0) / (frame1 - frame0) : 0.0f;

        // SLERP between quaternions
        rotation = glm::slerp(q0, q1, ratio);
      }

      break; // Only one quaternion channel per pivot
    }
  }

  return rotation;
}

std::pair<size_t, size_t> AnimationPlayer::findKeyframes(const CompressedAnimChannel& channel, float frame) const {
  if (channel.timeCodes.empty()) {
    return {0, 0};
  }

  // Convert frame to uint16_t for comparison
  uint16_t frameCode = static_cast<uint16_t>(std::round(frame));

  // Binary search for the frame
  auto it = std::lower_bound(channel.timeCodes.begin(), channel.timeCodes.end(), frameCode);

  if (it == channel.timeCodes.end()) {
    // Frame is beyond last keyframe
    size_t lastIdx = channel.timeCodes.size() - 1;
    return {lastIdx, lastIdx};
  }

  if (it == channel.timeCodes.begin()) {
    // Frame is before first keyframe
    return {0, 0};
  }

  // Get surrounding keyframes
  size_t idx1 = std::distance(channel.timeCodes.begin(), it);
  size_t idx0 = idx1 - 1;

  return {idx0, idx1};
}

float AnimationPlayer::interpolate(float value0, float value1, float ratio) const {
  return glm::mix(value0, value1, ratio);
}

} // namespace w3d
