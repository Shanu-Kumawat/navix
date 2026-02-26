# NAVIX Project Overview

## Vision
NAVIX is a Finite Element Method (FEM) analysis software designed with a strong emphasis on usability and accessibility for engineers. The goal is to enable users to easily work with predefined engineering components (e.g., beams, shock absorbers, ball bearings) and perform robust finite element analysis.

## Current State (As of Feb 2026)
The application currently functions as a 2D drawing and 3D visualization tool. It supports:
- Basic 2D shape drawing (lines, circles, splines, bezier curves).
- Predefined complex engineering components (Bellows, Ball Bearings, Shock Absorbers).
- 3D visualization of these components using OpenGL.
- A user interface built with ImGui and SDL2.

## Future Goals
- **Meshing**: Integration of robust meshing capabilities (e.g., Gmsh) to convert geometric models into finite element meshes.
- **FEA Solver**: Integration of a numerical solver (e.g., using Eigen for linear algebra) to perform structural analysis.
- **Post-Processing**: Visualization of analysis results (e.g., stress contours, displacements).

## Core Technologies
- **Language**: C++17
- **UI Framework**: Dear ImGui
- **Windowing/Input**: SDL2
- **Graphics API**: OpenGL (via GLAD)
- **Math**: Custom `ImVec2` math (currently being evaluated for replacement with GLM `double` precision).
