## Asset/Resource/Content Building Pipeline

### TODO

- allow multiple asset data files to be built from a single source file


select * from Assets where AssetTypeName = 'Rendering::NwMesh';
select * from Assets where AssetTypeName = 'Rendering::NwSkinnedMesh';
select * from Assets where AssetTypeName = 'Rendering::NwAnimClip';
select * from Assets where AssetTypeName = 'idEntityDef';

delete from Assets where AssetTypeName = 'Rendering::NwMesh';
delete from Assets where AssetTypeName = 'Rendering::NwAnimClip';
delete from Assets where AssetTypeName = 'Rendering::NwSkinnedMesh';
delete from Assets where AssetTypeName = 'idEntityDef';
delete from Assets where AssetTypeName = 'NwSoundSource';
delete from Assets where AssetTypeName = 'NwModel';





delete from Assets where AssetTypeName = 'NwClump';

delete from Assets where AssetTypeName = 'NwVertexShader';
delete from Assets where AssetTypeName = 'NwPixelShader';
delete from Assets where AssetTypeName = 'NwGeometryShader';
delete from Assets where AssetTypeName = 'NwComputeShader';
delete from Assets where AssetTypeName = 'NwShaderProgram';
delete from Assets where AssetTypeName = 'NwShaderEffect';






select * from Assets where AssetName = 'spaceship-alien-fighter';

delete from Assets where AssetName = 'spaceship-alien-fighter';

delete from Assets where AssetName = 'small_fighter';