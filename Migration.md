# Note on deprecations
A tick-tock release cycle allows easy migration to new software versions.
Obsolete code is marked as deprecated for one major release.
Deprecated code produces compile-time warnings. These warning serve as
notification to users that their code should be upgraded. The next major
release will remove the deprecated code.


## Ignition Rendering 2.X to 3.X

### Deletions

1. **Ogre2DepthCamera.hh**
    + Removed unused member variables `captureData` and `newData`

## Ignition Rendering 1.X to 2.X

### Modifications

1. **Visual.hh** and **Node.hh**
    + All functions for setting and getting scale properties in the Visual class
      are moved to the base Node class. These functions are now inherited by
      the Visual class.
