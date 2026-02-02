# Animations

W3D animation format documentation.

## Overview

W3D supports two animation formats:

1. **Uncompressed Animation** - Full keyframe data
2. **Compressed Animation** - Timecoded or adaptive delta

## Animation Types

### Uncompressed Animation

`ANIMATION` (0x00000200) - Container

```
ANIMATION
├── ANIMATION_HEADER (0x00000201)
├── ANIMATION_CHANNEL (0x00000202) - Multiple
└── BIT_CHANNEL (0x00000203) - Optional, multiple
```

### Compressed Animation

`COMPRESSED_ANIMATION` (0x00000280) - Container

```
COMPRESSED_ANIMATION
├── COMPRESSED_ANIMATION_HEADER (0x00000281)
├── COMPRESSED_ANIMATION_CHANNEL (0x00000282) - Multiple
└── COMPRESSED_BIT_CHANNEL (0x00000283) - Optional, multiple
```

## Animation Header

### Uncompressed Header

`ANIMATION_HEADER` (0x00000201)

```cpp
struct AnimationHeader {
  uint32_t version;         // Format version
  char name[16];            // Animation name
  char hierarchyName[16];   // Target hierarchy name
  uint32_t numFrames;       // Total frame count
  uint32_t frameRate;       // Frames per second
};
```

### Compressed Header

`COMPRESSED_ANIMATION_HEADER` (0x00000281)

```cpp
struct CompressedAnimationHeader {
  uint32_t version;
  char name[16];
  char hierarchyName[16];
  uint32_t numFrames;
  uint16_t frameRate;
  uint16_t flavor;          // Compression type
};
```

**Flavor values**:

| Value | Type |
|-------|------|
| 0 | Timecoded |
| 1 | Adaptive Delta |

## Animation Channels

### Channel Types

Each channel animates one component of one bone:

```cpp
namespace AnimChannelType {
  constexpr uint16_t X = 0;   // Translation X
  constexpr uint16_t Y = 1;   // Translation Y
  constexpr uint16_t Z = 2;   // Translation Z
  constexpr uint16_t XR = 3;  // Rotation X (Euler)
  constexpr uint16_t YR = 4;  // Rotation Y (Euler)
  constexpr uint16_t ZR = 5;  // Rotation Z (Euler)
  constexpr uint16_t Q = 6;   // Quaternion rotation
}
```

### Uncompressed Channel

`ANIMATION_CHANNEL` (0x00000202)

```cpp
struct AnimationChannel {
  uint16_t firstFrame;   // Start frame
  uint16_t lastFrame;    // End frame
  uint16_t vectorLen;    // Values per key (1 or 4 for quaternion)
  uint16_t flags;        // Channel type
  uint16_t pivot;        // Bone index
  uint16_t padding;
  // Followed by: float data[frameCount * vectorLen]
};
```

**Reading channel data**:

```cpp
AnimChannel readChannel(ChunkReader& reader) {
  AnimChannel ch;
  ch.firstFrame = reader.read<uint16_t>();
  ch.lastFrame = reader.read<uint16_t>();
  ch.vectorLen = reader.read<uint16_t>();
  ch.flags = reader.read<uint16_t>();
  ch.pivot = reader.read<uint16_t>();
  reader.skip(2);  // padding

  uint32_t frameCount = ch.lastFrame - ch.firstFrame + 1;
  uint32_t dataCount = frameCount * ch.vectorLen;

  ch.data.resize(dataCount);
  reader.read(ch.data.data(), dataCount * sizeof(float));

  return ch;
}
```

### Compressed Channel

`COMPRESSED_ANIMATION_CHANNEL` (0x00000282)

```cpp
struct CompressedAnimChannel {
  uint32_t numTimeCodes;  // Number of keyframes
  uint16_t pivot;         // Bone index
  uint16_t vectorLen;     // 1 or 4
  uint16_t flags;         // Channel type + compression
  uint16_t padding;
  // Followed by:
  // - uint16_t timeCodes[numTimeCodes]  (Timecoded)
  // - float data[numTimeCodes * vectorLen]
};
```

**Compressed channel types**:

```cpp
namespace AnimChannelType {
  constexpr uint16_t TIMECODED_X = 0;
  constexpr uint16_t TIMECODED_Y = 1;
  constexpr uint16_t TIMECODED_Z = 2;
  constexpr uint16_t TIMECODED_Q = 3;
  constexpr uint16_t ADAPTIVEDELTA_X = 4;
  constexpr uint16_t ADAPTIVEDELTA_Y = 5;
  constexpr uint16_t ADAPTIVEDELTA_Z = 6;
  constexpr uint16_t ADAPTIVEDELTA_Q = 7;
}
```

## Bit Channels

Visibility/binary channels for toggling bone visibility:

### Uncompressed Bit Channel

