# Auto Part By Distance tool for Modo plug-in

<b>Auto Part By Distance</b> is a tool that clusters groups of connected polygons based on distance. For the clustered polygons, the ID number of the cluster to which each polygon belongs is written into its polygon part tag.<br>

Polygon groups are clustered based on the distances between their bounding boxes; boxes located within a specified search distance are grouped into the same cluster.<br>

If the <b>Search Distance</b> is set to 0, clusters are created for each polygon group.<br>

The search distance is calculated based on the distance within the 2D planes (XY, XZ, or YZ) or the 3D space (XYZ) specified by the spaces.<br>

You can prepend the string specified in the <b>Prefix</b> to the cluster number for polygon part tags.<br>

The number of polygon parts and polygons contained in the currently selected cluster is displayed in the top-left corner of the 3D view. You can change the current cluster by clicking on the bounding box on the 3D view.<br>

Hauling LMB on 3D view adjusts <b>Search Distance</b> value.<br>

This kit contains a direct modeling tool for Modo macOS and Windows.

<div align="left">
<img src="images/autoPart.png" style='max-height: 500px; object-fit: contain'/>
</div>

Nine polygon parts are grouped into four clusters.

<div align="left">
<img src="images/statistics.png" style='max-height: 500px; object-fit: contain'/>
</div>

The number of the assigned cluster is set as a text string in the part tag of each polygon. You can view the part tags in the Statistics panel.

## Installing

- Download lpk from releases. Drag and drop it into your Modo viewport. If you're upgrading, delete previous version.

## How to use Auto Part By Distance tool

- The tool version of Auto Part By Distance can be launched from **Auto Part By Distance** button on **Polygon** tab of **Model** ToolBar on left.

<div align="left">
<img src="images/UI.png" style='max-height: 620px; object-fit: contain'/>
</div>

## Basic Usage with Auto Part By Distance tool

- Activate **Auto Part By Distance** button on **Polygon** tab of **Model** Modo toolbar.
- Lasso elements within selections on 3D view.

