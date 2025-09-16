# RMeshImporter
 RMesh Import for Unreal Engine 5
 
 The rmesh format file is a custom format 3D model file for SCP: Containment Breach game. 
 This plugin supports importing RMesh files into Unreal Engine 5. 
 Before use, the game's texture maps need to be manually imported into the/Game/CBAsset/Texture directory. 
 Currently, this directory is fixed, and you can also make custom modifications in the source code. 
 Since lighting maps and collision volumes do not need to be used, 
 they are automatically ignored during import



rmesh格式文件是SCP: Containment Breach游戏的自定义格式的3D模型文件，
此插件支持将RMesh文件导入到虚幻引擎5中，
使用前需手动将游戏的纹理贴图提前导入到/Game/CBAsset/Texture 目录中，
目前这个目录是固定的，您也可以在源码中进行自定义修改，
由于光照贴图和碰撞体积不需要使用，因此在导入时自动进行了忽略