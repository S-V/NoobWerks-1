# Resource/Asset Management

## TODO:
- AssetID's size should always be 8 bytes
- allow both string and numeric (integer) Asset IDs

- don't use reference counting, use "resource labels":
https://github.com/floooh/oryol/blob/master/code/Modules/Resource/README.md

- create a smart pointer class (like TResPtr) that can cache assets by value
to avoid derefencing if the asset is a small object,
e.g. a TextureHandle, a RenderStateMask, ...