`BIT_CHANNEL` (0x00000203)

```cpp
struct BitChannel {
  uint16_t firstFrame;
  uint16_t lastFrame;
  uint16_t flags;       // Always 0
  uint16_t pivot;       // Bone index
  float defaultVal;     // Default visibility (0 or 1)
  // Followed by: uint8_t data[ceil(frameCount / 8)]
};
```

Bits are packed, one per frame:

```cpp
bool getVisibility(const BitChannel& ch, int frame) {
  if (frame < ch.firstFrame || frame > ch.lastFrame) {
    return ch.defaultVal >= 0.5f;
  }

  int localFrame = frame - ch.firstFrame;
  int byteIndex = localFrame / 8;
  int bitIndex = localFrame % 8;

  return (ch.data[byteIndex] >> bitIndex) & 1;
}
```

## Interpolation

### Linear Interpolation

For translation channels:

```cpp
float interpolate(const AnimChannel& ch, float frame) {
  if (frame <= ch.firstFrame) return ch.data[0];
  if (frame >= ch.lastFrame) return ch.data.back();

  float localFrame = frame - ch.firstFrame;
  int frame0 = static_cast<int>(localFrame);
  int frame1 = frame0 + 1;
  float t = localFrame - frame0;

  return glm::mix(ch.data[frame0], ch.data[frame1], t);
}
```

### Quaternion Interpolation

For rotation channels, use slerp:

```cpp
glm::quat interpolateQuat(const AnimChannel& ch, float frame) {
  float localFrame = frame - ch.firstFrame;
  int frame0 = static_cast<int>(localFrame) * 4;
  int frame1 = frame0 + 4;
  float t = localFrame - static_cast<int>(localFrame);

  glm::quat q0(ch.data[frame0+3], ch.data[frame0], ch.data[frame0+1], ch.data[frame0+2]);
  glm::quat q1(ch.data[frame1+3], ch.data[frame1], ch.data[frame1+1], ch.data[frame1+2]);

  return glm::slerp(q0, q1, t);
}
```

### Timecoded Interpolation

For compressed animations with timecodes:

```cpp
float interpolateTimecoded(const CompressedAnimChannel& ch, float frame) {
  // Find surrounding keyframes
  int key0 = 0;
  for (int i = 0; i < ch.timeCodes.size(); i++) {
    if (ch.timeCodes[i] <= frame) key0 = i;
    else break;
  }
  int key1 = std::min(key0 + 1, (int)ch.timeCodes.size() - 1);

  // Interpolation factor
  float t = 0;
  if (ch.timeCodes[key1] != ch.timeCodes[key0]) {
    t = (frame - ch.timeCodes[key0]) /
        (ch.timeCodes[key1] - ch.timeCodes[key0]);
  }

  return glm::mix(ch.data[key0], ch.data[key1], t);
}
```

## Applying Animation

### Update Bone Transforms

```cpp
void applyAnimation(
    const Animation& anim,
    float frame,
    std::vector<Pivot>& pose
) {
  for (const auto& channel : anim.channels) {
    Pivot& pivot = pose[channel.pivot];

    switch (channel.flags) {
      case AnimChannelType::X:
        pivot.translation.x = interpolate(channel, frame);
        break;
      case AnimChannelType::Y:
        pivot.translation.y = interpolate(channel, frame);
        break;
      case AnimChannelType::Z:
        pivot.translation.z = interpolate(channel, frame);
        break;
      case AnimChannelType::Q:
        pivot.rotation = interpolateQuat(channel, frame);
        break;
    }
  }
}
```

### Animation Playback

```cpp
class AnimationPlayer {
  float currentFrame = 0;
  float speed = 1.0f;
  bool loop = true;

  void update(float deltaTime) {
    currentFrame += deltaTime * animation->frameRate * speed;

    if (currentFrame >= animation->numFrames) {
      if (loop) {
        currentFrame = fmod(currentFrame, animation->numFrames);
      } else {
        currentFrame = animation->numFrames - 1;
      }
    }
  }
};
```

## Animation Blending

For smooth transitions between animations:

```cpp
void blendAnimations(
    const Animation& animA,
    const Animation& animB,
    float frameA,
    float frameB,
    float blendFactor,
    std::vector<Pivot>& pose
) {
  // Apply animation A
  std::vector<Pivot> poseA = restPose;
  applyAnimation(animA, frameA, poseA);

  // Apply animation B
  std::vector<Pivot> poseB = restPose;
  applyAnimation(animB, frameB, poseB);

  // Blend
  for (size_t i = 0; i < pose.size(); i++) {
    pose[i].translation = glm::mix(
      poseA[i].translation, poseB[i].translation, blendFactor);
    pose[i].rotation = glm::slerp(
      poseA[i].rotation, poseB[i].rotation, blendFactor);
  }
}
```
