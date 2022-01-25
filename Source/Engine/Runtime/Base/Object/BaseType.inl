
/*================================
			AObject
================================*/

mxFORCEINLINE
bool AObject::IsA( const TbMetaClass& type ) const
{
	return this->rttiClass().IsDerivedFrom( type );
}

mxFORCEINLINE
bool AObject::IsA( const TypeGUID& typeCode ) const
{
	return this->rttiClass().IsDerivedFrom( typeCode );
}

mxFORCEINLINE
bool AObject::IsInstanceOf( const TbMetaClass& type ) const
{
	return ( this->rttiClass() == type );
}

mxFORCEINLINE
bool AObject::IsInstanceOf( const TypeGUID& typeCode ) const
{
	return ( this->rttiClass().GetTypeGUID() == typeCode );
}

mxFORCEINLINE
const char* AObject::rttiTypeName() const
{
	return this->rttiClass().GetTypeName();
}

mxFORCEINLINE
const TypeGUID AObject::rttiTypeID() const
{
	return this->rttiClass().GetTypeGUID();
}

mxFORCEINLINE
TbMetaClass & AObject::metaClass()
{
	return ms_staticTypeInfo;
}

mxFORCEINLINE
TbMetaClass & AObject::rttiClass() const
{
	return ms_staticTypeInfo;
}

//----------------------------------------------------------//
//				End Of File.									//
//----------------------------------------------------------//
