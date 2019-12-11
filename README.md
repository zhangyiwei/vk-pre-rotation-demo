# Vulkan Pre-rotation Demo

## What's covered?

1. Detect all surface rotations in Android 10+(easier if landscape only without resizing).
2. Handle swapchain recreation.
3. Fix the shaders in clipping space with a simple 2x2 matrix.

## What's not covered?

1. Detect surface rotation in Android Pie and below, which needs jni for Display.getRotation().
2. Fix advanced shader features like dfdx and dfdy, which can be fixed by mapping +-dfdx, +-dfdy to the preTransform pushed to the shader.
3. Other miscellaneous.

