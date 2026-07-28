# UV Transform tool for Modo plug-in

<b>UV Transform</b> is a tool for moving, rotating, and scaling UV values ​​in both the UV view and the 3D view. It allows you to modify the UV values ​​mapped to polygons selected in the 3D view, using a designated reference polygon as a basis. When the <b>Tweak</b> option is enabled, dragging an element (polygon, edge, or vertex) with the mouse cursor modifies the UV values ​​mapped to that element.

The reference polygon is the polygon displayed in magenda triangle in the 3D view; you can change it by clicking on a polygon by the Left Mouse Button (LMB).

<b>Center of Selection to Pivot</b> moves the rotation and scale pivot to the center of UV bounding box with selected elements. The blue cross mark indicates the center position of the selection. Dragging the center handle snaps to this center position.

<b>Tear Off</b> split selected polygons from remaining polygons in UV space.

This kit contains a direct modeling tool for Modo macOS and Windows.

## Installing

- Download lpk from releases. Drag and drop it into your Modo viewport. If you're upgrading, delete previous version.

## How to use UV Transform tool

- The tool version of UV Transform can be launched from **UV Transform** button on **UV** ToolBar on left.

<div align="left">
<img src="images/UI.png" style='max-height: 620px; object-fit: contain'/>
</div>

## Basic Usage with UV Transform tool

- Activate **UV Transform** button on **UV** tab of **Model** Modo toolbar.

## Mouse Operations

- <b>LMB Click and Hauling</b> : Set the reference polygon and translate UV values.
- <b>Control-LMB Click and Hauling</b> : Set the reference polygon and translate UV values along constraint axis.
- <b>Control-LMB Dragging Scale Handle</b> : Uniform scale.
- <b>Control-LMB Dragging Rotate Handle</b> : Rotate angle with 15 degree snapping.
- <b>RMB Click and Hauling</b> : Set the reference polygon and uniform scale UV values.
- <b>Control-RMB Click and Hauling</b> : Set the reference polygon and scale UV values.

