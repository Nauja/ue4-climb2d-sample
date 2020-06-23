# ue4-climb2d-sample

![UE4](https://img.shields.io/badge/UE4-4.25+-blue)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/Nauja/ue4-jetpack-sample/master/LICENSE)

Sample of a custom climbing movement done with Paper2D.

![Preview](https://github.com/Nauja/ue4-climb2d-sample/raw/master/docs/preview.gif)

This project is an example of how to write a custom climbing movement in a Paper2D game, with the constraint of
being fully replicated over network.

Keyboard/Gamepad controls:
  * Z(A)QSD/Left Thumbstick: move
  * Space/Face Bottom Button: jump
  * Left CTRL/Right Trigger (hold): climb

Prerequisites:
  * [Pixel Perfect 2D Sample](https://github.com/Nauja/ue4-pixelperfect2d-sample)

Features:
  * Detecting when character can climb

### Detecting when character can climb

While the map is created with a TileMap, it is not used to identify the tiles that character can climb.
Instead we use climbable volumes that are directly placed in the level to represent the climbable surfaces:

![Preview](https://github.com/Nauja/ue4-climb2d-sample/raw/master/docs/editor-climbable-volume.png)

The implementation of `ASampleClimbableVolume.cpp` is quite simple as it only serve to detect overlapping with
the character:

```cpp
void ASampleClimbableVolume::NotifyActorBeginOverlap(class AActor* Other)
{
    Super::NotifyActorBeginOverlap(Other);

    if (IsValid(Other) && !IsPendingKill())
    {
        StaticCast<ASampleCharacter*>(Other)->AddClimbableVolume(this);
    }
}

void ASampleClimbableVolume::NotifyActorEndOverlap(class AActor* Other)
{
    Super::NotifyActorEndOverlap(Other);

    if (IsValid(Other) && !IsPendingKill())
    {
        StaticCast<ASampleCharacter*>(Other)->RemoveClimbableVolume(this);
    }
}
```

### Credits

Sprites are coming from [The Spriters Resource](https://www.spriters-resource.com/).

Font from [FontSpace](https://www.fontspace.com/atlantis-international-font-f31357).
